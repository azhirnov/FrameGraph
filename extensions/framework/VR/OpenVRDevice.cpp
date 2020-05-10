// Copyright (c) 2018-2020,  Zhirnov Andrey. For more information see 'LICENSE'

#ifdef FG_ENABLE_OPENVR

#include "framework/VR/OpenVRDevice.h"
#include "stl/Algorithms/StringUtils.h"

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
	STATIC_ASSERT( CountOf(AxisNames) == vr::k_unControllerStateAxisCount );

/*
=================================================
	OpenVRMatToMat4
=================================================
*/
	ND_ forceinline IVRDevice::Mat4_t  OpenVRMatToMat4 (const vr::HmdMatrix44_t &mat)
	{
		return IVRDevice::Mat4_t{
				mat.m[0][0], mat.m[1][0], mat.m[2][0], mat.m[3][0],
				mat.m[0][1], mat.m[1][1], mat.m[2][1], mat.m[3][1], 
				mat.m[0][2], mat.m[1][2], mat.m[2][2], mat.m[3][2], 
				mat.m[0][3], mat.m[1][3], mat.m[2][3], mat.m[3][3] };
	}
	
	ND_ forceinline IVRDevice::Mat4_t  OpenVRMatToMat4 (const vr::HmdMatrix34_t &mat)
	{
		return IVRDevice::Mat4_t{
				mat.m[0][0], mat.m[1][0], mat.m[2][0], 0.0f,
				mat.m[0][1], mat.m[1][1], mat.m[2][1], 0.0f,
				mat.m[0][2], mat.m[1][2], mat.m[2][2], 0.0f,
				mat.m[0][3], mat.m[1][3], mat.m[2][3], 1.0f };
	}

	ND_ forceinline IVRDevice::Mat3_t  OpenVRMatToMat3 (const vr::HmdMatrix34_t &mat)
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
	GetTrackedDeviceString
=================================================
*/
	ND_ static String  GetTrackedDeviceString (Ptr<vr::IVRSystem> pHmd, vr::TrackedDeviceIndex_t unDevice, vr::TrackedDeviceProperty prop, vr::TrackedPropertyError *peError = null)
	{
		uint32_t unRequiredBufferLen = pHmd->GetStringTrackedDeviceProperty( unDevice, prop, null, 0, peError );
		if( unRequiredBufferLen == 0 )
			return "";

		char *pchBuffer = new char[ unRequiredBufferLen ];
		unRequiredBufferLen = pHmd->GetStringTrackedDeviceProperty( unDevice, prop, pchBuffer, unRequiredBufferLen, peError );
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
		vr::EVRInitError err = vr::VRInitError_None;
		_hmd = vr::VR_Init( OUT &err, vr::VRApplication_Scene );

		if ( err != vr::VRInitError_None )
			RETURN_ERR( "VR_Init error: "s << vr::VR_GetVRInitErrorAsEnglishDescription( err ));
		
		//_renderModels = Cast<vr::IVRRenderModels>(vr::VR_GetGenericInterface( vr::IVRRenderModels_Version, OUT &err ));
		//if ( !_renderModels )
		//	RETURN_ERR( "VR_GetGenericInterface error: "s << vr::VR_GetVRInitErrorAsEnglishDescription( err ));
		
		FG_LOGI( "driver:  "s << GetTrackedDeviceString( _hmd, vr::k_unTrackedDeviceIndex_Hmd, vr::Prop_TrackingSystemName_String ) <<
				 "display: " << GetTrackedDeviceString( _hmd, vr::k_unTrackedDeviceIndex_Hmd, vr::Prop_SerialNumber_String ) );
		
		CHECK_ERR( vr::VRCompositor() );

		vr::VRCompositor()->SetTrackingSpace( vr::TrackingUniverseStanding );

		vr::EDeviceActivityLevel level = _hmd->GetTrackedDeviceActivityLevel( vr::k_unTrackedDeviceIndex_Hmd );
		BEGIN_ENUM_CHECKS();
		switch ( level )
		{
			case vr::k_EDeviceActivityLevel_Unknown :					_hmdStatus = EHmdStatus::PowerOff;	break;
			case vr::k_EDeviceActivityLevel_Idle :						_hmdStatus = EHmdStatus::Standby;	break;
			case vr::k_EDeviceActivityLevel_UserInteraction :			_hmdStatus = EHmdStatus::Active;	break;
			case vr::k_EDeviceActivityLevel_UserInteraction_Timeout :	_hmdStatus = EHmdStatus::Standby;	break;
			case vr::k_EDeviceActivityLevel_Standby :					_hmdStatus = EHmdStatus::Standby;	break;
			case vr::k_EDeviceActivityLevel_Idle_Timeout :				_hmdStatus = EHmdStatus::Standby;	break;
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
		for (vr::TrackedDeviceIndex_t i = 0; i < vr::k_unMaxTrackedDeviceCount; ++i)
		{
			vr::ETrackedDeviceClass	dev_class = _hmd->GetTrackedDeviceClass( i );

			if ( dev_class == vr::ETrackedDeviceClass::TrackedDeviceClass_Controller or
				 dev_class == vr::ETrackedDeviceClass::TrackedDeviceClass_GenericTracker )
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
		vr::ETrackedControllerRole	role = _hmd->GetControllerRoleForTrackedDeviceIndex( tdi );
		
		BEGIN_ENUM_CHECKS();
		switch ( role )
		{
			case vr::TrackedControllerRole_LeftHand :	return ControllerID::LeftHand;
			case vr::TrackedControllerRole_RightHand :	return ControllerID::RightHand;
			case vr::TrackedControllerRole_Invalid :
			case vr::TrackedControllerRole_OptOut :
			case vr::TrackedControllerRole_Treadmill :
			case vr::TrackedControllerRole_Max :		break;
		}
		END_ENUM_CHECKS();
		return ControllerID::Unknown;
	}

/*
=================================================
	SetVKDevice
=================================================
*/
	bool  OpenVRDevice::SetVKDevice (VkInstance instance, VkPhysicalDevice physicalDevice, VkDevice logicalDevice)
	{
		// check is vulkan device supports output to VR device
		{
			uint64_t	hmd_physical_device = 0;
			_hmd->GetOutputDevice( OUT &hmd_physical_device, vr::TextureType_Vulkan, (VkInstance_T *)instance );

			if ( hmd_physical_device != 0 )
			{
				CHECK_ERR( VkPhysicalDevice(hmd_physical_device) == physicalDevice );
			}
		}

		_vkInstance			= instance;
		_vkPhysicalDevice	= physicalDevice;
		_vkLogicalDevice	= logicalDevice;

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
			vr::VR_Shutdown();
			_hmd = null;
		}
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
		if ( not _hmd or not vr::VRCompositor() )
			return false;

		vr::VREvent_t	ev;
		while ( _hmd->PollNextEvent( OUT &ev, sizeof(ev) ))
		{
			if ( ev.trackedDeviceIndex == vr::k_unTrackedDeviceIndex_Hmd )
				_ProcessHmdEvents( ev );

			auto	iter = _controllers.find( ev.trackedDeviceIndex );
			if ( iter != _controllers.end() )
				_ProcessControllerEvents( iter->second, ev );
		}

		const auto	now = TimePoint_t::clock::now();

		for (auto& [idx, cont] : _controllers)
		{
			vr::VRControllerState_t	state;
			if ( _hmd->GetControllerState( idx, OUT &state, sizeof(state) ))
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
	void  OpenVRDevice::_ProcessHmdEvents (const vr::VREvent_t &ev)
	{
		switch( ev.eventType )
		{
			case vr::VREvent_TrackedDeviceActivated :
			case vr::VREvent_TrackedDeviceUserInteractionStarted :
				if ( _hmdStatus < EHmdStatus::Active ) {
					_hmdStatus = EHmdStatus::Active;

					for (auto& listener : _listeners) {
						listener->HmdStatusChanged( _hmdStatus );
					}
				}
				break;

			case vr::VREvent_TrackedDeviceUserInteractionEnded :
				_hmdStatus = EHmdStatus::Standby;

				for (auto& listener : _listeners) {
					listener->HmdStatusChanged( _hmdStatus );
				}
				break;

			case vr::VREvent_ButtonPress :
				if ( ev.data.controller.button == vr::k_EButton_ProximitySensor ) {
					_hmdStatus = EHmdStatus::Mounted;
					
					for (auto& listener : _listeners) {
						listener->HmdStatusChanged( _hmdStatus );
					}
				}
				break;

			case vr::VREvent_ButtonUnpress :
				if ( ev.data.controller.button == vr::k_EButton_ProximitySensor ) {
					_hmdStatus = EHmdStatus::Active;
					
					for (auto& listener : _listeners) {
						listener->HmdStatusChanged( _hmdStatus );
					}
				}
				break;

			case vr::VREvent_Quit :
				_hmdStatus = EHmdStatus::PowerOff;
				
				for (auto& listener : _listeners) {
					listener->HmdStatusChanged( _hmdStatus );
				}
				break;

			case vr::VREvent_LensDistortionChanged :
				SetupCamera( _camera.clipPlanes );
				FG_LOGI( "LensDistortionChanged" );
				break;

			case vr::VREvent_PropertyChanged :
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
			case vr::k_EButton_System :			return "system";
			case vr::k_EButton_ApplicationMenu:	return "app menu";
			case vr::k_EButton_Grip :			return "grip";
			case vr::k_EButton_DPad_Left :		return "dpad left";
			case vr::k_EButton_DPad_Up :		return "dpad up";
			case vr::k_EButton_DPad_Right :		return "dpad right";
			case vr::k_EButton_DPad_Down :		return "dpad down";
			case vr::k_EButton_A :				return "A";
			case vr::k_EButton_Axis0 :			return "Axis 0";
			case vr::k_EButton_Axis1 :			return "Axis 1";
			case vr::k_EButton_Axis2 :			return "Axis 2";
			case vr::k_EButton_Axis3 :			return "Axis 3";
			case vr::k_EButton_Axis4 :			return "Axis 4";
		}
		return "";
	}

/*
=================================================
	_ProcessControllerEvents
=================================================
*/
	void  OpenVRDevice::_ProcessControllerEvents (INOUT Controller &cont, const vr::VREvent_t &ev)
	{
		switch( ev.eventType )
		{
			case vr::VREvent_ButtonPress :
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

			case vr::VREvent_ButtonUnpress :
			{
				StringView	key		= MapOpenVRButton( ev.data.controller.button );
				cont.keys[ ev.data.controller.button ] = false;

				for (auto& listener : _listeners) {
					listener->OnButton( cont.id, key, EButtonAction::Up );
				}
				break;
			}

			case vr::VREvent_ButtonTouch :
			case vr::VREvent_ButtonUntouch :
			{
				uint	idx = ev.data.controller.button - vr::k_EButton_Axis0;
				if ( idx < cont.axis.size() ) {
					cont.axis[idx].pressed = (ev.eventType == vr::VREvent_ButtonTouch);
				}
				break;
			}

			case vr::VREvent_MouseMove :
				FG_LOGI( "MouseMove: " + ToString(ev.data.mouse.button) + ", dev: " + ToString(ev.trackedDeviceIndex) );
				break;
			case vr::VREvent_MouseButtonDown :
				FG_LOGI( "MouseButtonDown: " + ToString(ev.data.mouse.button) + ", dev: " + ToString(ev.trackedDeviceIndex) );
				break;
			case vr::VREvent_MouseButtonUp :
				FG_LOGI( "MouseButtonUp: " + ToString(ev.data.mouse.button) + ", dev: " + ToString(ev.trackedDeviceIndex) );
				break;
			case vr::VREvent_TouchPadMove :
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
		CHECK_ERR( _hmd and vr::VRCompositor() );

		vr::VRTextureBounds_t	bounds;
		bounds.uMin = image.bounds.left;
		bounds.uMax = image.bounds.right;
		bounds.vMin = image.bounds.top;
		bounds.vMax = image.bounds.bottom;

		vr::VRVulkanTextureData_t	vk_data;
		vk_data.m_nImage			= (uint64_t) image.handle;
		vk_data.m_pDevice			= (VkDevice_T *) _vkLogicalDevice;
		vk_data.m_pPhysicalDevice	= (VkPhysicalDevice_T *) _vkPhysicalDevice;
		vk_data.m_pInstance			= (VkInstance_T *) _vkInstance;
		vk_data.m_pQueue			= (VkQueue_T *) image.currQueue;
		vk_data.m_nQueueFamilyIndex	= image.queueFamilyIndex;
		vk_data.m_nWidth			= image.dimension.x;
		vk_data.m_nHeight			= image.dimension.y;
		vk_data.m_nFormat			= image.format;
		vk_data.m_nSampleCount		= image.sampleCount;

		DEBUG_ONLY(
		switch( image.format )
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

		vr::EVREye				vr_eye		= (eye == Eye::Left ? vr::Eye_Left : vr::Eye_Right);
		vr::Texture_t			texture		= { &vk_data, vr::TextureType_Vulkan, vr::ColorSpace_Auto };
		vr::EVRCompositorError	err			= vr::VRCompositor()->Submit( vr_eye, &texture, &bounds );

		//CHECK_ERR( err == vr::VRCompositorError_None );

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
		CHECK( vr::VRCompositor()->WaitGetPoses( OUT _trackedDevicePose, uint(CountOf(_trackedDevicePose)), null, 0 ) == vr::VRCompositorError_None );

		auto&	hmd_pose = _trackedDevicePose[ vr::k_unTrackedDeviceIndex_Hmd ];

		if ( hmd_pose.bPoseIsValid )
		{
			auto&	mat		= hmd_pose.mDeviceToAbsoluteTracking;
			auto&	vel		= hmd_pose.vVelocity;
			auto&	avel	= hmd_pose.vAngularVelocity;

			_camera.pose			= OpenVRMatToMat3( mat ).Inverse();
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
		CHECK_ERR( vr::VRCompositor() );
		
		Array<String>	result;
		uint			count;
		String			str;
		String			extensions;
		
		count = vr::VRCompositor()->GetVulkanInstanceExtensionsRequired( null, 0 );
		if ( count == 0 )
			return result;

		extensions.resize( count );
		vr::VRCompositor()->GetVulkanInstanceExtensionsRequired( OUT extensions.data(), count );

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
	Array<String>  OpenVRDevice::GetRequiredDeviceExtensions (VkInstance instance) const
	{
		CHECK_ERR( _hmd and vr::VRCompositor() );
		
		uint64_t	hmd_physical_device = 0;
		_hmd->GetOutputDevice( OUT &hmd_physical_device, vr::TextureType_Vulkan, (VkInstance_T *)instance );

		Array<String>	result;
		uint			count;
		String			str;
		String			extensions;
		
		count = vr::VRCompositor()->GetVulkanDeviceExtensionsRequired( (VkPhysicalDevice_T *)hmd_physical_device, null, 0 );
		if ( count == 0 )
			return result;

		extensions.resize( count );
		vr::VRCompositor()->GetVulkanDeviceExtensionsRequired( (VkPhysicalDevice_T *)hmd_physical_device, OUT extensions.data(), count );

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
		uint2	result;
		_hmd->GetRecommendedRenderTargetSize( OUT &result.x, OUT &result.y );
		return result;
	}

/*
=================================================
	SetupCamera
=================================================
*/
	void  OpenVRDevice::SetupCamera (const float2 &clipPlanes)
	{
		CHECK_ERR( _hmd, void());

		_camera.clipPlanes	= clipPlanes;
		_camera.left.proj	= OpenVRMatToMat4( _hmd->GetProjectionMatrix( vr::Eye_Left, _camera.clipPlanes[0], _camera.clipPlanes[1] ));
		_camera.right.proj	= OpenVRMatToMat4( _hmd->GetProjectionMatrix( vr::Eye_Right, _camera.clipPlanes[0], _camera.clipPlanes[1] ));
		_camera.left.view	= OpenVRMatToMat4( _hmd->GetEyeToHeadTransform( vr::Eye_Left )).Inverse();
		_camera.right.view	= OpenVRMatToMat4( _hmd->GetEyeToHeadTransform( vr::Eye_Right )).Inverse();
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


}	// FGC

#endif	// FG_ENABLE_OPENVR
