#pragma once
#include "vulkan_include.h"

#include "texture_manager.h"

#include <vector>

class renderer
{
public:
	renderer(uint32_t width, uint32_t height, uint32_t buffering, const std::vector<const char*>& instance_layers, const std::vector<const char*>& instance_extensions, const std::vector<const char*>& device_layers, const std::vector<const char*>& device_extensions);
	~renderer();

	const auto&								instance()						const { return _instance; }
	const auto&								gpu()							const { return _gpu; }
	const auto&								device()						const { return _device; }
	const auto&								graphics_queue()				const { return _graphics_queue; }
	const auto&								present_queue()					const { return _present_queue; }
	auto									graphics_family_index()			const { return _graphics_family_index; }
	auto									present_family_index()			const { return _present_family_index; }
	const auto&								rendering_finished_semaphore()	const { return _rendering_finished_semaphore; }
	const auto&								surface()						const { return _surface; }
	const auto&								image_available_semaphore()		const { return _image_available_semaphore; }
	const auto&								swapchain()						const { return _swapchain; }
	const auto&								swapchain_images()				const { return _swapchain_images; }
	const auto&								format()						const { return _swapchain_format; }
	const auto&								gpu_properties()				const { return _gpu_properties; }
	auto*									window_handle()					const { return _window; }
	auto									width() 						const { return _width; }
	auto									height()						const { return _height; }
	bool									ready()							const { return _ready;	}
	const auto&								render_command_buffers()		const { return _render_command_buffers;	}
	
	auto&									tex_manager() { return _texture_manager; }
	vk::ShaderModule						load_shader(const std::string& filename) const;
	uint32_t								find_adequate_memory(vk::MemoryRequirements mem_reqs, vk::MemoryPropertyFlagBits requirements_mask) const;

	void									render(vk::Fence fence = {});
	void									present() const;

	vk::DeviceSize							ubo_aligned_size(vk::DeviceSize size) const;

private:

	void init_instance();
	void init_device();

	void setup_debug();
	
	void init_debug();
	void uninit_debug();

	void init_render_command_buffers();

	void create_window_and_surface(uint32_t width, uint32_t height, uint32_t buffering);
	void recreate_swapchain(uint32_t buffering, uint32_t width, uint32_t height);

	std::pair<uint32_t, uint32_t>	retrieve_queues_family_index();

	


	bool									_ready = false;
	vk::Instance							_instance;
	vk::PhysicalDevice						_gpu;
	vk::Device								_device;
	vk::Queue								_graphics_queue;
	vk::Queue								_present_queue;
	uint32_t								_graphics_family_index = 0;
	uint32_t								_present_family_index = 0;
	
	vk::PhysicalDeviceMemoryProperties		_memory_properties;
	vk::PhysicalDeviceProperties			_gpu_properties;

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

	vk::Format								_swapchain_format;

	vk::CommandPool							_render_command_pool;
	std::vector<vk::CommandBuffer>			_render_command_buffers;

	uint32_t								_current_image_index;
	
	texture_manager							_texture_manager;

};
