// Copyright (c) 2018-2020,  Zhirnov Andrey. For more information see 'LICENSE'

#if defined(FG_ENABLE_OPENVR) && defined(FG_ENABLE_VULKAN)

# include "framework/VR/OpenVRDevice.h"
# include "stl/Algorithms/StringUtils.h"

# ifdef PLATFORM_WINDOWS
#	include "stl/Platforms/WindowsHeader.h"
# else
#	include <dlfcn.h>
#   include <linux/limits.h>
# endif

namespace FGC
{
namespace
{

	static constexpr char*	AxisNames[] = {
		"dpad",		// X,Y
		"trigger",	// only X axis
		"stick",	// X,Y
		"axis-3",
		"axis-4"
	};
	STATIC_ASSERT( CountOf(AxisNames) == k_unControllerStateAxisCount );

/*
=================================================
	OpenVRMatToMat4
=================================================
*/
	ND_ forceinline IVRDevice::Mat4_t  OpenVRMatToMat4 (const HmdMatrix44_t &mat)
	{
		return IVRDevice::Mat4_t{
				mat.m[0][0], mat.m[1][0], mat.m[2][0], mat.m[3][0],
				mat.m[0][1], mat.m[1][1], mat.m[2][1], mat.m[3][1], 
				mat.m[0][2], mat.m[1][2], mat.m[2][2], mat.m[3][2], 
				mat.m[0][3], mat.m[1][3], mat.m[2][3], mat.m[3][3] };
	}
	
	ND_ forceinline IVRDevice::Mat4_t  OpenVRMatToMat4 (const HmdMatrix34_t &mat)
	{
		return IVRDevice::Mat4_t{
				mat.m[0][0], mat.m[1][0], mat.m[2][0], 0.0f,
				mat.m[0][1], mat.m[1][1], mat.m[2][1], 0.0f,
				mat.m[0][2], mat.m[1][2], mat.m[2][2], 0.0f,
				mat.m[0][3], mat.m[1][3], mat.m[2][3], 1.0f };
	}

	ND_ forceinline IVRDevice::Mat3_t  OpenVRMatToMat3 (const HmdMatrix34_t &mat)
	{
		return IVRDevice::Mat3_t{
				mat.m[0][0], mat.m[1][0], mat.m[2][0],
				mat.m[0][1], mat.m[1][1], mat.m[2][1],
				mat.m[0][2], mat.m[1][2], mat.m[2][2] };
	}

}	// namespace
//-----------------------------------------------------------------------------



/*
=================================================
	constructor
=================================================
*/
	OpenVRDevice::OpenVRDevice () :
		_vkInstance{VK_NULL_HANDLE},
		_vkPhysicalDevice{VK_NULL_HANDLE},
		_vkLogicalDevice{VK_NULL_HANDLE}
	{}
	
/*
=================================================
	destructor
=================================================
*/
	OpenVRDevice::~OpenVRDevice ()
	{
		Destroy();
	}
	
/*
=================================================
	_GetTrackedDeviceString
=================================================
*/
	String  OpenVRDevice::_GetTrackedDeviceString (TrackedDeviceIndex_t unDevice, TrackedDeviceProperty prop, TrackedPropertyError *peError) const
	{
		uint32_t unRequiredBufferLen = _vrSystem->GetStringTrackedDeviceProperty( unDevice, prop, null, 0, peError );
		if( unRequiredBufferLen == 0 )
			return "";

		char *pchBuffer = new char[ unRequiredBufferLen ];
		unRequiredBufferLen = _vrSystem->GetStringTrackedDeviceProperty( unDevice, prop, pchBuffer, unRequiredBufferLen, peError );
		std::string sResult = pchBuffer;
		delete [] pchBuffer;
		return sResult;
	}

/*
=================================================
	Create
=================================================
*/
	bool  OpenVRDevice::Create ()
	{
		if ( not _LoadLib() )
		{
			FG_LOGI( "failed to load OpenVR library" );
			return false;
		}

		if ( not _ovr.IsHmdPresent() )
		{
			FG_LOGI( "VR Headset is not present" );
			return false;
		}

		if ( not _ovr.IsRuntimeInstalled() )
		{
			FG_LOGI( "VR Runtime is not installed" );
			return false;
		}

		EVRInitError	err = EVRInitError_VRInitError_None;
		_hmd = _ovr.InitInternal( OUT &err, EVRApplicationType_VRApplication_Scene );

		if ( err != EVRInitError_VRInitError_None )
			RETURN_ERR( "VR_Init error: "s << _ovr.GetVRInitErrorAsEnglishDescription( err ));
		
		_vrSystem = BitCast<IVRSystemTable>( _ovr.GetGenericInterface( ("FnTable:"s << IVRSystem_Version).c_str(), OUT &err ));
		CHECK_ERR( _vrSystem and err == EVRInitError_VRInitError_None );
		
		FG_LOGI( "driver:  "s << _GetTrackedDeviceString( k_unTrackedDeviceIndex_Hmd, ETrackedDeviceProperty_Prop_TrackingSystemName_String ) <<
				 "display: " << _GetTrackedDeviceString( k_unTrackedDeviceIndex_Hmd, ETrackedDeviceProperty_Prop_TrackingSystemName_String ));
		
		_vrCompositor = BitCast<IVRCompositorTable>( _ovr.GetGenericInterface( ("FnTable:"s << IVRCompositor_Version).c_str(), OUT &err ));
		CHECK_ERR( _vrCompositor and err == EVRInitError_VRInitError_None );

		_vrCompositor->SetTrackingSpace( ETrackingUniverseOrigin_TrackingUniverseStanding );

		EDeviceActivityLevel level = _vrSystem->GetTrackedDeviceActivityLevel( k_unTrackedDeviceIndex_Hmd );
		BEGIN_ENUM_CHECKS();
		switch ( level )
		{
			case EDeviceActivityLevel_k_EDeviceActivityLevel_Unknown :					_hmdStatus = EHmdStatus::PowerOff;	break;
			case EDeviceActivityLevel_k_EDeviceActivityLevel_Idle :						_hmdStatus = EHmdStatus::Standby;	break;
			case EDeviceActivityLevel_k_EDeviceActivityLevel_UserInteraction :			_hmdStatus = EHmdStatus::Active;	break;
			case EDeviceActivityLevel_k_EDeviceActivityLevel_UserInteraction_Timeout :	_hmdStatus = EHmdStatus::Standby;	break;
			case EDeviceActivityLevel_k_EDeviceActivityLevel_Standby :					_hmdStatus = EHmdStatus::Standby;	break;
			case EDeviceActivityLevel_k_EDeviceActivityLevel_Idle_Timeout :				_hmdStatus = EHmdStatus::Standby;	break;
		}
		END_ENUM_CHECKS();

		for (auto& listener : _listeners) {
			listener->HmdStatusChanged( _hmdStatus );
		}
		
		_InitControllers();

		return true;
	}
	
/*
=================================================
	_InitControllers
=================================================
*/
	void  OpenVRDevice::_InitControllers ()
	{
		const auto	now = TimePoint_t::clock::now();

		_controllers.clear();
		for (TrackedDeviceIndex_t i = 0; i < k_unMaxTrackedDeviceCount; ++i)
		{
			ETrackedDeviceClass	dev_class = _vrSystem->GetTrackedDeviceClass( i );

			if ( dev_class == ETrackedDeviceClass_TrackedDeviceClass_Controller or
				 dev_class == ETrackedDeviceClass_TrackedDeviceClass_GenericTracker )
			{
				Controller	cont;
				cont.id			= _GetControllerID( i );
				cont.lastPacket	= ~0u;
				cont.lastUpdate	= now;
				cont.keys.fill( false );

				_controllers.insert_or_assign( i, cont );
			}
		}
	}
	
/*
=================================================
	_GetControllerID
=================================================
*/
	OpenVRDevice::ControllerID  OpenVRDevice::_GetControllerID (uint tdi) const
	{
		ETrackedControllerRole	role = _vrSystem->GetControllerRoleForTrackedDeviceIndex( tdi );
		
		BEGIN_ENUM_CHECKS();
		switch ( role )
		{
			case ETrackedControllerRole_TrackedControllerRole_LeftHand :	return ControllerID::LeftHand;
			case ETrackedControllerRole_TrackedControllerRole_RightHand :	return ControllerID::RightHand;
			case ETrackedControllerRole_TrackedControllerRole_Invalid :
			case ETrackedControllerRole_TrackedControllerRole_OptOut :
			case ETrackedControllerRole_TrackedControllerRole_Treadmill :
			case ETrackedControllerRole_TrackedControllerRole_Max :			break;
		}
		END_ENUM_CHECKS();
		return ControllerID::Unknown;
	}

/*
=================================================
	SetVKDevice
=================================================
*/
	bool  OpenVRDevice::SetVKDevice (InstanceVk_t instance, PhysicalDeviceVk_t physicalDevice, DeviceVk_t logicalDevice)
	{
		_vkInstance			= BitCast<VkInstance>( instance );
		_vkPhysicalDevice	= BitCast<VkPhysicalDevice>( physicalDevice );
		_vkLogicalDevice	= BitCast<VkDevice>( logicalDevice );

		// check is vulkan device supports output to VR device
		{
			uint64_t	hmd_physical_device = 0;
			_vrSystem->GetOutputDevice( OUT &hmd_physical_device, ETextureType_TextureType_Vulkan, _vkInstance );

			CHECK_ERR( hmd_physical_device != 0 );
			CHECK_ERR( VkPhysicalDevice(hmd_physical_device) == _vkPhysicalDevice );
		}

		return true;
	}

/*
=================================================
	Destroy
=================================================
*/
	void  OpenVRDevice::Destroy ()
	{
		_vkInstance			= VK_NULL_HANDLE;
		_vkPhysicalDevice	= VK_NULL_HANDLE;
		_vkLogicalDevice	= VK_NULL_HANDLE;

		//_renderModels		= null;
		_hmdStatus			= EHmdStatus::PowerOff;
		_camera				= Default;

		_controllers.clear();
		_listeners.clear();

		if ( _hmd )
		{
			_ovr.ShutdownInternal();
			_hmd			= Zero;
			_vrSystem		= null;
			_vrCompositor	= null;
		}

		_UnloadLib();
	}
	
/*
=================================================
	AddListener
=================================================
*/
	void  OpenVRDevice::AddListener (IVRDeviceEventListener *listener)
	{
		ASSERT( listener );
		_listeners.insert( listener );
	}
	
/*
=================================================
	RemoveListener
=================================================
*/
	void  OpenVRDevice::RemoveListener (IVRDeviceEventListener *listener)
	{
		ASSERT( listener );
		_listeners.erase( listener );
	}

/*
=================================================
	Update
=================================================
*/
	bool  OpenVRDevice::Update ()
	{
		if ( not _vrSystem or not _vrCompositor )
			return false;

		VREvent_t	ev;
		while ( _vrSystem->PollNextEvent( OUT &ev, sizeof(ev) ))
		{
			if ( ev.trackedDeviceIndex == k_unTrackedDeviceIndex_Hmd )
				_ProcessHmdEvents( ev );

			auto	iter = _controllers.find( ev.trackedDeviceIndex );
			if ( iter != _controllers.end() )
				_ProcessControllerEvents( iter->second, ev );
		}

		const auto	now = TimePoint_t::clock::now();

		for (auto& [idx, cont] : _controllers)
		{
			VRControllerState_t	state;
			if ( _vrSystem->GetControllerState( idx, OUT &state, sizeof(state) ))
			{
				if ( state.unPacketNum == cont.lastPacket )
					continue;

				const float	dt = std::chrono::duration_cast<SecondsF>( now - cont.lastUpdate ).count();

				cont.lastPacket	= state.unPacketNum;
				cont.lastUpdate	= now;
				cont.id			= _GetControllerID( idx );

				for (size_t i = 0; i < CountOf(state.rAxis); ++i)
				{
					auto&	axis	= cont.axis[i];
					float2	v		= float2{ state.rAxis[i].x, state.rAxis[i].y };
					float2	old		= axis.value;
					float2	del		= v - old;

					axis.value = v;

					if ( not axis.pressed )
						continue;
					
					for (auto& listener : _listeners) {
						listener->OnAxisStateChanged( cont.id, AxisNames[i], v, del, dt );
					}
				}
			}
		}

		return true;
	}
	
/*
=================================================
	_ProcessHmdEvents
=================================================
*/
	void  OpenVRDevice::_ProcessHmdEvents (const VREvent_t &ev)
	{
		switch( ev.eventType )
		{
			case EVREventType_VREvent_TrackedDeviceActivated :
			case EVREventType_VREvent_TrackedDeviceUserInteractionStarted :
				if ( _hmdStatus < EHmdStatus::Active ) {
					_hmdStatus = EHmdStatus::Active;

					for (auto& listener : _listeners) {
						listener->HmdStatusChanged( _hmdStatus );
					}
				}
				break;

			case EVREventType_VREvent_TrackedDeviceUserInteractionEnded :
				_hmdStatus = EHmdStatus::Standby;

				for (auto& listener : _listeners) {
					listener->HmdStatusChanged( _hmdStatus );
				}
				break;

			case EVREventType_VREvent_ButtonPress :
				if ( ev.data.controller.button == EVRButtonId_k_EButton_ProximitySensor ) {
					_hmdStatus = EHmdStatus::Mounted;
					
					for (auto& listener : _listeners) {
						listener->HmdStatusChanged( _hmdStatus );
					}
				}
				break;

			case EVREventType_VREvent_ButtonUnpress :
				if ( ev.data.controller.button == EVRButtonId_k_EButton_ProximitySensor ) {
					_hmdStatus = EHmdStatus::Active;
					
					for (auto& listener : _listeners) {
						listener->HmdStatusChanged( _hmdStatus );
					}
				}
				break;

			case EVREventType_VREvent_Quit :
				_hmdStatus = EHmdStatus::PowerOff;
				
				for (auto& listener : _listeners) {
					listener->HmdStatusChanged( _hmdStatus );
				}
				break;

			case EVREventType_VREvent_LensDistortionChanged :
				SetupCamera( _camera.clipPlanes );
				FG_LOGI( "LensDistortionChanged" );
				break;

			case EVREventType_VREvent_PropertyChanged :
				//FG_LOGI( "PropertyChanged" );
				break;
		}
	}
	
/*
=================================================
	MapOpenVRButton
=================================================
*/
	ND_ static StringView  MapOpenVRButton (uint key)
	{
		switch ( key )
		{
			case EVRButtonId_k_EButton_System :			return "system";
			case EVRButtonId_k_EButton_ApplicationMenu:	return "app menu";
			case EVRButtonId_k_EButton_Grip :			return "grip";
			case EVRButtonId_k_EButton_DPad_Left :		return "dpad left";
			case EVRButtonId_k_EButton_DPad_Up :		return "dpad up";
			case EVRButtonId_k_EButton_DPad_Right :		return "dpad right";
			case EVRButtonId_k_EButton_DPad_Down :		return "dpad down";
			case EVRButtonId_k_EButton_A :				return "A";
			case EVRButtonId_k_EButton_Axis0 :			return "Axis 0";
			case EVRButtonId_k_EButton_Axis1 :			return "Axis 1";
			case EVRButtonId_k_EButton_Axis2 :			return "Axis 2";
			case EVRButtonId_k_EButton_Axis3 :			return "Axis 3";
			case EVRButtonId_k_EButton_Axis4 :			return "Axis 4";
		}
		return "";
	}

/*
=================================================
	_ProcessControllerEvents
=================================================
*/
	void  OpenVRDevice::_ProcessControllerEvents (INOUT Controller &cont, const VREvent_t &ev)
	{
		switch( ev.eventType )
		{
			case EVREventType_VREvent_ButtonPress :
			{
				StringView	key		= MapOpenVRButton( ev.data.controller.button );
				bool &		curr	= cont.keys[ ev.data.controller.button ];
				auto		act		= curr ? EButtonAction::Pressed : EButtonAction::Down;
				curr = true;

				for (auto& listener : _listeners) {
					listener->OnButton( cont.id, key, act );
				}
				break;
			}

			case EVREventType_VREvent_ButtonUnpress :
			{
				StringView	key		= MapOpenVRButton( ev.data.controller.button );
				cont.keys[ ev.data.controller.button ] = false;

				for (auto& listener : _listeners) {
					listener->OnButton( cont.id, key, EButtonAction::Up );
				}
				break;
			}

			case EVREventType_VREvent_ButtonTouch :
			case EVREventType_VREvent_ButtonUntouch :
			{
				uint	idx = ev.data.controller.button - EVRButtonId_k_EButton_Axis0;
				if ( idx < cont.axis.size() ) {
					cont.axis[idx].pressed = (ev.eventType == EVREventType_VREvent_ButtonTouch);
				}
				break;
			}

			case EVREventType_VREvent_MouseMove :
				FG_LOGI( "MouseMove: " + ToString(ev.data.mouse.button) + ", dev: " + ToString(ev.trackedDeviceIndex) );
				break;
			case EVREventType_VREvent_MouseButtonDown :
				FG_LOGI( "MouseButtonDown: " + ToString(ev.data.mouse.button) + ", dev: " + ToString(ev.trackedDeviceIndex) );
				break;
			case EVREventType_VREvent_MouseButtonUp :
				FG_LOGI( "MouseButtonUp: " + ToString(ev.data.mouse.button) + ", dev: " + ToString(ev.trackedDeviceIndex) );
				break;
			case EVREventType_VREvent_TouchPadMove :
				FG_LOGI( "TouchPadMove: " + ToString(ev.data.mouse.button) + ", dev: " + ToString(ev.trackedDeviceIndex) );
				break;
		}
	}

/*
=================================================
	Submit
----
	Image must be in 'VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL' layout and
	must be created with usage flags 'VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_SAMPLED_BIT'
=================================================
*/
	bool  OpenVRDevice::Submit (const VRImage &image, Eye eye)
	{
		CHECK_ERR( _hmd and _vrSystem and _vrCompositor );

		VRTextureBounds_t	bounds;
		bounds.uMin = image.bounds.left;
		bounds.uMax = image.bounds.right;
		bounds.vMin = image.bounds.top;
		bounds.vMax = image.bounds.bottom;

		VRVulkanTextureData_t	vk_data;
		vk_data.m_nImage			= BitCast<uint64_t>( image.handle );
		vk_data.m_pDevice			= reinterpret_cast<VkDevice_T *>( _vkLogicalDevice );
		vk_data.m_pPhysicalDevice	= reinterpret_cast<VkPhysicalDevice_T *>( _vkPhysicalDevice );
		vk_data.m_pInstance			= reinterpret_cast<VkInstance_T *>( _vkInstance );
		vk_data.m_pQueue			= reinterpret_cast<VkQueue_T *>( image.currQueue );
		vk_data.m_nQueueFamilyIndex	= uint(image.queueFamilyIndex);
		vk_data.m_nWidth			= image.dimension.x;
		vk_data.m_nHeight			= image.dimension.y;
		vk_data.m_nFormat			= BitCast<uint32_t>(image.format);
		vk_data.m_nSampleCount		= image.sampleCount;

		DEBUG_ONLY(
		switch( BitCast<VkFormat>(image.format) )
		{
			case VK_FORMAT_R8G8B8A8_UNORM :
			case VK_FORMAT_R8G8B8A8_SRGB :
			case VK_FORMAT_B8G8R8A8_UNORM :
			case VK_FORMAT_B8G8R8A8_SRGB :
			case VK_FORMAT_R32G32B32A32_SFLOAT :
			case VK_FORMAT_R32G32B32_SFLOAT :
			case VK_FORMAT_R16G16B16A16_SFLOAT :
			case VK_FORMAT_A2R10G10B10_UINT_PACK32 :
				break;	// ok
			default :
				RETURN_ERR( "unsupported image format for OpenVR" );
		})

		EVREye		vr_eye	= (eye == Eye::Left ? EVREye_Eye_Left : EVREye_Eye_Right);
		Texture_t	texture	= { &vk_data, ETextureType_TextureType_Vulkan, EColorSpace_ColorSpace_Auto };
		auto		err		= _vrCompositor->Submit( vr_eye, &texture, &bounds, EVRSubmitFlags_Submit_Default );

		Unused( err );
		//CHECK_ERR( err == EVRCompositorError_VRCompositorError_None );

		_submitted[uint(eye)] = true;

		if ( _submitted[uint(Eye::Left)] and _submitted[uint(Eye::Right)] )
		{
			_submitted[uint(Eye::Left)]  = false;
			_submitted[uint(Eye::Right)] = false;
		
			_UpdateHMDMatrixPose();
		}
		return true;
	}
	
/*
=================================================
	_UpdateHMDMatrixPose
=================================================
*/
	void  OpenVRDevice::_UpdateHMDMatrixPose ()
	{
		CHECK( _vrCompositor->WaitGetPoses( OUT _trackedDevicePose, uint(CountOf(_trackedDevicePose)), null, 0 ) == EVRCompositorError_VRCompositorError_None );

		auto&	hmd_pose = _trackedDevicePose[ k_unTrackedDeviceIndex_Hmd ];

		if ( hmd_pose.bPoseIsValid )
		{
			auto&	mat		= hmd_pose.mDeviceToAbsoluteTracking;
			auto&	vel		= hmd_pose.vVelocity;
			auto&	avel	= hmd_pose.vAngularVelocity;

			_camera.pose			= OpenVRMatToMat4( mat ).Inverse();
			_camera.position		= { mat.m[0][3], mat.m[1][3], mat.m[2][3] };
			_camera.velocity		= { vel.v[0], vel.v[1], vel.v[2] };
			_camera.angularVelocity	= { avel.v[0], avel.v[1], avel.v[2] };
		}
		else
		{
			_camera.velocity		= float3{0.0f};
			_camera.angularVelocity	= float3{0.0f};
		}
		
		for (auto& [idx, cont] : _controllers)
		{
			auto&	cont_pose	= _trackedDevicePose[ idx ];
			auto&	data		= _vrControllers( cont.id );

			if ( not cont_pose.bPoseIsValid )
			{
				data.isValid = false;
				continue;
			}
			
			auto&	mat		= cont_pose.mDeviceToAbsoluteTracking;
			auto&	vel		= cont_pose.vVelocity;
			auto&	avel	= cont_pose.vAngularVelocity;

			data.pose			 = OpenVRMatToMat3( mat ).Inverse();
			data.position		 = { mat.m[0][3], mat.m[1][3], mat.m[2][3] };
			data.velocity		 = { vel.v[0], vel.v[1], vel.v[2] };
			data.angularVelocity = { avel.v[0], avel.v[1], avel.v[2] };
			data.isValid		 = true;
		}
	}

/*
=================================================
	GetRequiredInstanceExtensions
=================================================
*/
	Array<String>  OpenVRDevice::GetRequiredInstanceExtensions () const
	{
		CHECK_ERR( _vrCompositor );
		
		Array<String>	result;
		uint			count;
		String			str;
		String			extensions;
		
		count = _vrCompositor->GetVulkanInstanceExtensionsRequired( null, 0 );
		if ( count == 0 )
			return result;

		extensions.resize( count );
		_vrCompositor->GetVulkanInstanceExtensionsRequired( OUT extensions.data(), count );

		for (uint i = 0; extensions[i] != 0 and i < count; ++i)
		{
			if ( extensions[i] == ' ' )
			{
				result.push_back( str );
				str.clear();
			}
			else
				str += extensions[i];
		}

		if ( str.size() )
			result.push_back( str );

		return result;
	}

/*
=================================================
	GetRequiredDeviceExtensions
=================================================
*/
	Array<String>  OpenVRDevice::GetRequiredDeviceExtensions (InstanceVk_t instance) const
	{
		CHECK_ERR( _hmd and _vrSystem and _vrCompositor );
		
		uint64_t	hmd_physical_device = 0;
		_vrSystem->GetOutputDevice( OUT &hmd_physical_device, ETextureType_TextureType_Vulkan, BitCast<VkInstance>(instance) );

		Array<String>	result;
		uint			count;
		String			str;
		String			extensions;
		
		count = _vrCompositor->GetVulkanDeviceExtensionsRequired( (VkPhysicalDevice_T *)hmd_physical_device, null, 0 );
		if ( count == 0 )
			return result;

		extensions.resize( count );
		_vrCompositor->GetVulkanDeviceExtensionsRequired( (VkPhysicalDevice_T *)hmd_physical_device, OUT extensions.data(), count );

		for (uint i = 0; extensions[i] != 0 and i < count; ++i)
		{
			if ( extensions[i] == ' ' )
			{
				result.push_back( str );
				str.clear();
			}
			else
				str += extensions[i];
		}

		if ( str.size() )
			result.push_back( str );

		return result;
	}
		
/*
=================================================
	GetRequiredExtensions
=================================================
*/
	uint2  OpenVRDevice::GetRenderTargetDimension () const
	{
		CHECK_ERR( _vrSystem );

		uint2	result;
		_vrSystem->GetRecommendedRenderTargetSize( OUT &result.x, OUT &result.y );
		return result;
	}

/*
=================================================
	SetupCamera
=================================================
*/
	void  OpenVRDevice::SetupCamera (const float2 &clipPlanes)
	{
		CHECK_ERRV( _vrSystem );

		_camera.clipPlanes	= clipPlanes;
		_camera.left.proj	= OpenVRMatToMat4( _vrSystem->GetProjectionMatrix( EVREye_Eye_Left, _camera.clipPlanes[0], _camera.clipPlanes[1] ));
		_camera.right.proj	= OpenVRMatToMat4( _vrSystem->GetProjectionMatrix( EVREye_Eye_Right, _camera.clipPlanes[0], _camera.clipPlanes[1] ));
		_camera.left.view	= OpenVRMatToMat4( _vrSystem->GetEyeToHeadTransform( EVREye_Eye_Left )).Inverse();
		_camera.right.view	= OpenVRMatToMat4( _vrSystem->GetEyeToHeadTransform( EVREye_Eye_Right )).Inverse();
	}
	
/*
=================================================
	GetHmdStatus
=================================================
*/
	IVRDevice::EHmdStatus  OpenVRDevice::GetHmdStatus () const
	{
		return _hmdStatus;
	}
	
/*
=================================================
	_LoadLib
=================================================
*/
	bool OpenVRDevice::_LoadLib ()
	{
	#ifdef PLATFORM_WINDOWS
		_openVRLib = ::LoadLibraryA( "openvr_api.dll" );
		
		const auto	Load =	[lib = _openVRLib] (OUT auto& outResult, const char *procName) -> bool
							{
								using FN = std::remove_reference_t< decltype(outResult) >;
								outResult = BitCast<FN>( ::GetProcAddress( BitCast<HMODULE>(lib), procName ));
								return outResult != null;
							};
	#else
		_openVRLib = ::dlopen( "libopenvr_api.so", RTLD_LAZY | RTLD_LOCAL );
		
		const auto	Load =	[lib = _openVRLib] (OUT auto& outResult, const char *procName) -> bool
							{
								using FN = std::remove_reference_t< decltype(outResult) >;
								outResult = BitCast<FN>( ::dlsym( lib, procName ));
								return outResult != null;
							};
	#endif
	#define VR_LOAD( _name_ )	res &= Load( _ovr._name_, "VR_" #_name_ )
			
		if ( _openVRLib == null )
			return false;
		
		bool	res = true;
		res &= VR_LOAD( InitInternal );
		res &= VR_LOAD( ShutdownInternal );
		res &= VR_LOAD( IsHmdPresent );
		res &= VR_LOAD( GetGenericInterface );
		res &= VR_LOAD( IsRuntimeInstalled );
		res &= VR_LOAD( GetVRInitErrorAsSymbol );
		res &= VR_LOAD( GetVRInitErrorAsEnglishDescription );
		return res;
		
	#undef VR_LOAD
	}
	
/*
=================================================
	_UnloadLib
=================================================
*/
	void OpenVRDevice::_UnloadLib ()
	{
		if ( _openVRLib == null )
			return;

	#ifdef PLATFORM_WINDOWS
		::FreeLibrary( BitCast<HMODULE>(_openVRLib) );
	#else
		::dlclose( _openVRLib );
	#endif

		_openVRLib = null;
	}


}	// FGC

#endif	// FG_ENABLE_OPENVR and FG_ENABLE_VULKAN
