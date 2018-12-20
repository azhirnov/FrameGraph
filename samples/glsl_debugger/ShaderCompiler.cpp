// Copyright (c) 2018,  Zhirnov Andrey. For more information see 'LICENSE'

#include "ShaderCompiler.h"
#include "stl/Algorithms/StringUtils.h"
#include "stl/Algorithms/StringParser.h"
#include "stl/Stream/FileStream.h"

// glslang includes
#include "glslang/glslang/Include/revision.h"
#include "glslang/glslang/OSDependent/osinclude.h"
#include "glslang/glslang/MachineIndependent/localintermediate.h"
#include "glslang/glslang/Include/intermediate.h"
#include "glslang/SPIRV/doc.h"
#include "glslang/SPIRV/disassemble.h"
#include "glslang/SPIRV/GlslangToSpv.h"
#include "glslang/SPIRV/GLSL.std.450.h"
#include "glslang/StandAlone/ResourceLimits.cpp"

using namespace glslang;


ShaderCompiler::ShaderCompiler ()
{
	glslang::InitializeProcess();
	_tempBuf.reserve( 1024 );
	_tempStr.reserve( 1024 );
}

ShaderCompiler::~ShaderCompiler ()
{
	glslang::FinalizeProcess();
}

/*
=================================================
	Compile
=================================================
*/
bool ShaderCompiler::Compile  (OUT VkShaderModule &		shaderModule,
							   const VulkanDevice &		vulkan,
							   ArrayView<const char *>	source,
							   EShLanguage				shaderType,
							   bool						debuggable,
							   EShTargetLanguageVersion	spvVersion)
{
	ShaderDebugInfo			debug_info;
	Array<const char *>		shader_src;
	const FG::String		header	= "#version 460 core\n"s <<
									  "#extension GL_ARB_separate_shader_objects : require\n" <<
									  "#extension GL_ARB_shading_language_420pack : require\n";
	
	shader_src.push_back( header.data() );

	if ( debuggable )
		shader_src.push_back( _DeclStorageBuffer().data() );

	shader_src.insert( shader_src.end(), source.begin(), source.end() );

	if ( not _Compile( OUT _tempBuf, OUT debuggable ? &debug_info : null, shader_src, shaderType, spvVersion ) )
		return false;

	VkShaderModuleCreateInfo	info = {};
	info.sType		= VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
	info.flags		= 0;
	info.pCode		= _tempBuf.data();
	info.codeSize	= size_t(ArraySizeOf( _tempBuf ));

	VK_CHECK( vulkan.vkCreateShaderModule( vulkan.GetVkDevice(), &info, null, OUT &shaderModule ));

	if ( debuggable ) {
		_debuggableShaders.insert_or_assign( shaderModule, std::move(debug_info) );
	}
	return true;
}

/*
=================================================
	_Compile
=================================================
*/
bool ShaderCompiler::_Compile (OUT Array<uint>&			spirvData,
							   OUT ShaderDebugInfo*		dbgInfo,
							   ArrayView<const char *>	source,
							   EShLanguage				shaderType,
							   EShTargetLanguageVersion	spvVersion)
{

	EShMessages					messages		= EShMsgDefault;
	TProgram					program;
	TShader						shader			{ shaderType };
	EshTargetClientVersion		client_version	= EShTargetVulkan_1_1;
	TBuiltInResource			builtin_res		= DefaultTBuiltInResource;

	shader.setStrings( source.data(), int(source.size()) );
	shader.setEntryPoint( "main" );
	shader.setEnvInput( EShSourceGlsl, shaderType, EShClientVulkan, 110 );
	shader.setEnvClient( EShClientVulkan, client_version );
	shader.setEnvTarget( EshTargetSpv, spvVersion );

	if ( not shader.parse( &builtin_res, 460, ENoProfile, false, true, messages ) )
	{
		FG_LOGI( shader.getInfoLog() );
		return false;
	}
		
	program.addShader( &shader );

	if ( not program.link( messages ) )
	{
		FG_LOGI( program.getInfoLog() );
		return false;
	}

	const TIntermediate *	intermediate = program.getIntermediate( shader.getStage() );
	if ( not intermediate )
		return false;

	if ( dbgInfo )
	{
		CHECK_ERR( _GenerateDebugInfo( INOUT *intermediate, OUT *dbgInfo ));
		
		dbgInfo->source.reserve( source.size() );

		for (auto& src : source) {
			dbgInfo->source.push_back( src );
		}
	}

	SpvOptions				spv_options;
	spv::SpvBuildLogger		logger;

	spv_options.generateDebugInfo	= false;
	spv_options.disableOptimizer	= true;
	spv_options.optimizeSize		= false;
	spv_options.validate			= true;
		
	spirvData.clear();
	GlslangToSpv( *intermediate, OUT spirvData, &logger, &spv_options );

	if ( spirvData.empty() )
		return false;

	return true;
}

/*
=================================================
	_GetSourceLine
=================================================
*/
bool ShaderCompiler::_GetSourceLine (const ShaderDebugInfo &info, uint locationId, OUT FG::String &str)
{
	CHECK_ERR( locationId < info.srcLocations.size() );
	auto&	loc = info.srcLocations[ locationId ];

	CHECK_ERR( loc.sourceId < info.source.size() );
	auto	src = StringView{ info.source[ loc.sourceId ] };

	size_t	pos = 0;
	CHECK( StringParser::MoveToLine( src, INOUT pos, Max( loc.begin.Line(), 1u ) - 1 ));

	const int	count	= int(loc.end.Line() - loc.begin.Line() + 1);

	for (int i = 0; i < count; ++i)
	{
		str << ToString(loc.begin.Line() + i) << ". ";
		
		const size_t	start = pos;
		StringParser::ToEndOfLine( src, INOUT pos );

		str << src.substr( start, pos - start ) << "\n";

		StringParser::ToNextLine( src, INOUT pos );
	}
	return true;
}

/*
=================================================
	ParseDebugOutput
=================================================
*/
bool ShaderCompiler::ParseDebugOutput (VkShaderModule shaderModule, const void *ptr, BytesU maxSize, BytesU posOffset) const
{
	auto	iter = _debuggableShaders.find( shaderModule );
	CHECK_ERR( iter != _debuggableShaders.end() );

	const auto&		info		= iter->second;
	const uint*		data_ptr	= Cast<uint>( ptr + info.dataOffset );
	const uint64_t	count		= Min( *Cast<uint>(ptr + info.posOffset), uint64_t(maxSize - info.dataOffset) / sizeof(uint) );
	const uint*		end_ptr		= data_ptr + count;

	CHECK_ERR( info.posOffset == posOffset );
	FG_LOGI( "Recorded "s << ToString(count) << " variables" );

	FG::String		str;	str.reserve( 1024 );
	FileWStream		file{ R"(shader_debug_output.glsl)" };
	CHECK_ERR( file.IsOpen() );

	for (; data_ptr < end_ptr;)
	{
		uint		loc_id		= *data_ptr;		++data_ptr;
		uint		type		= *data_ptr;		++data_ptr;
		TBasicType	t_basic		= TBasicType(type & 0xFF);
		uint		vec_size	= (type >> 8) & 0xFF;
		
		CHECK_ERR( vec_size > 0 and vec_size <= 4 );
		str << "// ";

		for (uint i = 0; i < vec_size; ++i, ++data_ptr) {
			switch ( t_basic ) {
				case TBasicType::EbtFloat : {
					if ( i == 0 )	str << "float" << ToString(vec_size) << "{ ";
					else			str << ", ";
					str << ToString( BitCast<float>( *data_ptr ));
					break;
				}

				case TBasicType::EbtInt : {
					if ( i == 0 )	str << "int" << ToString(vec_size) << "{ ";
					else			str << ", ";
					str << ToString( int(*data_ptr) );
					break;
				}

				case TBasicType::EbtUint : {
					if ( i == 0 )	str << "uint" << ToString(vec_size) << "{ ";
					else			str << ", ";
					str << ToString( *data_ptr );
					break;
				}

				default :
					RETURN_ERR( "not supported" );
			}
		}

		str << " }\n";
		CHECK_ERR( _GetSourceLine( info, loc_id, INOUT str ));
		str << "\n";
		
		CHECK( file.Write( StringView{str} ));
		str.clear();
	}

	return true;
}

/*
=================================================
	_DeclStorageBuffer
=================================================
*/
StringView  ShaderCompiler::_DeclStorageBuffer ()
{
	return R"#(
	layout(binding=1, std430) buffer DebugOutputStorage
	{
		// vertex shader
		uint	vs_InstanceID;
		uint	vs_VertexID;

		// tessellation control shader
		uint	tcs_InvocationID;
		uint	tcs_PrimitiveID;

		// tessellation evaluation shader
		//uint	tes_TessCoord;

		// geometry shader
		uint	gs_InvocationID;
		uint	gs_PrimitiveID;

		// fragment shader
		uvec2	fs_Coord;
		uint	fs_PrimitiveID;
		uint	fs_SampleID;
		uint	fs_ViewportIndex;
		uint	fs_Layer;
	
		// compute shader
		uvec3	cs_globalInvocationID;

		// mesh shader
		// TODO...

		// tast shader
		// TODO...

		// ray tracing shaders
		// TODO...

		// output data
		uint	position;
		uint	outData [];

	} dbgStorage;

)#";
}
//-----------------------------------------------------------------------------



struct DebugInfo
{
private:
	using SrcLoc = ShaderCompiler::SourceLocation;

	struct StackFrame
	{
		TIntermNode*		node;
		SrcLoc				loc;

		StackFrame () {}
		explicit StackFrame (TIntermNode* node) : node{node}, loc{node->getLoc().string, node->getLoc().line, node->getLoc().column} {}
	};


public:
	Array<StackFrame>					_callStack;
	TIntermTyped *						_injection		= null;

	HashSet<TString>					_requiredFunctions;
	HashMap<TString, TIntermSymbol*>	_cachedSymbols;

	Array<SrcLoc>						_srcLocations;

	int									_maxSymbolId	= 0;

	TIntermSymbol *						_dbgStorage		= null;
	TIntermBinary *						_dbgStoragePos	= null;
	TIntermBinary *						_dbgStorageData	= null;

	BytesU								_posOffset;
	BytesU								_dataOffset;


public:
	DebugInfo ()
	{
		_requiredFunctions.insert( "IsCurrentShader(" );
	}


	void Enter (TIntermNode* node)
	{
		_callStack.push_back(StackFrame{ node });
	}

	void Leave (TIntermNode* node)
	{
		CHECK( _callStack.back().node == node );
		_callStack.pop_back();
	}


	ND_ TIntermTyped*  GetInjection ()
	{
		auto	temp = _injection;
		_injection = null;
		return temp;
	}

	void InjectNode (TIntermTyped *node)
	{
		CHECK( not _injection );
		_injection = node;
	}


	ND_ uint  GetSourceLocation ()
	{
		const auto&	loc = _callStack.back().loc;
		ASSERT( loc.sourceId != UMax );

		_srcLocations.push_back( loc );
		return uint(_srcLocations.size()-1);
	}

	void GetSourceLocationsMap (OUT Array<SrcLoc> &result)
	{
		std::swap( result, _srcLocations );
	}

	ND_ SrcLoc const&  GetCurrentLocation () const
	{
		return _callStack.back().loc;
	}
	
	void AddLocation (const TSourceLoc &loc)
	{
		return AddLocation({ uint(loc.string), uint(loc.line), uint(loc.column) });
	}

	void AddLocation (const SrcLoc &src)
	{
		if ( _callStack.empty() )
			return;

		auto&	dst = _callStack.back().loc;

		if ( dst.sourceId == 0 and dst.begin.value == 0 and dst.end.value == 0 )
		{
			dst = src;
			return;
		}

		CHECK( src.sourceId == dst.sourceId );
		dst.begin.value = Min( dst.begin.value,	src.begin.value );
		dst.end.value	= Max( dst.end.value,	src.end.value );
	}


	void RequestFunc (const TString &fname)
	{
		_requiredFunctions.insert( fname );
	}

	ND_ HashSet<TString> const&  GetRequiredFunctions () const
	{
		return _requiredFunctions;
	}


	void AddSymbolID (int id)
	{
		_maxSymbolId = Max( _maxSymbolId, id );
	}

	ND_ int  GetUniqueSymbolID ()
	{
		return ++_maxSymbolId;
	}


	void CacheSymbolNode (TIntermSymbol* node)
	{
		_cachedSymbols.insert_or_assign( node->getName(), node );
	}

	ND_ TIntermSymbol*  GetCachedSymbolNode (const TString &name)
	{
		auto	iter = _cachedSymbols.find( name );
		return	iter != _cachedSymbols.end() ? iter->second : null;
	}


	ND_ TIntermBinary*  GetDebugStoragePositionField () const
	{
		CHECK( _dbgStoragePos );
		return _dbgStoragePos;
	}

	ND_ TIntermBinary*  GetDebugStorageDataField () const
	{
		CHECK( _dbgStorageData );
		return _dbgStorageData;
	}

	ND_ TIntermSymbol*  GetDebugStorage () const
	{
		CHECK( _dbgStorage );
		return _dbgStorage;
	}

	bool SetDebugStorage (TIntermSymbol* symb)
	{
		if ( _dbgStorage )
			return true;

		CHECK_ERR( symb );
		CHECK_ERR( symb->getType().isStruct() );

		_dbgStorage = symb;
		
		TPublicType		uint_type;
		uint_type.init( Default );
		uint_type.basicType			= TBasicType::EbtInt;
		uint_type.vectorSize		= 1;
		uint_type.qualifier.storage = TStorageQualifier::EvqConst;
		
		const TTypeList&	st_list	= *_dbgStorage->getType().getStruct();
		CHECK_ERR( st_list.size() > 2 );

		const TType&		pos_type	= *(st_list.end()-2)->type;
		const TType&		data_type	= *(st_list.end()-1)->type;
		CHECK_ERR( pos_type.getFieldName()  == "position" );
		CHECK_ERR( data_type.getFieldName() == "outData" );

		TConstUnionArray		pos_index(1);	pos_index[0].setIConst( int(st_list.size())-2 );
		TConstUnionArray		data_index(1);	data_index[0].setIConst( int(st_list.size())-1 );
		TIntermConstantUnion*	pos_field		= new TIntermConstantUnion{ pos_index, TType{uint_type} };
		TIntermConstantUnion*	data_field		= new TIntermConstantUnion{ data_index, TType{uint_type} };
		
		_dbgStoragePos  = new TIntermBinary{ TOperator::EOpIndexDirectStruct };	// dbgStorage.position
		_dbgStorageData = new TIntermBinary{ TOperator::EOpIndexDirectStruct };	// dbgStorage.outData

		_dbgStoragePos->setType( pos_type );
		_dbgStoragePos->setLeft( _dbgStorage );
		_dbgStoragePos->setRight( pos_field );

		_dbgStorageData->setType( data_type );
		_dbgStorageData->setLeft( _dbgStorage );
		_dbgStorageData->setRight( data_field );

		_posOffset	= BytesU{uint( pos_type.getQualifier().layoutOffset )};
		_dataOffset	= BytesU{uint( data_type.getQualifier().layoutOffset )};
		return true;
	}

	void GetDebugStorageFieldsOffset (OUT BytesU &pos, OUT BytesU &data) const
	{
		pos		= _posOffset;
		data	= _dataOffset;
	}
};

static bool RecursiveProcessAggregateNode (TIntermAggregate* node, DebugInfo &dbgInfo);
static bool RecursiveProcessNode (TIntermNode* node, DebugInfo &dbgInfo);
static bool RecursiveProcessBranchNode (TIntermBranch* node, DebugInfo &dbgInfo);
static bool RecursiveProcessSwitchNode (TIntermSwitch* node, DebugInfo &dbgInfo);
static bool RecursiveProcessConstUnionNode (TIntermConstantUnion* node, DebugInfo &dbgInfo);
static bool RecursiveProcessSelectionNode (TIntermSelection* node, DebugInfo &dbgInfo);
static bool RecursiveProcessMethodNode (TIntermMethod* node, DebugInfo &dbgInfo);
static bool RecursiveProcessSymbolNode (TIntermSymbol* node, DebugInfo &dbgInfo);
static bool RecursiveProcessTypedNode (TIntermTyped* node, DebugInfo &dbgInfo);
static bool RecursiveProcessOperatorNode (TIntermOperator* node, DebugInfo &dbgInfo);
static bool RecursiveProcessUnaryNode (TIntermUnary* node, DebugInfo &dbgInfo);
static bool RecursiveProcessBinaryNode (TIntermBinary* node, DebugInfo &dbgInfo);
static bool RecursiveProccessLoop (TIntermLoop* node, DebugInfo &dbgInfo);
static bool CreateDebugDumpFunctions (TIntermNode* root, DebugInfo &dbgInfo);

/*
=================================================
	_GenerateDebugInfo
=================================================
*/
bool  ShaderCompiler::_GenerateDebugInfo (const TIntermediate &intermediate, OUT ShaderDebugInfo &shaderInfo)
{
	TIntermNode*	root = intermediate.getTreeRoot();
	CHECK_ERR( root );

	DebugInfo	dbg_info;

	dbg_info.Enter( root );
	CHECK_ERR( RecursiveProcessNode( root, dbg_info ));

	CHECK( not dbg_info.GetInjection() );

	CHECK_ERR( CreateDebugDumpFunctions( root, dbg_info ));
	dbg_info.Leave( root );

	dbg_info.GetSourceLocationsMap( OUT shaderInfo.srcLocations );
	dbg_info.GetDebugStorageFieldsOffset( OUT shaderInfo.posOffset, OUT shaderInfo.dataOffset );
	return true;
}

/*
=================================================
	RecursiveProcessNode
=================================================
*/
static bool RecursiveProcessNode (TIntermNode* node, DebugInfo &dbgInfo)
{
	if ( not node )
		return true;

	if ( auto* aggr = node->getAsAggregate() )
	{
		CHECK_ERR( RecursiveProcessAggregateNode( aggr, dbgInfo ));
		return true;
	}

	if ( auto* unary = node->getAsUnaryNode() )
	{
		CHECK_ERR( RecursiveProcessUnaryNode( unary, dbgInfo ));
		return true;
	}

	if ( auto* binary = node->getAsBinaryNode() )
	{
		CHECK_ERR( RecursiveProcessBinaryNode( binary, dbgInfo ));
		return true;
	}

	if ( auto* op = node->getAsOperator() )
	{
		//CHECK_ERR( RecursiveProcessOperatorNode( op, dbgInfo ));
		return false;
	}

	if ( auto* branch = node->getAsBranchNode() )
	{
		CHECK_ERR( RecursiveProcessBranchNode( branch, dbgInfo ));
		return true;
	}

	if ( auto* sw = node->getAsSwitchNode() )
	{
		CHECK_ERR( RecursiveProcessSwitchNode( sw, dbgInfo ));
		return true;
	}

	if ( auto* cunion = node->getAsConstantUnion() )
	{
		dbgInfo.AddLocation( node->getLoc() );
		//CHECK_ERR( RecursiveProcessConstUnionNode( cunion, dbgInfo ));
		return true;
	}

	if ( auto* selection = node->getAsSelectionNode() )
	{
		CHECK_ERR( RecursiveProcessSelectionNode( selection, dbgInfo ));
		return true;
	}

	if ( auto* method = node->getAsMethodNode() )
	{
		//CHECK_ERR( RecursiveProcessMethodNode( method, dbgInfo ));
		return true;
	}

	if ( auto* symbol = node->getAsSymbolNode() )
	{
		dbgInfo.AddLocation( node->getLoc() );
		CHECK_ERR( RecursiveProcessSymbolNode( symbol, dbgInfo ));
		return true;
	}

	if ( auto* typed = node->getAsTyped() )
	{
		dbgInfo.AddLocation( node->getLoc() );
		//CHECK_ERR( RecursiveProcessTypedNode( typed, dbgInfo ));
		return true;
	}

	if ( auto* loop = node->getAsLoopNode() )
	{
		CHECK_ERR( RecursiveProccessLoop( loop, dbgInfo ));
		return true;
	}

	return false;
}

/*
=================================================
	CreateDebugDumpBody
=================================================
*/
ND_ static TIntermAggregate*  CreateDebugDumpBody (const TString &fnName, DebugInfo &dbgInfo)
{
	TPublicType		value_type;
	TPublicType		uint_type;

	value_type.init( Default );
	uint_type.init( Default );

	uint_type.basicType			= TBasicType::EbtUint;
	uint_type.qualifier.storage = TStorageQualifier::EvqConstReadOnly;

	// extract type
	{
		size_t	pos  = fnName.find( '(' );
		size_t	pos1 = fnName.find( ';' );
		CHECK_ERR( pos != TString::npos and pos1 != TString::npos and pos < pos1 );

		bool	is_vector	= (fnName[++pos] == 'v');

		if ( is_vector )	++pos;

		switch ( fnName[pos] )
		{
			case 'f' :	value_type.basicType = TBasicType::EbtFloat;	break;
			case 'i' :	value_type.basicType = TBasicType::EbtInt;		break;
			case 'u' :	value_type.basicType = TBasicType::EbtUint;		break;
			default  :	RETURN_ERR( "unknown type" );
		}
		++pos;

		if ( is_vector ) {
			value_type.vectorSize = fnName[pos] - '0';
			CHECK_ERR( value_type.vectorSize > 0 and value_type.vectorSize <= 4 );
		}
		else {
			// scalar
			value_type.vectorSize = 1;
		}

		// TODO: matrix
	}

	const uint			dbg_data_size = value_type.vectorSize + 2;	// location, size, value

	TIntermAggregate*	fn_node		= new TIntermAggregate{ TOperator::EOpFunction };
	TIntermAggregate*	fn_args		= new TIntermAggregate{ TOperator::EOpParameters };
	TIntermAggregate*	fn_body		= new TIntermAggregate{ TOperator::EOpSequence };
	TIntermAggregate*	branch_body = new TIntermAggregate{ TOperator::EOpSequence };

	// build function body
	{
		value_type.qualifier.storage = TStorageQualifier::EvqGlobal;
		fn_node->setType( TType{value_type} );
		fn_node->setName( fnName );
		fn_node->getSequence().push_back( fn_args );
		fn_node->getSequence().push_back( fn_body );
	}
	
	// build function argument sequence
	{
		value_type.qualifier.storage = TStorageQualifier::EvqConstReadOnly;
		TIntermSymbol*		arg0	 = new TIntermSymbol{ dbgInfo.GetUniqueSymbolID(), "value", TType{value_type} };
		TIntermSymbol*		arg1	 = new TIntermSymbol{ dbgInfo.GetUniqueSymbolID(), "sourceLocation", TType{uint_type} };

		fn_args->setType( TType{EbtVoid} );
		fn_args->getSequence().push_back( arg0 );
		fn_args->getSequence().push_back( arg1 );
	}

	// build function body
	{
		value_type.qualifier.storage = TStorageQualifier::EvqTemporary;
		fn_body->setType( TType{value_type} );
	}

	// "pos" variable
	uint_type.qualifier.storage = TStorageQualifier::EvqTemporary;
	TIntermSymbol*	var_pos		= new TIntermSymbol{ dbgInfo.GetUniqueSymbolID(), "pos", TType{uint_type} };
	
	// "uint pos = atomicAdd( dbgStorage.position, x );"
	//   or
	// "uint pos = dbgStorage.position;"	"dbgStorage.position += x;"
	{
		TIntermAggregate*	move_pos	= new TIntermAggregate{ TOperator::EOpSequence };
		TIntermBinary*		assign_op	= new TIntermBinary{ TOperator::EOpAssign };			// pos = ...
		
		branch_body->setType( TType{EbtVoid} );
		branch_body->getSequence().push_back( move_pos );

		move_pos->setType( TType{EbtVoid} );
		move_pos->getSequence().push_back( assign_op );
		
		uint_type.qualifier.storage = TStorageQualifier::EvqConst;
		TConstUnionArray		data_size_value(1);	data_size_value[0].setUConst( dbg_data_size );
		TIntermConstantUnion*	data_size	= new TIntermConstantUnion{ data_size_value, TType{uint_type} };
		
	#if 1
		TIntermAggregate*		add_op	= new TIntermAggregate{ TOperator::EOpAtomicAdd };		// atomicAdd
		uint_type.qualifier.storage = TStorageQualifier::EvqGlobal;
		add_op->setType( TType{uint_type} );
		add_op->setOperationPrecision( TPrecisionQualifier::EpqHigh );
		add_op->getQualifierList().push_back( TStorageQualifier::EvqInOut );
		add_op->getQualifierList().push_back( TStorageQualifier::EvqIn );
		add_op->getSequence().push_back( dbgInfo.GetDebugStoragePositionField() );
		add_op->getSequence().push_back( data_size );
	#endif

		assign_op->setType( TType{uint_type} );
		assign_op->setLeft( var_pos );
		assign_op->setRight( add_op );
	}

	// "dbgStorage.outData[pos++] = ..."
	{
		uint_type.qualifier.storage = TStorageQualifier::EvqTemporary;

		TIntermBinary*			assign_data1	= new TIntermBinary{ TOperator::EOpAssign };
		TIntermBinary*			assign_data2	= new TIntermBinary{ TOperator::EOpAssign };
		TIntermBinary*			indexed_access	= new TIntermBinary{ TOperator::EOpIndexIndirect };
		TIntermUnary*			inc_pos			= new TIntermUnary{ TOperator::EOpPostIncrement };
		TConstUnionArray		type_value(1);	type_value[0].setUConst( (uint(value_type.basicType) & 0xFF) | (value_type.vectorSize << 8) );
		TIntermConstantUnion*	type_id			= new TIntermConstantUnion{ type_value, TType{uint_type} };

		inc_pos->setType( TType{uint_type} );
		inc_pos->setOperand( var_pos );

		indexed_access->setType( TType{uint_type} );
		indexed_access->getWritableType().setFieldName( "outData" );
		indexed_access->setLeft( dbgInfo.GetDebugStorageDataField() );
		indexed_access->setRight( inc_pos );

		assign_data1->setType( TType{uint_type} );
		assign_data1->setLeft( indexed_access );
		assign_data1->setRight( fn_args->getSequence()[1]->getAsTyped() );
		branch_body->getSequence().push_back( assign_data1 );
		
		assign_data2->setType( TType{uint_type} );
		assign_data2->setLeft( indexed_access );
		assign_data2->setRight( type_id );
		branch_body->getSequence().push_back( assign_data2 );

		if ( value_type.isScalar() )
		{
			TIntermBinary*	assign_data3	= new TIntermBinary{ TOperator::EOpAssign };
			TIntermTyped*	scalar			= fn_args->getSequence()[0]->getAsTyped();
			
			assign_data3->setType( TType{uint_type} );
			assign_data3->setLeft( indexed_access );
			branch_body->getSequence().push_back( assign_data3 );
			
			switch ( scalar->getType().getBasicType() )
			{
				case TBasicType::EbtFloat :
				{
					TIntermUnary*	as_uint = new TIntermUnary{ TOperator::EOpFloatBitsToUint };

					uint_type.qualifier.storage = TStorageQualifier::EvqGlobal;
					as_uint->setType( TType{uint_type} );
					as_uint->setOperand( scalar );
					as_uint->setOperationPrecision( TPrecisionQualifier::EpqHigh );
					uint_type.qualifier.storage = TStorageQualifier::EvqTemporary;
					
					assign_data3->setRight( as_uint );
					break;
				}

				case TBasicType::EbtInt :
				case TBasicType::EbtUint :
					assign_data3->setRight( scalar );
					break;

				default :
					RETURN_ERR( "not supported" );
			}
		}
		else
		for (int i = 0; i < value_type.vectorSize; ++i)
		{
			TPublicType		pub_type;
			pub_type.init( Default );
			pub_type.basicType			= TBasicType::EbtInt;
			pub_type.qualifier.storage	= TStorageQualifier::EvqConst;

			TIntermBinary*			assign_data3	= new TIntermBinary{ TOperator::EOpAssign };
			TConstUnionArray		field_index(1);	field_index[0].setIConst( i );
			TIntermConstantUnion*	vec_field		= new TIntermConstantUnion{ field_index, TType{pub_type} };
			TIntermBinary*			field_access	= new TIntermBinary{ TOperator::EOpIndexDirect };
			TIntermTyped*			vec				= fn_args->getSequence()[0]->getAsTyped();

			pub_type.basicType			= vec->getType().getBasicType();
			pub_type.qualifier.storage	= vec->getType().getQualifier().storage;

			field_access->setType( TType{pub_type} );
			field_access->setLeft( vec );
			field_access->setRight( vec_field );
			
			assign_data3->setType( TType{uint_type} );
			assign_data3->setLeft( indexed_access );
			branch_body->getSequence().push_back( assign_data3 );

			switch ( pub_type.basicType )
			{
				case TBasicType::EbtFloat :
				{
					TIntermUnary*	as_uint = new TIntermUnary{ TOperator::EOpFloatBitsToUint };

					uint_type.qualifier.storage = TStorageQualifier::EvqGlobal;
					as_uint->setType( TType{uint_type} );
					as_uint->setOperand( field_access );
					as_uint->setOperationPrecision( TPrecisionQualifier::EpqHigh );
					uint_type.qualifier.storage = TStorageQualifier::EvqTemporary;
					
					assign_data3->setRight( as_uint );
					break;
				}

				case TBasicType::EbtInt :
				case TBasicType::EbtUint :
					assign_data3->setRight( field_access );
					break;

				default :
					RETURN_ERR( "not supported" );
			}
		}
	}

	// "if ( IsCurrentShader() )"
	{
		TPublicType			bool_type;
		bool_type.init( Default );
		bool_type.basicType			= TBasicType::EbtBool;
		bool_type.qualifier.storage	= TStorageQualifier::EvqGlobal;

		TIntermAggregate*	condition	= new TIntermAggregate{ TOperator::EOpFunctionCall };
		condition->setType( TType{bool_type} );
		condition->setName( "IsCurrentShader(" );
		condition->setUserDefined();

		TIntermSelection*	selection	= new TIntermSelection{ condition, branch_body, null };
		selection->setType( TType{EbtVoid} );

		fn_body->getSequence().push_back( selection );
	}

	// "return value"
	{
		TIntermBranch*		fn_return	= new TIntermBranch{ TOperator::EOpReturn, fn_args->getSequence()[0]->getAsTyped() };
		fn_body->getSequence().push_back( fn_return );
	}

	return fn_node;
}

/*
=================================================
	CreateIsCurrentShaderBody
=================================================
*/
ND_ static TIntermAggregate*  CreateIsCurrentShaderBody (const TString &fnName, DebugInfo &dbgInfo)
{
	TIntermAggregate*	fn_node		= new TIntermAggregate{ TOperator::EOpFunction };
	TIntermAggregate*	fn_args		= new TIntermAggregate{ TOperator::EOpParameters };
	TIntermAggregate*	fn_body		= new TIntermAggregate{ TOperator::EOpSequence };
	
	TPublicType			bool_type;
	TPublicType			uint_type;
	TPublicType			index_type;
	TPublicType			float_type;

	bool_type.init( Default );
	bool_type.basicType			= TBasicType::EbtBool;
	bool_type.qualifier.storage	= TStorageQualifier::EvqGlobal;

	uint_type.init( Default );
	uint_type.basicType			= TBasicType::EbtUint;
	uint_type.qualifier.storage	= TStorageQualifier::EvqTemporary;
	
	float_type.init( Default );
	float_type.basicType		 = TBasicType::EbtFloat;
	float_type.qualifier.storage = TStorageQualifier::EvqTemporary;
	
	index_type.init( Default );
	index_type.basicType		 = TBasicType::EbtInt;
	index_type.qualifier.storage = TStorageQualifier::EvqConst;

	// build function body
	{
		fn_node->setType( TType{bool_type} );
		fn_node->setName( fnName );
		fn_node->setOptimize( true );
		fn_node->getSequence().push_back( fn_args );
		fn_node->getSequence().push_back( fn_body );
		
		fn_args->setType( TType{EbtVoid} );
		fn_body->setType( TType{EbtVoid} );
	}

	// dbgStorage.fs_Coord
	TIntermSymbol*			dbg_storage		= dbgInfo.GetDebugStorage();
	TIntermConstantUnion*	fscoord_field	= null;
	TIntermBinary*			fscoord_access	= new TIntermBinary{ TOperator::EOpIndexDirectStruct };
	TType const*			type_ptr		= null;

	for (auto& field : *dbg_storage->getType().getStruct())
	{
		if ( field.type->getFieldName() == "fs_Coord" )
		{
			const size_t		index = Distance( dbg_storage->getType().getStruct()->data(), &field );
			TConstUnionArray	index_Value(1);		index_Value[0].setIConst( int(index) );
			fscoord_field	= new TIntermConstantUnion{ index_Value, TType{index_type} };
			type_ptr		= field.type;
			break;
		}
	}
	CHECK_ERR( fscoord_field and type_ptr );
	fscoord_access->setType( *type_ptr );
	fscoord_access->setLeft( dbg_storage );
	fscoord_access->setRight( fscoord_field );

	// dbgStorage.fs_Coord.x
	TConstUnionArray		x_index(1);	x_index[0].setIConst( 0 );
	TIntermConstantUnion*	x_field		= new TIntermConstantUnion{ x_index, TType{index_type} };
	TIntermBinary*			fscoord_x	= new TIntermBinary{ TOperator::EOpIndexDirect };
	fscoord_x->setType( TType{uint_type} );
	fscoord_x->setLeft( fscoord_access );
	fscoord_x->setRight( x_field );

	// gl_FragCoord.x
	TIntermBinary*			frag_x		= new TIntermBinary{ TOperator::EOpIndexDirect };
	frag_x->setType( TType{float_type} );
	frag_x->setLeft( dbgInfo.GetCachedSymbolNode( "gl_FragCoord" ));
	frag_x->setRight( x_field );

	// uint(gl_FragCoord.x)
	TIntermUnary*			uint_fc_x	= new TIntermUnary{ TOperator::EOpConvFloatToUint };
	uint_fc_x->setType( TType{uint_type} );
	uint_fc_x->setOperand( frag_x );

	// ... == ...
	TIntermBinary*			eq1			= new TIntermBinary{ TOperator::EOpEqual };
	bool_type.qualifier.storage	= TStorageQualifier::EvqTemporary;
	eq1->setType( TType{bool_type} );
	eq1->setLeft( uint_fc_x );
	eq1->setRight( fscoord_x );

	
	// dbgStorage.fs_Coord.y
	TConstUnionArray		y_index(1);	y_index[0].setIConst( 1 );
	TIntermConstantUnion*	y_field		= new TIntermConstantUnion{ y_index, TType{index_type} };
	TIntermBinary*			fscoord_y	= new TIntermBinary{ TOperator::EOpIndexDirect };
	fscoord_y->setType( TType{uint_type} );
	fscoord_y->setLeft( fscoord_access );
	fscoord_y->setRight( y_field );

	// gl_FragCoord.y
	TIntermBinary*			frag_y		= new TIntermBinary{ TOperator::EOpIndexDirect };
	frag_y->setType( TType{float_type} );
	frag_y->setLeft( dbgInfo.GetCachedSymbolNode( "gl_FragCoord" ));
	frag_y->setRight( y_field );

	// uint(gl_FragCoord.y)
	TIntermUnary*			uint_fc_y	= new TIntermUnary{ TOperator::EOpConvFloatToUint };
	uint_fc_y->setType( TType{uint_type} );
	uint_fc_y->setOperand( frag_y );
	
	// ... == ...
	TIntermBinary*			eq2			= new TIntermBinary{ TOperator::EOpEqual };
	eq2->setType( TType{bool_type} );
	eq2->setLeft( uint_fc_y );
	eq2->setRight( fscoord_y );


	// ... && ...
	TIntermBinary*			cmp1		= new TIntermBinary{ TOperator::EOpLogicalAnd };
	bool_type.qualifier.storage	= TStorageQualifier::EvqTemporary;
	cmp1->setType( TType{bool_type} );
	cmp1->setLeft( eq1 );
	cmp1->setRight( eq2 );

	// return ...
	TIntermBranch*			fn_return	= new TIntermBranch{ TOperator::EOpReturn, cmp1 };
	fn_body->getSequence().push_back( fn_return );
	
	return fn_node;
}

/*
=================================================
	CreateDebugDumpFunctions
=================================================
*/
static bool CreateDebugDumpFunctions (TIntermNode* root, DebugInfo &dbgInfo)
{
	TIntermAggregate*	aggr = root->getAsAggregate();
	CHECK_ERR( aggr );

	for (auto& fn : dbgInfo.GetRequiredFunctions())
	{
		if ( StartsWith( fn, "DebugDump(" ) )
		{
			auto*	body = CreateDebugDumpBody( fn, dbgInfo );
			CHECK_ERR( body );

			aggr->getSequence().push_back( body );
		}
		else
		if ( fn == "IsCurrentShader(" )
		{
			auto*	body = CreateIsCurrentShaderBody( fn, dbgInfo );
			CHECK_ERR( body );

			aggr->getSequence().push_back( body );
		}
	}
	return true;
}

/*
=================================================
	CreateDebugDump
=================================================
*/
ND_ static TIntermAggregate*  CreateDebugDump (TIntermTyped* typed, DebugInfo &dbgInfo)
{
	TIntermAggregate*	fcall		= new TIntermAggregate( TOperator::EOpFunctionCall );
	TString				type_name;
	const TType &		type		= typed->getType();

	ENABLE_ENUM_CHECKS();
	switch ( type.getBasicType() )
	{
		case TBasicType::EbtFloat :		type_name = "f";	break;
		case TBasicType::EbtInt :		type_name = "i";	break;
		case TBasicType::EbtUint :		type_name = "u";	break;

		case TBasicType::EbtVoid :
		case TBasicType::EbtDouble :
		case TBasicType::EbtFloat16 :
		case TBasicType::EbtInt8 :
		case TBasicType::EbtUint8 :
		case TBasicType::EbtInt16 :
		case TBasicType::EbtUint16 :
		case TBasicType::EbtInt64 :
		case TBasicType::EbtUint64 :
		case TBasicType::EbtBool :
		case TBasicType::EbtAtomicUint :
		case TBasicType::EbtSampler :
		case TBasicType::EbtStruct :
		case TBasicType::EbtBlock :
		#ifdef NV_EXTENSIONS
		case TBasicType::EbtAccStructNV :
		#endif
		case TBasicType::EbtString :
		case TBasicType::EbtNumTypes :		RETURN_ERR( "not supported" );
	}
	DISABLE_ENUM_CHECKS();
	
	/*if ( type.isMatrix() )
		{}
	else*/
	if ( type.isVector() )
		type_name = TString("v") + type_name + TString(ToString(type.getVectorSize()).data());
	else
	if ( type.isScalarOrVec1() )
		type_name += "1";
	else
		RETURN_ERR( "not supported" );


	fcall->setLoc( typed->getLoc() );
	fcall->setUserDefined();
	fcall->setOptimize(true);
	fcall->setName( "DebugDump(" + type_name + ";u1;" );
	fcall->setType( typed->getType() );
	fcall->getQualifierList().push_back( TStorageQualifier::EvqConstReadOnly );
	fcall->getQualifierList().push_back( TStorageQualifier::EvqConstReadOnly );
	fcall->getSequence().push_back( typed );
	
	TPublicType			cu_pub_type;	cu_pub_type.init( typed->getLoc() );
	cu_pub_type.basicType				= TBasicType::EbtUint;
	cu_pub_type.vectorSize				= 1;
	cu_pub_type.qualifier.storage		= TStorageQualifier::EvqConst;

	TConstUnionArray		arr(1);		arr[0].setUConst( dbgInfo.GetSourceLocation() );
	TIntermConstantUnion*	c_union		= new TIntermConstantUnion{ arr, TType{cu_pub_type} };
	
	c_union->setLoc( typed->getLoc() );
	fcall->getSequence().push_back( c_union );

	dbgInfo.RequestFunc( fcall->getName() );

	return fcall;
}

/*
=================================================
	RecursiveProcessAggregateNode
=================================================
*/
static bool RecursiveProcessAggregateNode (TIntermAggregate* aggr, DebugInfo &dbgInfo)
{
	dbgInfo.Enter( aggr );

	for (auto& node : aggr->getSequence())
	{
		CHECK_ERR( RecursiveProcessNode( node, dbgInfo ));
		
		if ( auto* inj = dbgInfo.GetInjection() )
			node = inj;
	}
		
	const auto	loc = dbgInfo.GetCurrentLocation();
	dbgInfo.Leave( aggr );

	if ( aggr->getOp() >= TOperator::EOpNegative )
	{
		// propagate source location to root
		dbgInfo.AddLocation( loc );
	}
	return true;
}

/*
=================================================
	RecursiveProcessUnaryNode
=================================================
*/
static bool RecursiveProcessUnaryNode (TIntermUnary* unary, DebugInfo &dbgInfo)
{
	dbgInfo.Enter( unary );

	CHECK_ERR( RecursiveProcessNode( unary->getOperand(), dbgInfo ));
	
	if ( auto* inj = dbgInfo.GetInjection() )
		unary->setOperand( inj );
	
	const auto	loc = dbgInfo.GetCurrentLocation();
	dbgInfo.Leave( unary );

	// propagate source location to root
	dbgInfo.AddLocation( loc );
	return true;
}

/*
=================================================
	RecursiveProcessBinaryNode
=================================================
*/
static bool RecursiveProcessBinaryNode (TIntermBinary* binary, DebugInfo &dbgInfo)
{
	dbgInfo.Enter( binary );

	CHECK_ERR( RecursiveProcessNode( binary->getLeft(), dbgInfo ));

	if ( auto* inj = dbgInfo.GetInjection() )
		binary->setLeft( inj );

	CHECK_ERR( RecursiveProcessNode( binary->getRight(), dbgInfo ));

	if ( auto* inj = dbgInfo.GetInjection() )
		binary->setRight( inj );

	switch ( binary->getOp() )
	{
		case TOperator::EOpAssign :
		case TOperator::EOpAddAssign :
		case TOperator::EOpSubAssign :
		case TOperator::EOpMulAssign :
		case TOperator::EOpDivAssign :
		case TOperator::EOpModAssign :
		case TOperator::EOpAndAssign :
		case TOperator::EOpInclusiveOrAssign :
		case TOperator::EOpExclusiveOrAssign :
		case TOperator::EOpLeftShiftAssign :
		case TOperator::EOpRightShiftAssign :
		{
			dbgInfo.InjectNode( CreateDebugDump( binary, dbgInfo ));
			break;
		}
	}

	const auto	loc = dbgInfo.GetCurrentLocation();
	dbgInfo.Leave( binary );

	// propagate source location to root
	dbgInfo.AddLocation( loc );
	return true;
}

/*
=================================================
	RecursiveProcessBranchNode
=================================================
*/
static bool RecursiveProcessBranchNode (TIntermBranch* branch, DebugInfo &dbgInfo)
{
	dbgInfo.Enter( branch );

	CHECK_ERR( RecursiveProcessNode( branch->getExpression(), dbgInfo ));
	
	if ( auto* inj = dbgInfo.GetInjection() )
		PlacementNew<TIntermBranch>( branch, branch->getFlowOp(), inj );

	dbgInfo.Leave( branch );
	return true;
}

/*
=================================================
	ReplaceIntermSwitch
=================================================
*/
static void ReplaceIntermSwitch (INOUT TIntermSwitch* sw, TIntermTyped* cond, TIntermAggregate* b)
{
	bool		is_flatten		= sw->getFlatten();
	bool		dont_flatten	= sw->getDontFlatten();
	TSourceLoc	loc				= sw->getLoc();

	PlacementNew<TIntermSwitch>( sw, cond, b );
	
	sw->setLoc( loc );

	if ( is_flatten )	sw->setFlatten();
	if ( dont_flatten )	sw->setDontFlatten();
}

/*
=================================================
	RecursiveProcessSwitchNode
=================================================
*/
static bool RecursiveProcessSwitchNode (TIntermSwitch* sw, DebugInfo &dbgInfo)
{
	dbgInfo.Enter( sw );

	CHECK_ERR( RecursiveProcessNode( sw->getCondition(), dbgInfo ));
	
	if ( auto* inj = dbgInfo.GetInjection() )
		ReplaceIntermSwitch( sw, inj, sw->getBody() );

	CHECK_ERR( RecursiveProcessNode( sw->getBody(), dbgInfo ));
	
	if ( auto* inj = dbgInfo.GetInjection() )
		ReplaceIntermSwitch( sw, sw->getCondition()->getAsTyped(), inj->getAsAggregate() );

	dbgInfo.Leave( sw );
	return true;
}

/*
=================================================
	ReplaceIntermSelection
=================================================
*/
static void ReplaceIntermSelection (INOUT TIntermSelection *selection, TIntermTyped* cond, TIntermNode* trueB, TIntermNode* falseB)
{
	bool		is_flatten			= selection->getFlatten();
	bool		dont_flatten		= selection->getDontFlatten();
	bool		is_short_circuit	= selection->getShortCircuit();
	TSourceLoc	loc					= selection->getLoc();
	TType		type;				type.shallowCopy( selection->getType() );

	PlacementNew<TIntermSelection>( selection, cond, trueB, falseB, type );

	selection->setLoc( loc );
	
	if ( is_flatten )			selection->setFlatten();
	if ( dont_flatten )			selection->setDontFlatten();
	if ( not is_short_circuit )	selection->setNoShortCircuit();
}

/*
=================================================
	RecursiveProcessSelectionNode
=================================================
*/
static bool RecursiveProcessSelectionNode (TIntermSelection* selection, DebugInfo &dbgInfo)
{
	dbgInfo.Enter( selection );

	CHECK_ERR( RecursiveProcessNode( selection->getCondition(), dbgInfo ));
	
	if ( auto* inj = dbgInfo.GetInjection() )
		ReplaceIntermSelection( INOUT selection, inj, selection->getTrueBlock(), selection->getFalseBlock() );

	CHECK_ERR( RecursiveProcessNode( selection->getTrueBlock(), dbgInfo ));
	
	if ( auto* inj = dbgInfo.GetInjection() )
		ReplaceIntermSelection( INOUT selection, selection->getCondition(), inj, selection->getFalseBlock() );

	CHECK_ERR( RecursiveProcessNode( selection->getFalseBlock(), dbgInfo ));
	
	if ( auto* inj = dbgInfo.GetInjection() )
		ReplaceIntermSelection( INOUT selection, selection->getCondition(), selection->getTrueBlock(), inj );
		
	dbgInfo.Leave( selection );
	return true;
}

/*
=================================================
	ReplaceIntermLoop
=================================================
*/
static void ReplaceIntermLoop (INOUT TIntermLoop* loop, TIntermNode* aBody, TIntermTyped* aTest, TIntermTyped* aTerminal)
{
	bool		test_first	= loop->testFirst();
	bool		is_unroll	= loop->getUnroll();
	bool		dont_unroll	= loop->getDontUnroll();
	int			dependency	= loop->getLoopDependency();
	TSourceLoc	loc			= loop->getLoc();

	PlacementNew<TIntermLoop>( loop, aBody, aTest, aTerminal, test_first );

	if ( is_unroll )		loop->setUnroll();
	if ( dont_unroll )		loop->setDontUnroll();

	loop->setLoopDependency( dependency );
	loop->setLoc( loc );
}

/*
=================================================
	RecursiveProccessLoop
=================================================
*/
static bool RecursiveProccessLoop (TIntermLoop* loop, DebugInfo &dbgInfo)
{
	dbgInfo.Enter( loop );

	CHECK_ERR( RecursiveProcessNode( loop->getBody(), dbgInfo ));
	
	if ( auto* inj = dbgInfo.GetInjection() )
		ReplaceIntermLoop( INOUT loop, inj, loop->getTest(), loop->getTerminal() );

	if ( loop->getTerminal() )
	{
		CHECK_ERR( RecursiveProcessNode( loop->getTerminal(), dbgInfo ));
		
		if ( auto* inj = dbgInfo.GetInjection() )
			ReplaceIntermLoop( INOUT loop, loop->getBody(), loop->getTest(), inj );
	}

	if ( loop->getTest() )
	{
		CHECK_ERR( RecursiveProcessNode( loop->getTest(), dbgInfo ));
		
		if ( auto* inj = dbgInfo.GetInjection() )
			ReplaceIntermLoop( INOUT loop, loop->getBody(), inj, loop->getTerminal() );
	}
	
	dbgInfo.Leave( loop );
	return true;
}

/*
=================================================
	RecursiveProcessSymbolNode
=================================================
*/
static bool RecursiveProcessSymbolNode (TIntermSymbol* node, DebugInfo &dbgInfo)
{
	dbgInfo.AddSymbolID( node->getId() );

	if ( node->getName() == "dbgStorage" and node->getType().getStruct() )
	{
		dbgInfo.SetDebugStorage( node );
		return true;
	}

	if ( node->getName() == "gl_FragCoord" )
	{
		dbgInfo.CacheSymbolNode( node );
		return true;
	}

	// do nothing
	return true;
}
