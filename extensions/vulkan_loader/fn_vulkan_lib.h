
#ifdef VKLOADER_STAGE_DECLFNPOINTER
	extern PFN_vkEnumerateInstanceLayerProperties  _var_vkEnumerateInstanceLayerProperties;
	extern PFN_vkGetInstanceProcAddr  _var_vkGetInstanceProcAddr;
	extern PFN_vkCreateInstance  _var_vkCreateInstance;
	extern PFN_vkDebugUtilsMessengerCallbackEXT  _var_vkDebugUtilsMessengerCallbackEXT;
	extern PFN_vkDebugReportCallbackEXT  _var_vkDebugReportCallbackEXT;
	extern PFN_vkEnumerateInstanceVersion  _var_vkEnumerateInstanceVersion;
	extern PFN_vkEnumerateInstanceExtensionProperties  _var_vkEnumerateInstanceExtensionProperties;
#endif // VKLOADER_STAGE_DECLFNPOINTER


#ifdef VKLOADER_STAGE_FNPOINTER
	PFN_vkEnumerateInstanceLayerProperties  _var_vkEnumerateInstanceLayerProperties = null;
	PFN_vkGetInstanceProcAddr  _var_vkGetInstanceProcAddr = null;
	PFN_vkCreateInstance  _var_vkCreateInstance = null;
	PFN_vkDebugUtilsMessengerCallbackEXT  _var_vkDebugUtilsMessengerCallbackEXT = null;
	PFN_vkDebugReportCallbackEXT  _var_vkDebugReportCallbackEXT = null;
	PFN_vkEnumerateInstanceVersion  _var_vkEnumerateInstanceVersion = null;
	PFN_vkEnumerateInstanceExtensionProperties  _var_vkEnumerateInstanceExtensionProperties = null;
#endif // VKLOADER_STAGE_FNPOINTER


#ifdef VKLOADER_STAGE_INLINEFN
	ND_ forceinline VkResult vkEnumerateInstanceLayerProperties (uint32_t * pPropertyCount, VkLayerProperties * pProperties)								{ return _var_vkEnumerateInstanceLayerProperties( pPropertyCount, pProperties ); }
	ND_ forceinline PFN_vkVoidFunction vkGetInstanceProcAddr (VkInstance instance, const char * pName)								{ return _var_vkGetInstanceProcAddr( instance, pName ); }
	ND_ forceinline VkResult vkCreateInstance (const VkInstanceCreateInfo * pCreateInfo, const VkAllocationCallbacks * pAllocator, VkInstance * pInstance)								{ return _var_vkCreateInstance( pCreateInfo, pAllocator, pInstance ); }
	ND_ forceinline VkBool32 vkDebugUtilsMessengerCallbackEXT (VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity, VkDebugUtilsMessageTypeFlagsEXT messageTypes, const VkDebugUtilsMessengerCallbackDataEXT * pCallbackData, void * pUserData)								{ return _var_vkDebugUtilsMessengerCallbackEXT( messageSeverity, messageTypes, pCallbackData, pUserData ); }
	ND_ forceinline VkBool32 vkDebugReportCallbackEXT (VkDebugReportFlagsEXT flags, VkDebugReportObjectTypeEXT objectType, uint64_t object, size_t location, int32_t messageCode, const char * pLayerPrefix, const char * pMessage, void * pUserData)								{ return _var_vkDebugReportCallbackEXT( flags, objectType, object, location, messageCode, pLayerPrefix, pMessage, pUserData ); }
	ND_ forceinline VkResult vkEnumerateInstanceVersion (uint32_t * pApiVersion)								{ return _var_vkEnumerateInstanceVersion( pApiVersion ); }
	ND_ forceinline VkResult vkEnumerateInstanceExtensionProperties (const char * pLayerName, uint32_t * pPropertyCount, VkExtensionProperties * pProperties)								{ return _var_vkEnumerateInstanceExtensionProperties( pLayerName, pPropertyCount, pProperties ); }
#endif // VKLOADER_STAGE_INLINEFN


#ifdef VKLOADER_STAGE_DUMMYFN
	VKAPI_ATTR VkResult VKAPI_CALL Dummy_vkEnumerateInstanceLayerProperties (uint32_t * , VkLayerProperties * )			{  FG_LOGI( "used dummy function 'vkEnumerateInstanceLayerProperties'" );  return VkResult(~0u);  }
	VKAPI_ATTR PFN_vkVoidFunction VKAPI_CALL Dummy_vkGetInstanceProcAddr (VkInstance , const char * )			{  FG_LOGI( "used dummy function 'vkGetInstanceProcAddr'" );  return null;  }
	VKAPI_ATTR VkResult VKAPI_CALL Dummy_vkCreateInstance (const VkInstanceCreateInfo * , const VkAllocationCallbacks * , VkInstance * )			{  FG_LOGI( "used dummy function 'vkCreateInstance'" );  return VkResult(~0u);  }
	VKAPI_ATTR VkBool32 VKAPI_CALL Dummy_vkDebugUtilsMessengerCallbackEXT (VkDebugUtilsMessageSeverityFlagBitsEXT , VkDebugUtilsMessageTypeFlagsEXT , const VkDebugUtilsMessengerCallbackDataEXT * , void * )			{  FG_LOGI( "used dummy function 'vkDebugUtilsMessengerCallbackEXT'" );  return VkBool32(0);  }
	VKAPI_ATTR VkBool32 VKAPI_CALL Dummy_vkDebugReportCallbackEXT (VkDebugReportFlagsEXT , VkDebugReportObjectTypeEXT , uint64_t , size_t , int32_t , const char * , const char * , void * )			{  FG_LOGI( "used dummy function 'vkDebugReportCallbackEXT'" );  return VkBool32(0);  }
	VKAPI_ATTR VkResult VKAPI_CALL Dummy_vkEnumerateInstanceVersion (uint32_t * )			{  FG_LOGI( "used dummy function 'vkEnumerateInstanceVersion'" );  return VkResult(~0u);  }
	VKAPI_ATTR VkResult VKAPI_CALL Dummy_vkEnumerateInstanceExtensionProperties (const char * , uint32_t * , VkExtensionProperties * )			{  FG_LOGI( "used dummy function 'vkEnumerateInstanceExtensionProperties'" );  return VkResult(~0u);  }
#endif // VKLOADER_STAGE_DUMMYFN


#ifdef VKLOADER_STAGE_GETADDRESS
	Load( OUT _var_vkEnumerateInstanceLayerProperties, "vkEnumerateInstanceLayerProperties", Dummy_vkEnumerateInstanceLayerProperties );
	Load( OUT _var_vkGetInstanceProcAddr, "vkGetInstanceProcAddr", Dummy_vkGetInstanceProcAddr );
	Load( OUT _var_vkCreateInstance, "vkCreateInstance", Dummy_vkCreateInstance );
	Load( OUT _var_vkDebugUtilsMessengerCallbackEXT, "vkDebugUtilsMessengerCallbackEXT", Dummy_vkDebugUtilsMessengerCallbackEXT );
	Load( OUT _var_vkDebugReportCallbackEXT, "vkDebugReportCallbackEXT", Dummy_vkDebugReportCallbackEXT );
	Load( OUT _var_vkEnumerateInstanceVersion, "vkEnumerateInstanceVersion", Dummy_vkEnumerateInstanceVersion );
	Load( OUT _var_vkEnumerateInstanceExtensionProperties, "vkEnumerateInstanceExtensionProperties", Dummy_vkEnumerateInstanceExtensionProperties );
#endif // VKLOADER_STAGE_GETADDRESS

