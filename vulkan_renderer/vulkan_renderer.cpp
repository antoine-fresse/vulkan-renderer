#include "vulkan_renderer.h"

#include <assert.h>
#include <algorithm>
#include <sstream>
#include <iostream>

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#endif

PFN_vkCreateDebugReportCallbackEXT fvkCreateDebugReportCallbackEXT = nullptr;
PFN_vkDestroyDebugReportCallbackEXT fvkDestroyDebugReportCallbackEXT = nullptr;

VKAPI_ATTR VkBool32 VKAPI_CALL debug_report_callback_fn(
	VkDebugReportFlagsEXT flags,
	VkDebugReportObjectTypeEXT obj_type,
	uint64_t src_obj,
	size_t location,
	int32_t msg_code,
	const char* layer_prefix,
	const char* msg,
	void* user_data)
{
	std::ostringstream stream;

	if (flags & VK_DEBUG_REPORT_INFORMATION_BIT_EXT)
		stream << "INFO";
	else if (flags & VK_DEBUG_REPORT_WARNING_BIT_EXT)
		stream << "WARNING";
	else if (flags & VK_DEBUG_REPORT_PERFORMANCE_WARNING_BIT_EXT)
		stream << "PERFORMANCE_WARNING";
	else if (flags & VK_DEBUG_REPORT_ERROR_BIT_EXT)
		stream << "ERROR";
	else if (flags & VK_DEBUG_REPORT_DEBUG_BIT_EXT)
		stream << "DEBUG";

	stream << " (@" << layer_prefix << "): " << msg << "\n";

	std::cout << stream.str();

#ifdef _WIN32
	if (flags & VK_DEBUG_REPORT_ERROR_BIT_EXT)
		MessageBox(nullptr, stream.str().c_str(), "Vulkan Error!", 0);
#endif
	return false;
}

vulkan_renderer::vulkan_renderer(bool debug_layers) : _debug(debug_layers)
{
	if(_debug)
		setup_debug();

	init_instance();
	
	if (_debug)
		init_debug();

	init_device();

	init_command_buffers();
}


vulkan_renderer::~vulkan_renderer()
{
	if(_command_pool)
		vkDestroyCommandPool(_device, _command_pool, nullptr);

	vkDestroyDevice(_device, nullptr);

	if (_debug)
	{
		fvkDestroyDebugReportCallbackEXT(_instance, _debug_report_callback, nullptr);
	}

	vkDestroyInstance(_instance, nullptr);
	
}



void vulkan_renderer::init_instance()
{
	VkApplicationInfo app_info = {};
	app_info.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
	app_info.pNext = nullptr;
	app_info.pApplicationName = "vulkan test";
	app_info.applicationVersion = 1;
	app_info.pEngineName = "vulkan test";
	app_info.engineVersion = 1;
	app_info.apiVersion = VK_API_VERSION_1_0;

	VkInstanceCreateInfo inst_info = {};
	inst_info.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
	inst_info.pApplicationInfo = &app_info;

	inst_info.enabledLayerCount = _instance_layers.size();
	inst_info.ppEnabledLayerNames = _instance_layers.size() ? _instance_layers.data() : nullptr;
	inst_info.enabledExtensionCount = _instance_extensions.size();
	inst_info.ppEnabledExtensionNames = _instance_extensions.size() ? _instance_extensions.data() : nullptr;
	inst_info.pNext = &_debug_report_callback_create_info;

	auto res = vkCreateInstance(&inst_info, nullptr, &_instance);
	assert(res == VK_SUCCESS);
}

void vulkan_renderer::init_device()
{
	VkResult err = VK_SUCCESS;
	uint32_t device_count = 0;
	vkEnumeratePhysicalDevices(_instance, &device_count, nullptr);
	assert(err == VK_SUCCESS);
	std::vector<VkPhysicalDevice> physical_devices(device_count);
	vkEnumeratePhysicalDevices(_instance, &device_count, physical_devices.data());
	_gpu = physical_devices[0];


	uint32_t family_count = 0;
	vkGetPhysicalDeviceQueueFamilyProperties(_gpu, &family_count, nullptr);
	std::vector<VkQueueFamilyProperties> family_properties(device_count);
	vkGetPhysicalDeviceQueueFamilyProperties(_gpu, &family_count, family_properties.data());
	
	auto graphics_queue = std::find_if(family_properties.begin(), family_properties.end(), [](const VkQueueFamilyProperties& family)->bool { return family.queueFlags & VK_QUEUE_GRAPHICS_BIT; });
	assert(graphics_queue != family_properties.end());

	//if(_debug)
	{
		uint32_t layer_count = 0;
		vkEnumerateInstanceLayerProperties(&layer_count, nullptr);
		std::vector<VkLayerProperties> layers(layer_count);
		vkEnumerateInstanceLayerProperties(&layer_count, layers.data());

		printf("Instance Layers :\n");
		for (auto layer : layers)
		{
			printf("%s : %s\n", layer.layerName, layer.description);
		}
	}

	VkDeviceQueueCreateInfo device_queue_create_info = {};

	device_queue_create_info.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
	
	_graphics_family_index = distance(family_properties.begin(), graphics_queue);
	device_queue_create_info.queueFamilyIndex = _graphics_family_index;
	device_queue_create_info.queueCount = 1;
	float queue_priorities[] = { 1.0f };
	device_queue_create_info.pQueuePriorities = queue_priorities;

	VkDeviceCreateInfo device_create_info = {};

	device_create_info.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
	device_create_info.queueCreateInfoCount = 1;
	device_create_info.pQueueCreateInfos = &device_queue_create_info;

	device_create_info.enabledLayerCount = _device_layers.size();
	device_create_info.ppEnabledLayerNames = _device_layers.size() ? _device_layers.data() : nullptr;
	device_create_info.enabledExtensionCount = _device_extensions.size();
	device_create_info.ppEnabledExtensionNames = _device_extensions.size() ? _device_extensions.data() : nullptr;

	err = vkCreateDevice(_gpu, &device_create_info, nullptr, &_device);
	assert(err == VK_SUCCESS);

	//if (_debug)
	{
		uint32_t layer_count = 0;
		vkEnumerateDeviceLayerProperties(_gpu, &layer_count, nullptr);
		std::vector<VkLayerProperties> layers(layer_count);
		vkEnumerateDeviceLayerProperties(_gpu, &layer_count, layers.data());

		printf("Device Layers :\n");
		for (auto layer : layers)
		{
			printf("%s : %s\n", layer.layerName, layer.description);
		}
	}

	vkGetDeviceQueue(_device, _graphics_family_index, 0, &_queue);
}

void vulkan_renderer::setup_debug()
{
	_debug_report_callback_create_info.sType = VK_STRUCTURE_TYPE_DEBUG_REPORT_CALLBACK_CREATE_INFO_EXT;
	_debug_report_callback_create_info.flags = 
		// VK_DEBUG_REPORT_INFORMATION_BIT_EXT |
		VK_DEBUG_REPORT_WARNING_BIT_EXT |
		VK_DEBUG_REPORT_PERFORMANCE_WARNING_BIT_EXT |
		VK_DEBUG_REPORT_ERROR_BIT_EXT |
		// VK_DEBUG_REPORT_DEBUG_BIT_EXT |
		0;
	_debug_report_callback_create_info.pfnCallback = debug_report_callback_fn;

	_instance_layers.push_back("VK_LAYER_LUNARG_standard_validation");
	
	// _instance_layers.push_back("VK_LAYER_GOOGLE_threading");
	// _instance_layers.push_back("VK_LAYER_LUNARG_image");
	// _instance_layers.push_back("VK_LAYER_LUNARG_core_validation");
	// _instance_layers.push_back("VK_LAYER_LUNARG_object_tracker");
	// _instance_layers.push_back("VK_LAYER_LUNARG_parameter_validation");



	_instance_extensions.push_back(VK_EXT_DEBUG_REPORT_EXTENSION_NAME);

	_device_layers.push_back("VK_LAYER_LUNARG_standard_validation");

	// _device_layers.push_back("VK_LAYER_GOOGLE_threading");
	// _device_layers.push_back("VK_LAYER_LUNARG_image");
	// _device_layers.push_back("VK_LAYER_LUNARG_core_validation");
	// _device_layers.push_back("VK_LAYER_LUNARG_object_tracker");
	// _device_layers.push_back("VK_LAYER_LUNARG_parameter_validation");
}

void vulkan_renderer::init_debug()
{
	fvkCreateDebugReportCallbackEXT = (PFN_vkCreateDebugReportCallbackEXT)vkGetInstanceProcAddr(_instance, "vkCreateDebugReportCallbackEXT");
	fvkDestroyDebugReportCallbackEXT = (PFN_vkDestroyDebugReportCallbackEXT)vkGetInstanceProcAddr(_instance, "vkDestroyDebugReportCallbackEXT");
	assert(fvkCreateDebugReportCallbackEXT && fvkDestroyDebugReportCallbackEXT);

	fvkCreateDebugReportCallbackEXT(_instance, &_debug_report_callback_create_info, nullptr, &_debug_report_callback);
}

void vulkan_renderer::init_command_buffers()
{
	VkCommandPoolCreateInfo command_pool_ci = {};
	command_pool_ci.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
	command_pool_ci.queueFamilyIndex = _graphics_family_index;
	command_pool_ci.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
	
	vkCreateCommandPool(_device, &command_pool_ci, nullptr, &_command_pool);

	VkCommandBufferAllocateInfo cmd_buffer_alloc_ci = {};
	cmd_buffer_alloc_ci.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	cmd_buffer_alloc_ci.commandPool = _command_pool;
	cmd_buffer_alloc_ci.commandBufferCount = 1;
	cmd_buffer_alloc_ci.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;

	vkAllocateCommandBuffers(_device, &cmd_buffer_alloc_ci, &_command_buffer);
}
