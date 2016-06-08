#pragma once
#include "vulkan_include.h"

#include "texture_manager.h"

#include <vector>

class renderer
{
public:
	renderer(uint32_t width, uint32_t height, uint32_t buffering, const std::vector<const char*>& instance_layers, const std::vector<const char*>& instance_extensions, const std::vector<const char*>& device_layers, const std::vector<const char*>& device_extensions);
	~renderer();

	vk::Instance							instance()						const { return _instance; }
	vk::PhysicalDevice						gpu()							const { return _gpu; }
	vk::Device								device()						const { return _device; }
	vk::Queue								graphics_queue()				const { return _graphics_queue; }
	vk::Queue								present_queue()					const { return _present_queue; }
	auto									graphics_family_index()			const { return _graphics_family_index; }
	auto									present_family_index()			const { return _present_family_index; }
	vk::Semaphore							rendering_finished_semaphore()	const { return _rendering_finished_semaphore; }
	vk::SurfaceKHR							surface()						const { return _surface; }
	vk::Semaphore							image_available_semaphore()		const { return _image_available_semaphore; }
	vk::SwapchainKHR						swapchain()						const { return _swapchain; }
	const std::vector<vk::Image>&			swapchain_images()				const { return _swapchain_images; }
	vk::Format								format()						const { return _swapchain_format; }
	const vk::PhysicalDeviceProperties&		gpu_properties()				const { return _gpu_properties; }
	GLFWwindow*								window_handle()					const { return _window; }
	auto									width() 						const { return _width; }
	auto									height()						const { return _height; }
	bool									ready()							const { return _ready;	}
	const std::vector<vk::CommandBuffer>&	render_command_buffers()		const { return _render_command_buffers;	}
	vk::Format								depth_format()					const { return _depth_format; }
	vk::PipelineCache						pipeline_cache()				const { return _pipeline_cache;	}

	texture_manager&						tex_manager() { return _texture_manager; }
	vk::ShaderModule						load_shader(const std::string& filename) const;
	uint32_t								find_adequate_memory(vk::MemoryRequirements mem_reqs, vk::MemoryPropertyFlagBits requirements_mask) const;

	double									render(vk::Fence fence = {});
	void									present() const;

	vk::CommandBuffer						setup_cmd_buffer();
	
	void									flush_setup();

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

	std::pair<uint32_t, uint32_t>			retrieve_queues_family_index();

	


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

	vk::PipelineCache						_pipeline_cache;

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
	vk::Format								_depth_format;
	vk::CommandPool							_render_command_pool;
	std::vector<vk::CommandBuffer>			_render_command_buffers;
	vk::CommandBuffer						_setup_command_buffer;
	bool									_need_setup = false;
	uint32_t								_current_image_index;
	
	texture_manager							_texture_manager;

};
