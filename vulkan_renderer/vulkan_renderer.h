#pragma once
#include "vulkan_include.h"

#include <vector>



class vulkan_renderer
{
public:
	vulkan_renderer(const std::vector<const char*>& instance_layers, const std::vector<const char*>& instance_extensions, const std::vector<const char*>& device_layers, const std::vector<const char*>& device_extensions);
	~vulkan_renderer();

	vk::Instance get_instance() const { return _instance; }
	vk::PhysicalDevice get_gpu() const { return _gpu; }
	vk::Device get_device() const { return _device; }
	vk::Queue get_queue() const { return _queue; }
	uint32_t get_graphics_family_index() const { return _graphics_family_index; }

private:

	void init_instance();
	void init_device();

	void setup_debug();
	
	void init_debug();
	void uninit_debug();

	void init_command_buffers();

	vk::Instance _instance;
	vk::PhysicalDevice _gpu;
	vk::Device _device;
	vk::Queue _queue;
	uint32_t _graphics_family_index = 0;

	vk::CommandPool _command_pool;
	vk::CommandBuffer _command_buffer;

	vk::DebugReportCallbackEXT _debug_report_callback;
	vk::DebugReportCallbackCreateInfoEXT _debug_report_callback_create_info;

	std::vector<const char*> _instance_layers;
	std::vector<const char*> _instance_extensions;

	std::vector<const char*> _device_layers;
	std::vector<const char*> _device_extensions;
	
};
