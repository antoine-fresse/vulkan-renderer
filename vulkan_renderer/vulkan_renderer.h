#pragma once
#include "vulkan_include.h"
#include "vulkan_window.h"

#include <vector>

class vulkan_renderer
{
public:
	vulkan_renderer(uint32_t width, uint32_t height, uint32_t buffering, const std::vector<const char*>& instance_layers, const std::vector<const char*>& instance_extensions, const std::vector<const char*>& device_layers, const std::vector<const char*>& device_extensions);
	~vulkan_renderer();

	const vk::Instance&						instance()						const { return _instance; }
	const vk::PhysicalDevice&				gpu()							const { return _gpu; }
	const vk::Device&						device()						const { return _device; }
	const vk::Queue&						graphics_queue()				const { return _graphics_queue; }
	const vk::Queue&						present_queue()					const { return _present_queue; }
	uint32_t								graphics_family_index()			const { return _graphics_family_index; }
	uint32_t								present_family_index()			const { return _present_family_index; }
	const vk::Semaphore&					rendering_finished_semaphore()	const { return _rendering_finished_semaphore; }
	const vk::SurfaceKHR&					surface()						const { return _surface; }
	const vk::Semaphore&					image_available_semaphore()		const { return _image_available_semaphore; }
	const vk::SwapchainKHR&					swapchain()						const { return _swapchain; }
	const std::vector<vk::Image>&			swapchain_images()				const { return _swapchain_images; }
	const std::vector<vk::CommandBuffer>&	present_queue_cmd_buffers()		const { return _present_queue_cmd_buffers; };
	const vk::Format&						format()						const { return _swapchain_format; }
	GLFWwindow*								window_handle()					const { return _window; }
	uint32_t								width() 						const { return _width; }
	uint32_t								height()						const { return _height; }

	vk::ShaderModule			load_shader(const std::string& filename) const;

private:

	void init_instance();
	void init_device();

	void setup_debug();
	
	void init_debug();
	void uninit_debug();

	void init_command_buffers();

	void create_window_and_surface(uint32_t width, uint32_t height, uint32_t buffering);
	void recreate_swapchain(uint32_t buffering, uint32_t width, uint32_t height);
	void destroy_swapchain_resources();

	std::pair<uint32_t, uint32_t>	retrieve_queues_family_index();

	vk::Instance							_instance;
	vk::PhysicalDevice						_gpu;
	vk::Device								_device;
	vk::Queue								_graphics_queue;
	vk::Queue								_present_queue;
	uint32_t								_graphics_family_index = 0;
	uint32_t								_present_family_index = 0;
	

	vk::DebugReportCallbackEXT				_debug_report_callback;
	vk::DebugReportCallbackCreateInfoEXT	_debug_report_callback_create_info;

	std::vector<const char*>				_instance_layers;
	std::vector<const char*>				_instance_extensions;
	std::vector<const char*>				_device_layers;
	std::vector<const char*>				_device_extensions;

	vk::Semaphore							_rendering_finished_semaphore;

	GLFWwindow*								_window = nullptr;
	vk::SurfaceKHR							_surface;

	uint32_t								_width = 0;
	uint32_t								_height = 0;

	vk::SwapchainKHR						_swapchain;
	std::vector<vk::Image>					_swapchain_images;
	vk::Semaphore							_image_available_semaphore;

	uint32_t								_image_count = 0;
	vk::CommandPool							_present_queue_command_pool;
	std::vector<vk::CommandBuffer>			_present_queue_cmd_buffers;
	vk::Format								_swapchain_format;

	
};
