// Copyright (c) 2018-2019,  Zhirnov Andrey. For more information see 'LICENSE'

#include "DebugInfo.h"


static bool RecursiveProcessNode (TIntermNode* node, DebugInfo &dbgInfo);
static void CreateShaderDebugStorage (uint32_t setIndex, DebugInfo &dbgInfo, OUT uint64_t &posOffset, OUT uint64_t &dataOffset);
static void CreateShaderBuiltinSymbols (TIntermNode* root, DebugInfo &dbgInfo);
static bool CreateDebugTraceFunctions (TIntermNode* root, DebugInfo &dbgInfo);
static bool InsertDebugStorageBuffer (TIntermNode* root, DebugInfo &dbgInfo);
static TIntermNode*  DumpShaderInfo (const TSourceLoc &loc, DebugInfo &dbgInfo);

/*
=================================================
	GenerateDebugInfo
=================================================
*/
bool ShaderTrace::GenerateDebugInfo (TIntermediate &intermediate, uint32_t setIndex)
{
	DebugInfo		dbg_info{ intermediate.getStage(), OUT _exprLocations };

	TIntermNode*	root = intermediate.getTreeRoot();
	CHECK_ERR( root );
	
	CreateShaderDebugStorage( setIndex, dbg_info, OUT _posOffset, OUT _dataOffset );

	dbg_info.Enter( root );
	{
		CHECK_ERR( RecursiveProcessNode( root, dbg_info ));
		CHECK( not dbg_info.GetInjection() );
	
		CHECK_ERR( InsertDebugStorageBuffer( root, dbg_info ));

		CreateShaderBuiltinSymbols( root, dbg_info );

		CHECK_ERR( CreateDebugTraceFunctions( root, dbg_info ));
	}
	dbg_info.Leave( root );

	dbg_info.PostProcess( OUT _varNames );

	_shaderType = uint32_t(intermediate.getStage());
	return true;
}

/*
=================================================
	SetSource
=================================================
*/
void ShaderTrace::SetSource (const char* const* sources, const size_t *lengths, size_t count)
{
	_sources.clear();

	for (size_t i = 0; i < count; ++i)
	{
		SourceInfo	info;
		size_t		pos = 0;

		info.code.assign( sources[i], (lengths ? lengths[i] : strlen(sources[i])) );
		info.lines.reserve( 64 );

		for (size_t j = 0, len = info.code.length(); j < len; ++j)
		{
			const char	c = info.code[j];
			const char	n = (j+1) >= len ? 0 : info.code[j+1];

			// windows style "\r\n"
			if ( c == '\r' and n == '\n' )
			{
				info.lines.push_back({ pos, j });
				pos = (++j) + 1;
			}
			else
			// linux style "\n" (or mac style "\r")
			if ( c == '\n' or c == '\r' )
			{
				info.lines.push_back({ pos, j });
				pos = j + 1;
			}
		}

		if ( pos < info.code.length() )
			info.lines.push_back({ pos, info.code.length() });

		_sources.push_back( std::move(info) );
	}
}

void ShaderTrace::SetSource (const char* source, size_t length)
{
	const char*	sources[] = { source };
	return SetSource( sources, &length, 1 );
}

/*
=================================================
	GetSource
=================================================
*/
void ShaderTrace::GetSource (OUT std::string &result) const
{
	size_t	total_size = _sources.size()*2;

	for (auto& src : _sources) {
		total_size += src.code.length();
	}

	result.clear();
	result.reserve( total_size );

	for (auto& src : _sources) {
		result.append( src.code );
	}
}
//-----------------------------------------------------------------------------


static bool RecursiveProcessAggregateNode (TIntermAggregate* node, DebugInfo &dbgInfo);
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
		return true;
	}

	if ( auto* selection = node->getAsSelectionNode() )
	{
		CHECK_ERR( RecursiveProcessSelectionNode( selection, dbgInfo ));
		return true;
	}

	if ( auto* method = node->getAsMethodNode() )
	{
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
	CreateFragmentShaderDebugStorage
=================================================
*/
static void CreateFragmentShaderDebugStorage (TTypeList* typeList, INOUT TPublicType &type)
{
	type.basicType = TBasicType::EbtUint;

	TType*	fragcoord_x	= new TType{type};		fragcoord_x->setFieldName( "fragCoordX" );
	type.qualifier.layoutOffset += sizeof(uint32_t);

	TType*	fragcoord_y	= new TType{type};		fragcoord_y->setFieldName( "fragCoordY" );
	type.qualifier.layoutOffset += sizeof(uint32_t);

	TType*	sample_id	= new TType{type};		sample_id->setFieldName( "sampleID" );
	type.qualifier.layoutOffset += sizeof(uint32_t);

	TType*	primitive_id = new TType{type};		primitive_id->setFieldName( "primitiveID" );
	type.qualifier.layoutOffset += sizeof(uint32_t);
	
	typeList->push_back({ fragcoord_x,	TSourceLoc{} });
	typeList->push_back({ fragcoord_y,	TSourceLoc{} });
	typeList->push_back({ sample_id,	TSourceLoc{} });
	typeList->push_back({ primitive_id,	TSourceLoc{} });
}

/*
=================================================
	CreateComputeShaderDebugStorage
=================================================
*/
static void CreateComputeShaderDebugStorage (TTypeList* typeList, INOUT TPublicType &type)
{
	type.basicType = TBasicType::EbtUint;
	
	TType*	thread_id_x	= new TType{type};		thread_id_x->setFieldName( "globalInvocationX" );
	type.qualifier.layoutOffset += sizeof(uint32_t);
	
	TType*	thread_id_y	= new TType{type};		thread_id_y->setFieldName( "globalInvocationY" );
	type.qualifier.layoutOffset += sizeof(uint32_t);
	
	TType*	thread_id_z	= new TType{type};		thread_id_z->setFieldName( "globalInvocationZ" );
	type.qualifier.layoutOffset += sizeof(uint32_t);
	
	typeList->push_back({ thread_id_x,	TSourceLoc{} });
	typeList->push_back({ thread_id_y,	TSourceLoc{} });
	typeList->push_back({ thread_id_z,	TSourceLoc{} });
}

/*
=================================================
	CreateShaderDebugStorage
=================================================
*/
static void CreateShaderDebugStorage (uint32_t setIndex, DebugInfo &dbgInfo, OUT uint64_t &posOffset, OUT uint64_t &dataOffset)
{
	// "layout(binding=x, std430) buffer dbg_ShaderTraceStorage { ... } dbg_ShaderTrace"
	
	TPublicType		uint_type;			uint_type.init({});
	uint_type.basicType					= TBasicType::EbtUint;
	uint_type.qualifier.storage			= TStorageQualifier::EvqBuffer;
	uint_type.qualifier.layoutMatrix	= TLayoutMatrix::ElmColumnMajor;
	uint_type.qualifier.layoutPacking	= TLayoutPacking::ElpStd430;
	uint_type.qualifier.precision		= TPrecisionQualifier::EpqHigh;
	uint_type.qualifier.layoutOffset	= 0;
	
	TTypeList*		type_list	= new TTypeList{};
	TPublicType		temp		= uint_type;

	
	switch ( dbgInfo.GetShaderType() )
	{
		/*case EShLangVertex :
		case EShLangTessControl :
		case EShLangTessEvaluation :
		case EShLangGeometry :*/
		case EShLangFragment :			CreateFragmentShaderDebugStorage( type_list, INOUT temp );	break;
		case EShLangCompute :			CreateComputeShaderDebugStorage( type_list, INOUT temp );	break;
		/*case EShLangRayGenNV :
		case EShLangIntersectNV :
		case EShLangAnyHitNV :
		case EShLangClosestHitNV :
		case EShLangMissNV :
		case EShLangCallableNV :
		case EShLangTaskNV :
		case EShLangMeshNV :*/
		default : CHECK(false); return;
	}

	uint_type.qualifier.layoutOffset = temp.qualifier.layoutOffset;

	TType*			position	= new TType{uint_type};		position->setFieldName( "position" );
	
	uint_type.qualifier.layoutOffset += sizeof(uint32_t);
	uint_type.arraySizes			 = new TArraySizes{};
	uint_type.arraySizes->addInnerSize();
	
	TType*			data_arr	= new TType{uint_type};		data_arr->setFieldName( "outData" );

	type_list->push_back({ position,	TSourceLoc{} });
	type_list->push_back({ data_arr,	TSourceLoc{} });

	TQualifier		block_qual;	block_qual.clear();
	block_qual.storage			= TStorageQualifier::EvqBuffer;
	block_qual.layoutMatrix		= TLayoutMatrix::ElmColumnMajor;
	block_qual.layoutPacking	= TLayoutPacking::ElpStd430;
	block_qual.layoutBinding	= 0;
	block_qual.layoutSet		= setIndex;

	TIntermSymbol*	storage_buf	= new TIntermSymbol{ 0x10000001, "dbg_ShaderTrace", TType{type_list, "dbg_ShaderTraceStorage", block_qual} };

	posOffset  = position->getQualifier().layoutOffset;
	dataOffset = data_arr->getQualifier().layoutOffset;
	
	dbgInfo.SetDebugStorage( storage_buf );
}

/*
=================================================
	InsertDebugStorageBuffer
=================================================
*/
static bool InsertDebugStorageBuffer (TIntermNode* root, DebugInfo &dbgInfo)
{
	TIntermAggregate*	aggr		= root->getAsAggregate();
	TIntermAggregate*	linker_objs	= nullptr;
	TIntermAggregate*	global_vars	= nullptr;

	for (auto& node : aggr->getSequence())
	{
		if ( TIntermAggregate*  temp = node->getAsAggregate() )
		{
			if ( temp->getOp() == TOperator::EOpLinkerObjects )
				linker_objs = temp;

			if ( temp->getOp() == TOperator::EOpSequence )
				global_vars = temp;
		}
	}

	if ( not linker_objs )
	{
		linker_objs = new TIntermAggregate{ TOperator::EOpLinkerObjects };
		aggr->getSequence().push_back( linker_objs );
	}

	if ( not global_vars )
	{
		global_vars = new TIntermAggregate{ TOperator::EOpSequence };
		aggr->getSequence().push_back( global_vars );
	}
	
	// "uint dbg_LastPosition = ~0u"
	
	TPublicType		uint_type;	uint_type.init({});
	uint_type.basicType			= TBasicType::EbtUint;
	uint_type.qualifier.storage = TStorageQualifier::EvqGlobal;

	TIntermSymbol*			last_pos		= new TIntermSymbol{ dbgInfo.GetUniqueSymbolID(), "dbg_LastPosition", TType{uint_type} };
	linker_objs->getSequence().push_back( last_pos );
	dbgInfo.CacheSymbolNode( last_pos );
	
	uint_type.qualifier.storage = TStorageQualifier::EvqConst;
	TConstUnionArray		init_value(1);	init_value[0].setUConst( ~0u );
	TIntermConstantUnion*	init_const		= new TIntermConstantUnion{ init_value, TType{uint_type} };
	TIntermBinary*			init_pos		= new TIntermBinary{ TOperator::EOpAssign };
		
	uint_type.qualifier.storage = TStorageQualifier::EvqTemporary;
	init_pos->setType( TType{uint_type} );
	init_pos->setLeft( last_pos );
	init_pos->setRight( init_const );

	global_vars->getSequence().insert( global_vars->getSequence().begin(), init_pos );
	
	linker_objs->getSequence().push_back( dbgInfo.GetDebugStorage() );
	return true;
}

/*
=================================================
	CreateShaderBuiltinSymbols
=================================================
*/
static void CreateShaderBuiltinSymbols (TIntermNode* root, DebugInfo &dbgInfo)
{
	const auto	shader	= dbgInfo.GetShaderType();
	TSourceLoc	loc		{};

	// find default source location
	/*if ( auto* aggr = root->getAsAggregate() )
	{
		for (auto& node : aggr->getSequence())
		{
			auto*	fn = node->getAsAggregate();

			if ( fn and fn->getOp() == TOperator::EOpFunction and fn->getName() == "main(" )
			{
				loc = fn->getLoc();
				break;
			}
		}
	}*/

	if ( shader == EShLangFragment and not dbgInfo.GetCachedSymbolNode( "gl_FragCoord" ))
	{
		TPublicType		vec4_type;	vec4_type.init({});
		vec4_type.basicType			= TBasicType::EbtFloat;
		vec4_type.vectorSize		= 4;
		vec4_type.qualifier.storage	= TStorageQualifier::EvqFragCoord;
		vec4_type.qualifier.builtIn	= TBuiltInVariable::EbvFragCoord;
		
		TIntermSymbol*	symb = new TIntermSymbol{ dbgInfo.GetUniqueSymbolID(), "gl_FragCoord", TType{vec4_type} };
		symb->setLoc( loc );
		dbgInfo.CacheSymbolNode( symb );
	}
	
	// "Any static use of this variable in a fragment shader causes the entire shader to be evaluated per-sample rather than per-fragment."
	// so don't add 'gl_SampleID' and 'gl_SamplePosition' if it doesn't exists.
	
	if ( shader == EShLangFragment and not dbgInfo.GetCachedSymbolNode( "gl_PrimitiveID" ))
	{
		TPublicType		int_type;	int_type.init({});
		int_type.basicType			= TBasicType::EbtInt;
		int_type.qualifier.storage	= TStorageQualifier::EvqVaryingIn;
		int_type.qualifier.builtIn	= TBuiltInVariable::EbvPrimitiveId;
		int_type.qualifier.flat		= true;

		TIntermSymbol*	symb = new TIntermSymbol{ dbgInfo.GetUniqueSymbolID(), "gl_PrimitiveID", TType{int_type} };
		symb->setLoc( loc );
		dbgInfo.CacheSymbolNode( symb );
	}

	if ( shader == EShLangCompute and not dbgInfo.GetCachedSymbolNode( "gl_GlobalInvocationID" ))
	{
		TPublicType		uint_type;	uint_type.init({});
		uint_type.basicType			= TBasicType::EbtUint;
		uint_type.qualifier.storage	= TStorageQualifier::EvqVaryingIn;
		uint_type.qualifier.builtIn	= TBuiltInVariable::EbvGlobalInvocationId;

		TIntermSymbol*	symb = new TIntermSymbol{ dbgInfo.GetUniqueSymbolID(), "gl_GlobalInvocationID", TType{uint_type} };
		symb->setLoc( loc );
		dbgInfo.CacheSymbolNode( symb );
	}

	if ( shader == EShLangCompute and not dbgInfo.GetCachedSymbolNode( "gl_LocalInvocationID" ))
	{
		TPublicType		uint_type;	uint_type.init({});
		uint_type.basicType			= TBasicType::EbtUint;
		uint_type.qualifier.storage	= TStorageQualifier::EvqVaryingIn;
		uint_type.qualifier.builtIn	= TBuiltInVariable::EbvLocalInvocationId;

		TIntermSymbol*	symb = new TIntermSymbol{ dbgInfo.GetUniqueSymbolID(), "gl_LocalInvocationID", TType{uint_type} };
		symb->setLoc( loc );
		dbgInfo.CacheSymbolNode( symb );
	}

	if ( shader == EShLangCompute and not dbgInfo.GetCachedSymbolNode( "gl_WorkGroupID" ))
	{
		TPublicType		uint_type;	uint_type.init({});
		uint_type.basicType			= TBasicType::EbtUint;
		uint_type.qualifier.storage	= TStorageQualifier::EvqVaryingIn;
		uint_type.qualifier.builtIn	= TBuiltInVariable::EbvWorkGroupId;

		TIntermSymbol*	symb = new TIntermSymbol{ dbgInfo.GetUniqueSymbolID(), "gl_WorkGroupID", TType{uint_type} };
		symb->setLoc( loc );
		dbgInfo.CacheSymbolNode( symb );
	}

	// TODO: other builtins
}

/*
=================================================
	CreateAppendToTraceBody
=================================================
*/
static TIntermAggregate*  CreateAppendToTraceBody (const TString &fnName, DebugInfo &dbgInfo)
{
	TPublicType		value_type;	value_type.init({});
	TPublicType		uint_type;	uint_type.init({});

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

	const uint32_t		dbg_data_size = value_type.vectorSize + 3;	// last_pos, location, size, value

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
	
	// "uint pos = atomicAdd( dbg_ShaderTrace.position, x );"
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
		
		TIntermAggregate*		add_op	= new TIntermAggregate{ TOperator::EOpAtomicAdd };		// atomicAdd
		uint_type.qualifier.storage = TStorageQualifier::EvqGlobal;
		add_op->setType( TType{uint_type} );
		add_op->setOperationPrecision( TPrecisionQualifier::EpqHigh );
		add_op->getQualifierList().push_back( TStorageQualifier::EvqInOut );
		add_op->getQualifierList().push_back( TStorageQualifier::EvqIn );
		add_op->getSequence().push_back( dbgInfo.GetDebugStorageField("position") );
		add_op->getSequence().push_back( data_size );

		assign_op->setType( TType{uint_type} );
		assign_op->setLeft( var_pos );
		assign_op->setRight( add_op );
	}

	// "dbg_ShaderTrace.outData[pos++] = ..."
	{
		uint_type.qualifier.storage = TStorageQualifier::EvqConst;
		TConstUnionArray		type_value(1);	type_value[0].setUConst( (uint32_t(value_type.basicType) & 0xFF) | (value_type.vectorSize << 8) );
		TIntermConstantUnion*	type_id			= new TIntermConstantUnion{ type_value, TType{uint_type} };
		TConstUnionArray		const_value(1);	const_value[0].setUConst( 1 );
		TIntermConstantUnion*	const_one		= new TIntermConstantUnion{ const_value, TType{uint_type} };
		
		uint_type.qualifier.storage = TStorageQualifier::EvqTemporary;
		TIntermBinary*			assign_data0	= new TIntermBinary{ TOperator::EOpAssign };
		TIntermBinary*			assign_data1	= new TIntermBinary{ TOperator::EOpAssign };
		TIntermBinary*			assign_data2	= new TIntermBinary{ TOperator::EOpAssign };
		TIntermBinary*			indexed_access	= new TIntermBinary{ TOperator::EOpIndexIndirect };
		TIntermUnary*			inc_pos			= new TIntermUnary{ TOperator::EOpPostIncrement };
		TIntermSymbol*			last_pos		= dbgInfo.GetCachedSymbolNode( "dbg_LastPosition" );
		TIntermBinary*			new_last_pos	= new TIntermBinary{ TOperator::EOpAssign };
		TIntermBinary*			prev_pos		= new TIntermBinary{ TOperator::EOpSub };

		// "pos++"
		inc_pos->setType( TType{uint_type} );
		inc_pos->setOperand( var_pos );

		// "dbg_ShaderTrace.outData[pos++]"
		indexed_access->setType( TType{uint_type} );
		indexed_access->getWritableType().setFieldName( "outData" );
		indexed_access->setLeft( dbgInfo.GetDebugStorageField("outData") );
		indexed_access->setRight( inc_pos );
		
		// "dbg_ShaderTrace.outData[pos++] = dbg_LastPosition"
		assign_data0->setType( TType{uint_type} );
		assign_data0->setLeft( indexed_access );
		assign_data0->setRight( last_pos );
		branch_body->getSequence().push_back( assign_data0 );
		
		// "pos - 1"
		prev_pos->setType( TType{uint_type} );
		prev_pos->setLeft( var_pos );
		prev_pos->setRight( const_one );

		// "dbg_LastPosition = pos - 1"
		new_last_pos->setType( TType{uint_type} );
		new_last_pos->setLeft( last_pos );
		new_last_pos->setRight( prev_pos );
		branch_body->getSequence().push_back( new_last_pos );

		// "dbg_ShaderTrace.outData[pos++] = sourceLocation"
		assign_data1->setType( TType{uint_type} );
		assign_data1->setLeft( indexed_access );
		assign_data1->setRight( fn_args->getSequence()[1]->getAsTyped() );
		branch_body->getSequence().push_back( assign_data1 );
		
		// "dbg_ShaderTrace.outData[pos++] = typeid"
		assign_data2->setType( TType{uint_type} );
		assign_data2->setLeft( indexed_access );
		assign_data2->setRight( type_id );
		branch_body->getSequence().push_back( assign_data2 );
		
		// "ToUint(...)"
		const auto	TypeToUint	= [] (TIntermTyped* operand) -> TIntermTyped*
		{
			TPublicType		utype;	utype.init({});
			utype.basicType			= TBasicType::EbtUint;
			utype.qualifier.storage = TStorageQualifier::EvqGlobal;

			switch ( operand->getType().getBasicType() )
			{
				case TBasicType::EbtFloat : {
					TIntermUnary*	as_uint = new TIntermUnary{ TOperator::EOpFloatBitsToUint };
					as_uint->setType( TType{utype} );
					as_uint->setOperand( operand );
					as_uint->setOperationPrecision( TPrecisionQualifier::EpqHigh );
					return as_uint;
				}
				case TBasicType::EbtInt : {
					TIntermUnary*	to_uint = new TIntermUnary{ TOperator::EOpConvIntToUint };
					to_uint->setType( TType{utype} );
					to_uint->setOperand( operand );
					return to_uint;
				}
				case TBasicType::EbtUint :
					return operand;
			}
			RETURN_ERR( "not supported" );
		};

		// "dbg_ShaderTrace.outData[pos++] = ToUint(value)"
		if ( value_type.isScalar() )
		{
			TIntermBinary*	assign_data3	= new TIntermBinary{ TOperator::EOpAssign };
			TIntermTyped*	scalar			= fn_args->getSequence()[0]->getAsTyped();
			
			assign_data3->setType( TType{uint_type} );
			assign_data3->setLeft( indexed_access );
			assign_data3->setRight( TypeToUint( scalar ));
			branch_body->getSequence().push_back( assign_data3 );
		}
		else
		for (int i = 0; i < value_type.vectorSize; ++i)
		{
			TPublicType		pub_type;	pub_type.init({});
			pub_type.basicType			= TBasicType::EbtInt;
			pub_type.qualifier.storage	= TStorageQualifier::EvqConst;

			TIntermBinary*			assign_data3	= new TIntermBinary{ TOperator::EOpAssign };
			TConstUnionArray		field_index(1);	field_index[0].setIConst( i );
			TIntermConstantUnion*	vec_field		= new TIntermConstantUnion{ field_index, TType{pub_type} };
			TIntermBinary*			field_access	= new TIntermBinary{ TOperator::EOpIndexDirect };
			TIntermTyped*			vec				= fn_args->getSequence()[0]->getAsTyped();

			pub_type.basicType			= vec->getType().getBasicType();
			pub_type.qualifier.storage	= vec->getType().getQualifier().storage;

			// "value.x"
			field_access->setType( TType{pub_type} );
			field_access->setLeft( vec );
			field_access->setRight( vec_field );
			
			// "dbg_ShaderTrace.outData[pos++] = ToUint(value.x)"
			assign_data3->setType( TType{uint_type} );
			assign_data3->setLeft( indexed_access );
			assign_data3->setRight( TypeToUint( field_access ));
			branch_body->getSequence().push_back( assign_data3 );
		}
	}

	// "if ( dbg_IsDebugInvocation() )"
	{
		TPublicType		bool_type;	bool_type.init({});
		bool_type.basicType			= TBasicType::EbtBool;
		bool_type.qualifier.storage	= TStorageQualifier::EvqGlobal;

		TIntermAggregate*	condition	= new TIntermAggregate{ TOperator::EOpFunctionCall };
		condition->setType( TType{bool_type} );
		condition->setName( "dbg_IsDebugInvocation(" );
		condition->setUserDefined();

		TIntermSelection*	selection	= new TIntermSelection{ condition, branch_body, nullptr };
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
	DumpFragmentShaderInfo
=================================================
*/
static TIntermNode*  DumpFragmentShaderInfo (const TSourceLoc &loc, DebugInfo &dbgInfo)
{
	TIntermAggregate*	branch_body	= new TIntermAggregate{ TOperator::EOpSequence };
	branch_body->setType( TType{EbtVoid} );
	
	TPublicType		int_type;	int_type.init({});
	int_type.basicType			= TBasicType::EbtInt;
	int_type.qualifier.storage	= TStorageQualifier::EvqGlobal;
	
	TPublicType		uint_type;	uint_type.init({});
	uint_type.basicType			= TBasicType::EbtUint;
	uint_type.qualifier.storage	= TStorageQualifier::EvqConst;

	TPublicType		float_type;	float_type.init({});
	float_type.basicType		= TBasicType::EbtFloat;
	float_type.qualifier.storage= TStorageQualifier::EvqGlobal;

	// "dbg_AppendToTrace( gl_FragCoord, location )"
	{
		TIntermAggregate*		fncall			= new TIntermAggregate{ TOperator::EOpFunctionCall };
		TIntermSymbol*			fragcoord		= dbgInfo.GetCachedSymbolNode( "gl_FragCoord" );
		TConstUnionArray		loc_value(1);	loc_value[0].setUConst( dbgInfo.GetCustomSourceLocation( fragcoord, loc ));
		TIntermConstantUnion*	loc_const		= new TIntermConstantUnion{ loc_value, TType{uint_type} };
		
		float_type.vectorSize = 4;
		fncall->setType( TType{float_type} );
		fncall->setUserDefined();
		fncall->setName("dbg_AppendToTrace(vf4;u1;");
		fncall->getQualifierList().push_back( TStorageQualifier::EvqConstReadOnly );
		fncall->getQualifierList().push_back( TStorageQualifier::EvqConstReadOnly );
		fncall->getSequence().push_back( fragcoord );
		fncall->getSequence().push_back( loc_const );
		
		dbgInfo.RequestFunc( fncall->getName() );
		branch_body->getSequence().push_back( fncall );
	}

	// "dbg_AppendToTrace( gl_SampleID, location )"
	if ( TIntermSymbol*  sample_id = dbgInfo.GetCachedSymbolNode( "gl_SampleID" ))
	{
		TIntermAggregate*		fncall			= new TIntermAggregate{ TOperator::EOpFunctionCall };
		TConstUnionArray		loc_value(1);	loc_value[0].setUConst( dbgInfo.GetCustomSourceLocation( sample_id, loc ));
		TIntermConstantUnion*	loc_const		= new TIntermConstantUnion{ loc_value, TType{uint_type} };

		fncall->setUserDefined();
		fncall->setName("dbg_AppendToTrace(i1;u1;");
		fncall->setType( TType{int_type} );
		fncall->getQualifierList().push_back( TStorageQualifier::EvqConstReadOnly );
		fncall->getQualifierList().push_back( TStorageQualifier::EvqConstReadOnly );
		fncall->getSequence().push_back( sample_id );
		fncall->getSequence().push_back( loc_const );

		dbgInfo.RequestFunc( fncall->getName() );
		branch_body->getSequence().push_back( fncall );
	}

	// "dbg_AppendToTrace( gl_SamplePosition, location )"
	if ( TIntermSymbol*  sample_pos = dbgInfo.GetCachedSymbolNode( "gl_SamplePosition" ))
	{
		TIntermAggregate*		fncall			= new TIntermAggregate{ TOperator::EOpFunctionCall };
		TConstUnionArray		loc_value(1);	loc_value[0].setUConst( dbgInfo.GetCustomSourceLocation( sample_pos, loc ));
		TIntermConstantUnion*	loc_const		= new TIntermConstantUnion{ loc_value, TType{uint_type} };
		
		float_type.vectorSize = 2;
		fncall->setType( TType{float_type} );
		fncall->setUserDefined();
		fncall->setName("dbg_AppendToTrace(vf2;u1;");
		fncall->getQualifierList().push_back( TStorageQualifier::EvqConstReadOnly );
		fncall->getQualifierList().push_back( TStorageQualifier::EvqConstReadOnly );
		fncall->getSequence().push_back( sample_pos );
		fncall->getSequence().push_back( loc_const );

		dbgInfo.RequestFunc( fncall->getName() );
		branch_body->getSequence().push_back( fncall );
	}
	
	// "dbg_AppendToTrace( gl_PrimitiveID, location )"
	{
		TIntermAggregate*		fncall			= new TIntermAggregate{ TOperator::EOpFunctionCall };
		TIntermSymbol*			primitive_id	= dbgInfo.GetCachedSymbolNode( "gl_PrimitiveID" );
		TConstUnionArray		loc_value(1);	loc_value[0].setUConst( dbgInfo.GetCustomSourceLocation( primitive_id, loc ));
		TIntermConstantUnion*	loc_const		= new TIntermConstantUnion{ loc_value, TType{uint_type} };

		fncall->setUserDefined();
		fncall->setName("dbg_AppendToTrace(i1;u1;");
		fncall->setType( TType{int_type} );
		fncall->getQualifierList().push_back( TStorageQualifier::EvqConstReadOnly );
		fncall->getQualifierList().push_back( TStorageQualifier::EvqConstReadOnly );
		fncall->getSequence().push_back( primitive_id );
		fncall->getSequence().push_back( loc_const );

		dbgInfo.RequestFunc( fncall->getName() );
		branch_body->getSequence().push_back( fncall );
	}
	
	// "dbg_AppendToTrace( gl_SamplePosition, location )"
	if ( TIntermSymbol*  layer = dbgInfo.GetCachedSymbolNode( "gl_Layer" ))
	{
		TIntermAggregate*		fncall			= new TIntermAggregate{ TOperator::EOpFunctionCall };
		TConstUnionArray		loc_value(1);	loc_value[0].setUConst( dbgInfo.GetCustomSourceLocation( layer, loc ));
		TIntermConstantUnion*	loc_const		= new TIntermConstantUnion{ loc_value, TType{uint_type} };
		
		fncall->setUserDefined();
		fncall->setName("dbg_AppendToTrace(i1;u1;");
		fncall->setType( TType{int_type} );
		fncall->getQualifierList().push_back( TStorageQualifier::EvqConstReadOnly );
		fncall->getQualifierList().push_back( TStorageQualifier::EvqConstReadOnly );
		fncall->getSequence().push_back( layer );
		fncall->getSequence().push_back( loc_const );

		dbgInfo.RequestFunc( fncall->getName() );
		branch_body->getSequence().push_back( fncall );
	}

	// "dbg_AppendToTrace( gl_ViewportIndex, location )"
	if ( TIntermSymbol*  viewport_idx = dbgInfo.GetCachedSymbolNode( "gl_ViewportIndex" ))
	{
		TIntermAggregate*		fncall			= new TIntermAggregate{ TOperator::EOpFunctionCall };
		TConstUnionArray		loc_value(1);	loc_value[0].setUConst( dbgInfo.GetCustomSourceLocation( viewport_idx, loc ));
		TIntermConstantUnion*	loc_const		= new TIntermConstantUnion{ loc_value, TType{uint_type} };

		fncall->setUserDefined();
		fncall->setName("dbg_AppendToTrace(i1;u1;");
		fncall->setType( TType{int_type} );
		fncall->getQualifierList().push_back( TStorageQualifier::EvqConstReadOnly );
		fncall->getQualifierList().push_back( TStorageQualifier::EvqConstReadOnly );
		fncall->getSequence().push_back( viewport_idx );
		fncall->getSequence().push_back( loc_const );

		dbgInfo.RequestFunc( fncall->getName() );
		branch_body->getSequence().push_back( fncall );
	}

	// "if ( dbg_IsDebugInvocation() )"
	TPublicType		bool_type;	bool_type.init({});
	bool_type.basicType			= TBasicType::EbtBool;
	bool_type.qualifier.storage	= TStorageQualifier::EvqGlobal;

	TIntermAggregate*	condition	= new TIntermAggregate{ TOperator::EOpFunctionCall };
	condition->setType( TType{bool_type} );
	condition->setName( "dbg_IsDebugInvocation(" );
	condition->setUserDefined();

	TIntermSelection*	selection	= new TIntermSelection{ condition, branch_body, nullptr };
	selection->setType( TType{EbtVoid} );

	return selection;
}

/*
=================================================
	DumpComputeShaderInfo
=================================================
*/
static TIntermNode*  DumpComputeShaderInfo (const TSourceLoc &loc, DebugInfo &dbgInfo)
{
	TIntermAggregate*	branch_body	= new TIntermAggregate{ TOperator::EOpSequence };
	branch_body->setType( TType{EbtVoid} );
	
	TPublicType		uint_type;	uint_type.init({});
	uint_type.basicType			= TBasicType::EbtUint;
	uint_type.qualifier.storage	= TStorageQualifier::EvqGlobal;

	// "dbg_AppendToTrace( gl_GlobalInvocationID, location )"
	{
		TIntermAggregate*		fncall			= new TIntermAggregate{ TOperator::EOpFunctionCall };
		TIntermSymbol*			invocation		= dbgInfo.GetCachedSymbolNode( "gl_GlobalInvocationID" );
		TConstUnionArray		loc_value(1);	loc_value[0].setUConst( dbgInfo.GetCustomSourceLocation( invocation, loc ));
		TIntermConstantUnion*	loc_const		= new TIntermConstantUnion{ loc_value, TType{uint_type} };
		
		uint_type.vectorSize = 3;
		fncall->setType( TType{uint_type} );
		fncall->setUserDefined();
		fncall->setName("dbg_AppendToTrace(vu3;u1;");
		fncall->getQualifierList().push_back( TStorageQualifier::EvqConstReadOnly );
		fncall->getQualifierList().push_back( TStorageQualifier::EvqConstReadOnly );
		fncall->getSequence().push_back( invocation );
		fncall->getSequence().push_back( loc_const );
		
		dbgInfo.RequestFunc( fncall->getName() );
		branch_body->getSequence().push_back( fncall );
	}

	// "if ( dbg_IsDebugInvocation() )"
	TPublicType		bool_type;	bool_type.init({});
	bool_type.basicType			= TBasicType::EbtBool;
	bool_type.qualifier.storage	= TStorageQualifier::EvqGlobal;

	TIntermAggregate*	condition	= new TIntermAggregate{ TOperator::EOpFunctionCall };
	condition->setType( TType{bool_type} );
	condition->setName( "dbg_IsDebugInvocation(" );
	condition->setUserDefined();

	TIntermSelection*	selection	= new TIntermSelection{ condition, branch_body, nullptr };
	selection->setType( TType{EbtVoid} );

	return selection;
}

/*
=================================================
	DumpShaderInfo
=================================================
*/
static TIntermNode*  DumpShaderInfo (const TSourceLoc &loc, DebugInfo &dbgInfo)
{
	switch ( dbgInfo.GetShaderType() )
	{
		case EShLangVertex : return nullptr;
		case EShLangTessControl : return nullptr;
		case EShLangTessEvaluation : return nullptr;
		case EShLangGeometry : return nullptr;
		case EShLangFragment : return DumpFragmentShaderInfo( loc, dbgInfo );
		case EShLangCompute : return DumpComputeShaderInfo( loc, dbgInfo );
		/*case EShLangRayGenNV : return nullptr;
		case EShLangIntersectNV : return nullptr;
		case EShLangAnyHitNV : return nullptr;
		case EShLangClosestHitNV : return nullptr;
		case EShLangMissNV : return nullptr;
		case EShLangCallableNV : return nullptr;
		case EShLangTaskNV : return nullptr;
		case EShLangMeshNV : return nullptr;	// TODO*/
	}
	return nullptr;
}

/*
=================================================
	CreateFragmentShaderIsDebugInvocation
=================================================
*/
static TIntermAggregate*  CreateFragmentShaderIsDebugInvocation (const TString &fnName, DebugInfo &dbgInfo)
{
	TIntermAggregate*	fn_node		= new TIntermAggregate{ TOperator::EOpFunction };
	TIntermAggregate*	fn_args		= new TIntermAggregate{ TOperator::EOpParameters };
	TIntermAggregate*	fn_body		= new TIntermAggregate{ TOperator::EOpSequence };
	
	TPublicType		bool_type;	bool_type.init({});
	bool_type.basicType			= TBasicType::EbtBool;
	bool_type.qualifier.storage	= TStorageQualifier::EvqGlobal;

	TPublicType		uint_type;	uint_type.init({});
	uint_type.basicType			= TBasicType::EbtUint;
	uint_type.qualifier.storage	= TStorageQualifier::EvqTemporary;
	
	TPublicType		float_type;	 float_type.init({});
	float_type.basicType		 = TBasicType::EbtFloat;
	float_type.qualifier.storage = TStorageQualifier::EvqTemporary;
	
	TPublicType		index_type;	 index_type.init({});
	index_type.basicType		 = TBasicType::EbtInt;
	index_type.qualifier.storage = TStorageQualifier::EvqConst;

	// build function body
	{
		fn_node->setType( TType{bool_type} );
		fn_node->setName( fnName );
		fn_node->getSequence().push_back( fn_args );
		fn_node->getSequence().push_back( fn_body );
		
		fn_args->setType( TType{EbtVoid} );
		fn_body->setType( TType{EbtVoid} );
	}
	
	bool_type.qualifier.storage	= TStorageQualifier::EvqTemporary;

	// dbg_ShaderTrace.fragCoordX
	TIntermBinary*			fscoord_x	= dbgInfo.GetDebugStorageField( "fragCoordX" );
	CHECK_ERR( fscoord_x );

	// gl_FragCoord.x
	TConstUnionArray		x_index(1);	x_index[0].setIConst( 0 );
	TIntermConstantUnion*	x_field		= new TIntermConstantUnion{ x_index, TType{index_type} };
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
	eq1->setType( TType{bool_type} );
	eq1->setLeft( uint_fc_x );
	eq1->setRight( fscoord_x );

	
	// dbg_ShaderTrace.fragCoordY
	TIntermBinary*			fscoord_y	= dbgInfo.GetDebugStorageField( "fragCoordY" );
	CHECK_ERR( fscoord_y );

	// gl_FragCoord.y
	TConstUnionArray		y_index(1);	y_index[0].setIConst( 1 );
	TIntermConstantUnion*	y_field		= new TIntermConstantUnion{ y_index, TType{index_type} };
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
	CreateComputeShaderIsDebugInvocation
=================================================
*/
static TIntermAggregate*  CreateComputeShaderIsDebugInvocation (const TString &fnName, DebugInfo &dbgInfo)
{
	TIntermAggregate*	fn_node		= new TIntermAggregate{ TOperator::EOpFunction };
	TIntermAggregate*	fn_args		= new TIntermAggregate{ TOperator::EOpParameters };
	TIntermAggregate*	fn_body		= new TIntermAggregate{ TOperator::EOpSequence };
	
	TPublicType		bool_type;	bool_type.init({});
	bool_type.basicType			= TBasicType::EbtBool;
	bool_type.qualifier.storage	= TStorageQualifier::EvqGlobal;

	TPublicType		uint_type;	uint_type.init({});
	uint_type.basicType			= TBasicType::EbtUint;
	uint_type.qualifier.storage	= TStorageQualifier::EvqTemporary;

	TPublicType		index_type;	 index_type.init({});
	index_type.basicType		 = TBasicType::EbtInt;
	index_type.qualifier.storage = TStorageQualifier::EvqConst;

	// build function body
	{
		fn_node->setType( TType{bool_type} );
		fn_node->setName( fnName );
		fn_node->getSequence().push_back( fn_args );
		fn_node->getSequence().push_back( fn_body );
		
		fn_args->setType( TType{EbtVoid} );
		fn_body->setType( TType{EbtVoid} );
	}
	
	bool_type.qualifier.storage	= TStorageQualifier::EvqTemporary;

	TIntermBinary*	eq1 = new TIntermBinary{ TOperator::EOpEqual };
	{
		// dbg_ShaderTrace.globalInvocationX
		TIntermBinary*			thread_id_x	= dbgInfo.GetDebugStorageField( "globalInvocationX" );
		CHECK_ERR( thread_id_x );

		// gl_GlobalInvocationID.x
		TConstUnionArray		x_index(1);	x_index[0].setIConst( 0 );
		TIntermConstantUnion*	x_field		= new TIntermConstantUnion{ x_index, TType{index_type} };
		TIntermBinary*			ginvoc_x	= new TIntermBinary{ TOperator::EOpIndexDirect };
		ginvoc_x->setType( TType{uint_type} );
		ginvoc_x->setLeft( dbgInfo.GetCachedSymbolNode( "gl_GlobalInvocationID" ));
		ginvoc_x->setRight( x_field );
	
		// ... == ...
		eq1->setType( TType{bool_type} );
		eq1->setLeft( thread_id_x );
		eq1->setRight( ginvoc_x );
	}
	
	TIntermBinary*	eq2 = new TIntermBinary{ TOperator::EOpEqual };
	{
		// dbg_ShaderTrace.globalInvocationY
		TIntermBinary*			thread_id_y	= dbgInfo.GetDebugStorageField( "globalInvocationY" );
		CHECK_ERR( thread_id_y );

		// gl_GlobalInvocationID.y
		TConstUnionArray		y_index(1);	y_index[0].setIConst( 1 );
		TIntermConstantUnion*	y_field		= new TIntermConstantUnion{ y_index, TType{index_type} };
		TIntermBinary*			ginvoc_y	= new TIntermBinary{ TOperator::EOpIndexDirect };
		ginvoc_y->setType( TType{uint_type} );
		ginvoc_y->setLeft( dbgInfo.GetCachedSymbolNode( "gl_GlobalInvocationID" ));
		ginvoc_y->setRight( y_field );
	
		// ... == ...
		eq2->setType( TType{bool_type} );
		eq2->setLeft( thread_id_y );
		eq2->setRight( ginvoc_y );
	}
	
	TIntermBinary*	eq3 = new TIntermBinary{ TOperator::EOpEqual };
	{
		// dbg_ShaderTrace.globalInvocationZ
		TIntermBinary*			thread_id_z	= dbgInfo.GetDebugStorageField( "globalInvocationZ" );
		CHECK_ERR( thread_id_z );

		// gl_GlobalInvocationID.z
		TConstUnionArray		z_index(1);	z_index[0].setIConst( 2 );
		TIntermConstantUnion*	z_field		= new TIntermConstantUnion{ z_index, TType{index_type} };
		TIntermBinary*			ginvoc_z	= new TIntermBinary{ TOperator::EOpIndexDirect };
		ginvoc_z->setType( TType{uint_type} );
		ginvoc_z->setLeft( dbgInfo.GetCachedSymbolNode( "gl_GlobalInvocationID" ));
		ginvoc_z->setRight( z_field );
	
		// ... == ...
		eq3->setType( TType{bool_type} );
		eq3->setLeft( thread_id_z );
		eq3->setRight( ginvoc_z );
	}
	
	// ... && ...
	TIntermBinary*		cmp1		= new TIntermBinary{ TOperator::EOpLogicalAnd };
	cmp1->setType( TType{bool_type} );
	cmp1->setLeft( eq1 );
	cmp1->setRight( eq2 );
	
	// ... && ...
	TIntermBinary*		cmp2		= new TIntermBinary{ TOperator::EOpLogicalAnd };
	cmp2->setType( TType{bool_type} );
	cmp2->setLeft( cmp1 );
	cmp2->setRight( eq3 );

	// return ...
	TIntermBranch*		fn_return	= new TIntermBranch{ TOperator::EOpReturn, cmp2 };
	fn_body->getSequence().push_back( fn_return );
	
	return fn_node;
}

/*
=================================================
	CreateShaderIsDebugInvocation
=================================================
*/
static TIntermAggregate*  CreateShaderIsDebugInvocation (const TString &fnName, DebugInfo &dbgInfo)
{
	switch ( dbgInfo.GetShaderType() )
	{
		case EShLangVertex : return nullptr;
		case EShLangTessControl : return nullptr;
		case EShLangTessEvaluation : return nullptr;
		case EShLangGeometry : return nullptr;
		case EShLangFragment : return CreateFragmentShaderIsDebugInvocation( fnName, dbgInfo );
		case EShLangCompute : return CreateComputeShaderIsDebugInvocation( fnName, dbgInfo );
		/*case EShLangRayGenNV :
		case EShLangIntersectNV :
		case EShLangAnyHitNV :
		case EShLangClosestHitNV :
		case EShLangMissNV :
		case EShLangCallableNV :
		case EShLangTaskNV :
		case EShLangMeshNV : return nullptr;*/
	}
	return nullptr;
}

/*
=================================================
	CreateDebugTraceFunctions
=================================================
*/
static bool CreateDebugTraceFunctions (TIntermNode* root, DebugInfo &dbgInfo)
{
	TIntermAggregate*	aggr = root->getAsAggregate();
	CHECK_ERR( aggr );

	// find 'main'
	for (auto& entry : aggr->getSequence())
	{
		auto*	func = entry->getAsAggregate();
		if ( not func )
			continue;

		if ( func->getOp() == TOperator::EOpFunction and func->getName() == "main(" )
		{
			auto*	body = func->getSequence()[1]->getAsAggregate();
			CHECK_ERR( body );

			auto*	shader_info = DumpShaderInfo( /*body->getLoc()*/ TSourceLoc{}, dbgInfo );
			CHECK_ERR( shader_info );

			body->getSequence().insert( body->getSequence().begin(), shader_info );
		}
	}

	for (auto& fn : dbgInfo.GetRequiredFunctions())
	{
		if ( fn.rfind( "dbg_AppendToTrace(", 0 ) == 0 )
		{
			auto*	body = CreateAppendToTraceBody( fn, dbgInfo );
			CHECK_ERR( body );

			aggr->getSequence().push_back( body );
		}
		else
		if ( fn == "dbg_IsDebugInvocation(" )
		{
			auto*	body = CreateShaderIsDebugInvocation( fn, dbgInfo );
			CHECK_ERR( body );

			aggr->getSequence().push_back( body );
		}
	}
	return true;
}

/*
=================================================
	CreateDebugDump
----
	returns new node instead of 'exprNode'.
	'exprNode' - any operator
	'mutableNode' - variable which content was modified by operator
=================================================
*/
static TIntermAggregate*  CreateDebugDump (TIntermTyped* exprNode, TIntermTyped* mutableNode, DebugInfo &dbgInfo)
{
	TIntermAggregate*	fcall		= new TIntermAggregate( TOperator::EOpFunctionCall );
	TString				type_name;
	const TType &		type		= exprNode->getType();

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
		case TBasicType::EbtNumTypes :	RETURN_ERR( "not supported" );
	}
	
	if ( type.isArray() )
		RETURN_ERR( "arrays is not supported yet" )
	else
	if ( type.isMatrix() )
		RETURN_ERR( "matrix is not supported yet" )
	else
	if ( type.isVector() )
		type_name = TString("v") + type_name + TString(std::to_string(type.getVectorSize()).data());
	else
	if ( type.isScalarOrVec1() )
		type_name += "1";
	else
		RETURN_ERR( "unknown type" );


	fcall->setLoc( exprNode->getLoc() );
	fcall->setUserDefined();
	fcall->setName( "dbg_AppendToTrace(" + type_name + ";u1;" );
	fcall->setType( exprNode->getType() );
	fcall->getQualifierList().push_back( TStorageQualifier::EvqConstReadOnly );
	fcall->getQualifierList().push_back( TStorageQualifier::EvqConstReadOnly );
	fcall->getSequence().push_back( exprNode );
	
	TPublicType		uint_type;		uint_type.init( exprNode->getLoc() );
	uint_type.basicType				= TBasicType::EbtUint;
	uint_type.qualifier.storage		= TStorageQualifier::EvqConst;

	TConstUnionArray		loc_value(1);	loc_value[0].setUConst( dbgInfo.GetSourceLocation( mutableNode, exprNode->getLoc() ));
	TIntermConstantUnion*	loc_const		= new TIntermConstantUnion{ loc_value, TType{uint_type} };
	
	loc_const->setLoc( exprNode->getLoc() );
	fcall->getSequence().push_back( loc_const );

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
	
	// log out/inout parameters after call
	if ( aggr->getOp() == TOperator::EOpFunctionCall or
		 (aggr->getOp() >= TOperator::EOpRadians and aggr->getOp() < TOperator::EOpKill) )
	{
		bool	has_output = false;
		for (auto& qual : aggr->getQualifierList()) {
			if ( qual == TStorageQualifier::EvqOut or qual == TStorageQualifier::EvqInOut )
				has_output = true;
		}

		if ( has_output )
		{
			// TODO
		}
	}

	const auto	loc = dbgInfo.GetCurrentLocation();
	dbgInfo.Leave( aggr );

	if ( aggr->getOp() >= TOperator::EOpNegative	 or
		 aggr->getOp() == TOperator::EOpFunctionCall or
		 aggr->getOp() == TOperator::EOpParameters	 )
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
	
	switch ( unary->getOp() )
	{
		case TOperator::EOpPreIncrement :
		case TOperator::EOpPostIncrement :
		case TOperator::EOpPreDecrement :
		case TOperator::EOpPostDecrement :
		{
			dbgInfo.InjectNode( CreateDebugDump( unary, unary->getOperand(), dbgInfo ));
			break;
		}
	}

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
			dbgInfo.InjectNode( CreateDebugDump( binary, binary->getLeft(), dbgInfo ));
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
		new(branch) TIntermBranch{ branch->getFlowOp(), inj };

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

	new(sw) TIntermSwitch{ cond, b };
	
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

	new(selection) TIntermSelection{ cond, trueB, falseB, type };

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

	new(loop) TIntermLoop{ aBody, aTest, aTerminal, test_first };

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
	dbgInfo.AddSymbol( node );

	if ( // fragment shader
		 node->getName() == "gl_FragCoord"				or
		 node->getName() == "gl_SampleID"				or
		 node->getName() == "gl_PrimitiveID"			or
		 node->getName() == "gl_SamplePosition"			or
		 node->getName() == "gl_Layer"					or
		 node->getName() == "gl_ViewportIndex"			or
		 node->getName() == "gl_FrontFacing"			or
		 node->getName() == "gl_HelperInvocation"		or
		 // vertex shader
		 node->getName() == "gl_VertexIndex"			or
		 node->getName() == "gl_InstanceIndex"			or
		 node->getName() == "gl_DrawIDARB"				or		// requires GL_ARB_shader_draw_parameters
		 node->getName() == "gl_DrawID"					or		// requires version 460
		 // geometry shader
		 node->getName() == "gl_InvocationID"			or
		 node->getName() == "gl_PrimitiveIDIn"			or
		 // tessellation
		 //		reuse 'gl_InvocationID'
		 node->getName() == "gl_PatchVerticesIn"		or
		 node->getName() == "gl_TessCoord"				or
		 node->getName() == "gl_TessLevelInner"			or
		 node->getName() == "gl_TessLevelOuter"			or
		 // compute shader
		 node->getName() == "gl_GlobalInvocationID"		or
		 node->getName() == "gl_LocalInvocationID"		or
		 node->getName() == "gl_LocalInvocationIndex"	or
		 node->getName() == "gl_WorkGroupID"			or
		 node->getName() == "gl_NumWorkGroups"			or
		 node->getName() == "gl_WorkGroupSize"			or
		 // task & mesh shader
		 //		reuse 'gl_GlobalInvocationID'
		 //		reuse 'gl_LocalInvocationID'
		 //		reuse 'gl_LocalInvocationIndex'
		 //		reuse 'gl_WorkGroupID'
		 //		reuse 'gl_NumWorkGroups'
		 //		reuse 'gl_WorkGroupSize'
		 node->getName() == "gl_MeshViewCountNV"		or
		 // ray generation shader
		 node->getName() == "gl_LaunchIDNV"				or
		 node->getName() == "gl_LaunchSizeNV"			or
		 // ray intersection & any-hit & closest-hit & miss shaders
		 //		reuse 'gl_LaunchIDNV'
		 //		reuse 'gl_LaunchSizeNV'
		 //		reuse 'gl_PrimitiveID'
		 //		reuse 'gl_InstanceID' or 'gl_InstanceIndex' ?
		 node->getName() == "gl_InstanceCustomIndexNV"	or
		 node->getName() == "gl_WorldRayOriginNV"		or
		 node->getName() == "gl_WorldRayDirectionNV"	or
		 node->getName() == "gl_ObjectRayOriginNV"		or
		 node->getName() == "gl_ObjectRayDirectionNV"	or
		 node->getName() == "gl_RayTminNV"				or
		 node->getName() == "gl_RayTmaxNV"				or
		 node->getName() == "gl_IncomingRayFlagsNV"		or
		 node->getName() == "gl_ObjectToWorldNV"		or
		 node->getName() == "gl_WorldToObjectNV"		or
		 // ray intersection & any-hit & closest-hit shaders
		 node->getName() == "gl_HitTNV"					or
		 node->getName() == "gl_HitKindNV"				)
	{
		dbgInfo.CacheSymbolNode( node );
		return true;
	}

	// do nothing
	return true;
}
