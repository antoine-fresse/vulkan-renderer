#pragma once

#include "vulkan_include.h"
#include <vector>

class vulkan_renderer;

class vulkan_window
{
public:
	vulkan_window();
	~vulkan_window();

	const vk::SurfaceKHR&					surface() const { return _surface; }
	const vk::Semaphore&					image_available_semaphore() const { return _image_available_semaphore; }
	const vk::SwapchainKHR&					swapchain() const { return _swapchain; }
	const std::vector<vk::Image>&			swapchain_images() const { return _swapchain_images; }
	const std::vector<vk::CommandBuffer>&	present_queue_cmd_buffers() const { return _present_queue_cmd_buffers; };
	const vk::Format&						format() const { return _swapchain_format; }

	GLFWwindow*								window_handle() const { return _window; }
	void									create(vulkan_renderer* renderer, uint32_t width, uint32_t height, uint32_t buffering = 3);
	void									destroy();

	uint32_t								width() const { return _width; }
	uint32_t								height() const { return _height; }

	void									window_size_changed();

private:
	
	void									recreate_swapchain(uint32_t buffering, uint32_t width, uint32_t height);
	void									destroy_swapchain_resources();

	GLFWwindow*								_window = nullptr;
	vk::SurfaceKHR							_surface;
	vulkan_renderer*						_renderer;

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