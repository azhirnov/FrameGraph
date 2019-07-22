// Copyright (c) 2018-2019,  Zhirnov Andrey. For more information see 'LICENSE'

#ifdef FG_ENABLE_OPENVR

#include "framework/VR/OpenVRDevice.h"
#include "stl/Algorithms/StringUtils.h"

namespace FGC
{

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
	static String GetTrackedDeviceString (Ptr<vr::IVRSystem> pHmd, vr::TrackedDeviceIndex_t unDevice, vr::TrackedDeviceProperty prop, vr::TrackedPropertyError *peError = null)
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
	bool OpenVRDevice::Create ()
	{
		vr::EVRInitError err = vr::VRInitError_None;
		_hmd = vr::VR_Init( OUT &err, vr::VRApplication_Scene );

		if ( err != vr::VRInitError_None )
			RETURN_ERR( "VR_Init error: "s << vr::VR_GetVRInitErrorAsEnglishDescription( err ));
		
		_renderModels = Cast<vr::IVRRenderModels>(vr::VR_GetGenericInterface( vr::IVRRenderModels_Version, OUT &err ));
		if ( !_renderModels )
			RETURN_ERR( "VR_GetGenericInterface error: "s << vr::VR_GetVRInitErrorAsEnglishDescription( err ));
		
		FG_LOGI( "driver:  "s << GetTrackedDeviceString( _hmd, vr::k_unTrackedDeviceIndex_Hmd, vr::Prop_TrackingSystemName_String ) <<
				 "display: " << GetTrackedDeviceString( _hmd, vr::k_unTrackedDeviceIndex_Hmd, vr::Prop_SerialNumber_String ) );
		
		CHECK_ERR( vr::VRCompositor() );

		return true;
	}
	
/*
=================================================
	SetVKDevice
=================================================
*/
	bool OpenVRDevice::SetVKDevice (VkInstance instance, VkPhysicalDevice physicalDevice, VkDevice logicalDevice)
	{
		// check is vulkan device supports output to VR device
		{
			uint64_t	hmd_physical_device = 0;
			_hmd->GetOutputDevice( OUT &hmd_physical_device, vr::TextureType_Vulkan, (VkInstance_T *)instance );

			CHECK_ERR( VkPhysicalDevice(hmd_physical_device) == physicalDevice );
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
	void OpenVRDevice::Destroy ()
	{
		_vkInstance			= VK_NULL_HANDLE;
		_vkPhysicalDevice	= VK_NULL_HANDLE;
		_vkLogicalDevice	= VK_NULL_HANDLE;

		_renderModels		= null;
		_enabled			= false;

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
	void OpenVRDevice::AddListener (IVRDeviceEventListener *listener)
	{
		ASSERT( listener );
		_listeners.insert( listener );
	}
	
/*
=================================================
	RemoveListener
=================================================
*/
	void OpenVRDevice::RemoveListener (IVRDeviceEventListener *listener)
	{
		ASSERT( listener );
		_listeners.erase( listener );
	}

/*
=================================================
	Update
=================================================
*/
	bool OpenVRDevice::Update ()
	{
		if ( not _hmd or not vr::VRCompositor() )
			return false;

		vr::VREvent_t	ev;
		while ( _hmd->PollNextEvent( OUT &ev, sizeof(ev) ))
		{
			switch( ev.eventType )
			{
				case vr::VREvent_TrackedDeviceActivated :
					FG_LOGI( "TrackedDeviceActivated" );
					_enabled = true;
					for (auto& listener : _listeners) {
						listener->DeviceActivated();
					}
					break;

				case vr::VREvent_TrackedDeviceDeactivated :
					FG_LOGI( "TrackedDeviceDeactivated" );
					_enabled = false;
					for (auto& listener : _listeners) {
						listener->DeviceActivated();
					}
					break;
					
				case vr::VREvent_TrackedDeviceUserInteractionStarted :
					FG_LOGI( "TrackedDeviceUserInteractionStarted" );
					break;

				case vr::VREvent_TrackedDeviceUserInteractionEnded :
					FG_LOGI( "TrackedDeviceUserInteractionEnded" );
					break;

				case vr::VREvent_EnterStandbyMode :
					FG_LOGI( "EnterStandbyMode" );
					break;

				case vr::VREvent_LeaveStandbyMode :
					FG_LOGI( "LeaveStandbyMode" );
					break;

				case vr::VREvent_LensDistortionChanged :
					FG_LOGI( "LensDistortionChanged" );
					break;

				case vr::VREvent_PropertyChanged :
					FG_LOGI( "PropertyChanged" );
					break;

				case vr::VREvent_TrackedDeviceUpdated :
					FG_LOGI( "TrackedDeviceUpdated" );
					break;

				case vr::VREvent_ButtonPress :
					FG_LOGI( "ButtonPress: " + ToString(ev.data.controller.button) );
					break;
				case vr::VREvent_ButtonUnpress :
					FG_LOGI( "ButtonUnpress: " + ToString(ev.data.controller.button) );
					break;
				case vr::VREvent_ButtonTouch :
					FG_LOGI( "ButtonTouch: " + ToString(ev.data.controller.button) );
					break;
				case vr::VREvent_ButtonUntouch :
					FG_LOGI( "ButtonUntouch: " + ToString(ev.data.controller.button) );
					break;

				case vr::VREvent_MouseMove :
					FG_LOGI( "MouseMove: " + ToString(ev.data.mouse.button) );
					break;
				case vr::VREvent_MouseButtonDown :
					FG_LOGI( "MouseButtonDown: " + ToString(ev.data.mouse.button) );
					break;
				case vr::VREvent_MouseButtonUp :
					FG_LOGI( "MouseButtonUp: " + ToString(ev.data.mouse.button) );
					break;
				case vr::VREvent_TouchPadMove :
					FG_LOGI( "TouchPadMove: " + ToString(ev.data.mouse.button) );
					break;

				case vr::VREvent_Quit :
					break;
			}
		}

		for (vr::TrackedDeviceIndex_t i = 0; i < vr::k_unMaxTrackedDeviceCount; ++i)
		{
			vr::VRControllerState_t	state;
			if ( _hmd->GetControllerState( i, OUT &state, sizeof(state) ))
			{
				//m_rbShowTrackedDevice[i] = state.ulButtonPressed == 0;
			}
		}

		return true;
	}
	
/*
=================================================
	Submit
----
	Image must be in 'VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL' layout and
	must be created with usage flags 'VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_SAMPLED_BIT'
=================================================
*/
	bool OpenVRDevice::Submit (const VRImage &image, Eye eye)
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
			
		for (int i = 0; i < vr::k_unMaxTrackedDeviceCount; ++i)
		{
			if ( _trackedDevicePose[i].bPoseIsValid )
			{
				_devicePose[i] = OpenVRMatToMat4( _trackedDevicePose[i].mDeviceToAbsoluteTracking );
			}
		}

		if ( _trackedDevicePose[vr::k_unTrackedDeviceIndex_Hmd].bPoseIsValid )
		{
			_camera.pose	 = Mat3_t{ _devicePose[vr::k_unTrackedDeviceIndex_Hmd] }.Inverse();
			_camera.position = _devicePose[vr::k_unTrackedDeviceIndex_Hmd][3].xyz();
		}
	}
	
/*
=================================================
	GetCamera
=================================================
*/
	void  OpenVRDevice::GetCamera (OUT VRCamera &camera) const
	{
		camera = _camera;
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
	void OpenVRDevice::SetupCamera (const float2 &clipPlanes)
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
	IsEnabled
=================================================
*/
	bool OpenVRDevice::IsEnabled () const
	{
		return _enabled;
	}


}	// FGC

#endif	// FG_ENABLE_OPENVR
