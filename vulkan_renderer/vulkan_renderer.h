#pragma once

#include <vulkan/vulkan.h>

#include <vector>

class vulkan_renderer
{
public:
	vulkan_renderer(bool debug_layers = false);
	~vulkan_renderer();

private:

	void init_instance();
	void init_device();

	void setup_debug();
	void init_debug();

	void init_command_buffers();


	VkInstance _instance = VK_NULL_HANDLE;
	VkPhysicalDevice _gpu = VK_NULL_HANDLE;
	VkDevice _device = VK_NULL_HANDLE;
	VkQueue _queue = VK_NULL_HANDLE;
	uint32_t _graphics_family_index = 0;

	VkCommandPool _command_pool;
	VkCommandBuffer _command_buffer;

	const bool _debug;
	VkDebugReportCallbackEXT _debug_report_callback = VK_NULL_HANDLE;
	VkDebugReportCallbackCreateInfoEXT _debug_report_callback_create_info = {};

	std::vector<const char*> _instance_layers;
	std::vector<const char*> _instance_extensions;

	std::vector<const char*> _device_layers;
	std::vector<const char*> _device_extensions;
	
};

