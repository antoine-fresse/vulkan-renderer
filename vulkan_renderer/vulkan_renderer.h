#pragma once
#include "vulkan_include.h"
#include "vulkan_window.h"

#include <vector>

class vulkan_renderer
{
public:
	vulkan_renderer(const std::vector<const char*>& instance_layers, const std::vector<const char*>& instance_extensions, const std::vector<const char*>& device_layers, const std::vector<const char*>& device_extensions);
	~vulkan_renderer();

	const vk::Instance&			instance()						const { return _instance; }
	const vk::PhysicalDevice&	gpu()							const { return _gpu; }
	const vk::Device&			device()						const { return _device; }
	const vk::Queue&			queue()							const { return _queue; }
	uint32_t					graphics_family_index()			const { return _graphics_family_index; }
	vulkan_window&				window()							  { return _window; }
	const vk::Semaphore&		rendering_finished_semaphore()	const { return _rendering_finished_semaphore; }


	void						open_window(int width, int height, int buffering=2);


	vk::ShaderModule			load_shader(const std::string& filename) const;

private:

	void init_instance();
	void init_device();

	void setup_debug();
	
	void init_debug();
	void uninit_debug();

	void init_command_buffers();

	vk::Instance							_instance;
	vk::PhysicalDevice						_gpu;
	vk::Device								_device;
	vk::Queue								_queue;
	uint32_t								_graphics_family_index = 0;

	vk::DebugReportCallbackEXT				_debug_report_callback;
	vk::DebugReportCallbackCreateInfoEXT	_debug_report_callback_create_info;

	std::vector<const char*>				_instance_layers;
	std::vector<const char*>				_instance_extensions;
	std::vector<const char*>				_device_layers;
	std::vector<const char*>				_device_extensions;

	vulkan_window							_window;

	vk::Semaphore							_rendering_finished_semaphore;
	
};
