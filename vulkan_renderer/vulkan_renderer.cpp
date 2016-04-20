#include "vulkan_renderer.h"

#include "config_defines.h"
#include "platform.h"

#include <algorithm>
#include <sstream>
#include <iostream>

PFN_vkCreateDebugReportCallbackEXT fvkCreateDebugReportCallbackEXT = nullptr;
PFN_vkDestroyDebugReportCallbackEXT fvkDestroyDebugReportCallbackEXT = nullptr;



vulkan_renderer::vulkan_renderer(const std::vector<const char*>& instance_layers, const std::vector<const char*>& instance_extensions, const std::vector<const char*>& device_layers, const std::vector<const char*>& device_extensions)
: _debug_report_callback_create_info( vk::DebugReportFlagsEXT{}, nullptr, nullptr)
{
	setup_debug();

	_instance_layers.insert(_instance_layers.end(), instance_layers.begin(), instance_layers.end());
	_instance_extensions.insert(_instance_extensions.end(), instance_extensions.begin(), instance_extensions.end());
	_device_layers.insert(_device_layers.end(), device_layers.begin(), device_layers.end());
	_device_extensions.insert(_device_extensions.end(), device_extensions.begin(), device_extensions.end());

	init_instance();
	
	init_debug();

	init_device();

	init_command_buffers();
}


vulkan_renderer::~vulkan_renderer()
{
	if (_command_pool)
		_device.destroyCommandPool(_command_pool);

	
	_device.destroy();
	
	uninit_debug();
	
	_instance.destroy();
}



void vulkan_renderer::init_instance()
{
	{
		printf("Instance Layers :\n");
		for (auto layer : vk::enumerateInstanceLayerProperties())
		{
			printf("%s : %s\n", layer.layerName(), layer.description());
		}
	}
	
	vk::ApplicationInfo app_info{ "vulkan test", 1, "vulkan test", 1, VK_API_VERSION_1_0 };
	
	vk::InstanceCreateInfo ci{ vk::InstanceCreateFlags {} , &app_info, _instance_layers.size(), _instance_layers.data(), _instance_extensions.size(), _instance_extensions.data() };
	ci.pNext(&_debug_report_callback_create_info);
	
	_instance = vk::createInstance(ci);
}

void vulkan_renderer::init_device()
{
	VkResult err = VK_SUCCESS;
	
	std::vector<vk::PhysicalDevice> physical_devices = _instance.enumeratePhysicalDevices();
	_gpu = physical_devices[0];
	
	std::vector<vk::QueueFamilyProperties> family_properties = _gpu.getQueueFamilyProperties();
	
	auto graphics_queue = std::find_if(family_properties.begin(), family_properties.end(), [](const vk::QueueFamilyProperties& family)->bool { return (family.queueFlags() & vk::QueueFlagBits::eGraphics) == vk::QueueFlagBits::eGraphics; });
	
	assert(graphics_queue != family_properties.end());

	_graphics_family_index = distance(family_properties.begin(), graphics_queue);

	float queue_priorities[] = { 1.0f };
	vk::DeviceQueueCreateInfo device_queue_ci{ vk::DeviceQueueCreateFlags{},  _graphics_family_index , 1, queue_priorities };
	vk::DeviceCreateInfo device_ci{ vk::DeviceCreateFlags{}, 1, &device_queue_ci, _device_layers.size(), _device_layers.data(), _device_extensions.size(), _device_extensions.data(), nullptr };

	{
		printf("Device Layers :\n");
		for (auto layer : _gpu.enumerateDeviceLayerProperties())
		{
			printf("%s : %s\n", layer.layerName(), layer.description());
		}
	}

	_device = _gpu.createDevice(device_ci);

	_queue = _device.getQueue(_graphics_family_index, 0);
}


#if USE_VULKAN_DEBUG_LAYERS


inline VKAPI_ATTR VkBool32 VKAPI_CALL debug_report_callback_fn(
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

void vulkan_renderer::setup_debug()
{

	_debug_report_callback_create_info.flags(
		//  vk::DebugReportFlagBitsEXT::eInformation |
		vk::DebugReportFlagBitsEXT::eWarning
		| vk::DebugReportFlagBitsEXT::ePerformanceWarning
		| vk::DebugReportFlagBitsEXT::eError
		//  | vk::DebugReportFlagBitsEXT::eDebug 
	);

	_debug_report_callback_create_info.pfnCallback(debug_report_callback_fn);

	_instance_layers.push_back("VK_LAYER_LUNARG_standard_validation");
	_instance_layers.push_back("VK_LAYER_RENDERDOC_Capture");

	_instance_extensions.push_back(VK_EXT_DEBUG_REPORT_EXTENSION_NAME);

	_device_layers.push_back("VK_LAYER_LUNARG_standard_validation");
	_device_layers.push_back("VK_LAYER_RENDERDOC_Capture");

}

void vulkan_renderer::init_debug()
{
	fvkCreateDebugReportCallbackEXT = (PFN_vkCreateDebugReportCallbackEXT)vkGetInstanceProcAddr((VkInstance)_instance, "vkCreateDebugReportCallbackEXT");
	fvkDestroyDebugReportCallbackEXT = (PFN_vkDestroyDebugReportCallbackEXT)vkGetInstanceProcAddr((VkInstance)_instance, "vkDestroyDebugReportCallbackEXT");
	// assert(fvkCreateDebugReportCallbackEXT && fvkDestroyDebugReportCallbackEXT);

	VkDebugReportCallbackEXT dbg;
	fvkCreateDebugReportCallbackEXT((VkInstance)_instance, (VkDebugReportCallbackCreateInfoEXT*)&_debug_report_callback_create_info, nullptr, &dbg);
	_debug_report_callback = dbg;
}

void vulkan_renderer::uninit_debug()
{
	fvkDestroyDebugReportCallbackEXT((VkInstance)_instance, _debug_report_callback, nullptr);
}

#else

void vulkan_renderer::setup_debug() {}
void vulkan_renderer::init_debug() {}
void vulkan_renderer::uninit_debug() {}

#endif


void vulkan_renderer::init_command_buffers()
{
	/*VkCommandPoolCreateInfo command_pool_ci = {};
	command_pool_ci.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
	command_pool_ci.queueFamilyIndex = _graphics_family_index;
	command_pool_ci.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
	VkCommandPool cmdpool;
	vkCreateCommandPool(_device, &command_pool_ci, nullptr, &cmdpool);

	VkCommandBufferAllocateInfo cmd_buffer_alloc_ci = {};
	cmd_buffer_alloc_ci.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	cmd_buffer_alloc_ci.commandPool = cmdpool;
	cmd_buffer_alloc_ci.commandBufferCount = 1;
	cmd_buffer_alloc_ci.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;

	vkAllocateCommandBuffers(_device, &cmd_buffer_alloc_ci, &_command_buffer);*/
}
