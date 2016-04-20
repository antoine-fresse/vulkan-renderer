#include "vulkan_window.h"

#include "vulkan_renderer.h"


vulkan_window::vulkan_window()
{
	
}

void vulkan_window::create(vulkan_renderer* renderer, uint32_t width, uint32_t height, uint32_t buffering)
{
	if (_window) return;

	_renderer = renderer;

	glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API); // This tells GLFW to not create an OpenGL context with the window
	GLFWwindow * window = glfwCreateWindow(width, height, "vulkan_test", nullptr, nullptr);
	if (!window)
	{
		throw std::exception("Cannot create window");
	}

	// make sure we indeed get the surface size we want.
	int width_fb=0, height_fb=0;
	glfwGetFramebufferSize(window, &width_fb, &height_fb);

	_width = width_fb;
	_height = height_fb;

	VkSurfaceKHR surface = VK_NULL_HANDLE;
	VkResult ret = glfwCreateWindowSurface(renderer->instance(), window, nullptr, &surface);
	if (VK_SUCCESS != ret)
	{
		glfwDestroyWindow(window);
		throw std::exception("Cannot create window surface KHR");
	}

	_window = window;
	_surface = surface;

	recreate_swapchain(buffering, _width, _height);
}



void vulkan_window::destroy()
{
	if (_window)
	{
		auto device = _renderer->device();

		device.destroyCommandPool(_present_queue_command_pool);
		device.destroySwapchainKHR(_swapchain);
		device.destroySemaphore(_image_available_semaphore);

		_renderer->instance().destroySurfaceKHR(_surface);
		glfwDestroyWindow(_window);
	}
}

void vulkan_window::destroy_swapchain_resources()
{
	if(_swapchain)
	{
		auto device = _renderer->device();
		device.waitIdle();

		if (_present_queue_cmd_buffers.size())
			device.freeCommandBuffers(_present_queue_command_pool, _present_queue_cmd_buffers.size(), _present_queue_cmd_buffers.data());

		_present_queue_cmd_buffers.clear();

		device.destroyCommandPool(_present_queue_command_pool);
		_present_queue_command_pool = VK_NULL_HANDLE;
	}
}

void vulkan_window::recreate_swapchain(uint32_t buffering, uint32_t width, uint32_t height)
{
	destroy_swapchain_resources();

	auto gpu = _renderer->gpu();
	auto device = _renderer->device();

	vk::SurfaceCapabilitiesKHR surface_capabilities;
	gpu.getSurfaceCapabilitiesKHR(_surface, surface_capabilities);

	auto surface_formats = gpu.getSurfaceFormatsKHR(_surface);
	auto surface_present_modes = gpu.getSurfacePresentModesKHR(_surface);

	int image_count = buffering;
	if (image_count < surface_capabilities.minImageCount() || image_count > surface_capabilities.maxImageCount())
		throw std::exception("Unsupported Buffering");

	vk::SurfaceFormatKHR format{ vk::Format::eR8G8B8A8Unorm, vk::ColorSpaceKHR::eVkColorspaceSrgbNonlinear };
	if (surface_formats.size() == 1 && surface_formats[0].format() == vk::Format::eUndefined)
		format = { vk::Format::eR8G8B8A8Unorm, vk::ColorSpaceKHR::eVkColorspaceSrgbNonlinear };
	else
	{
		auto found = std::find_if(surface_formats.begin(), surface_formats.end(), [](vk::SurfaceFormatKHR f)
		{
			return f.format() == vk::Format::eR8G8B8A8Unorm;
		});

		if (found == surface_formats.end())
			format = surface_formats[0];
		else
			format = *found;
	}
	vk::Extent2D swap_chain_extent{ width, height };
	if (surface_capabilities.currentExtent().width() == -1)
	{
		if (swap_chain_extent.width() < surface_capabilities.minImageExtent().width())
			swap_chain_extent.width(surface_capabilities.minImageExtent().width());
		if (swap_chain_extent.height() < surface_capabilities.minImageExtent().height())
			swap_chain_extent.height(surface_capabilities.minImageExtent().height());

		if (swap_chain_extent.width() > surface_capabilities.maxImageExtent().width())
			swap_chain_extent.width(surface_capabilities.maxImageExtent().width());
		if (swap_chain_extent.height() > surface_capabilities.maxImageExtent().height())
			swap_chain_extent.height(surface_capabilities.maxImageExtent().height());
	}
	else
		swap_chain_extent = surface_capabilities.currentExtent();


	vk::ImageUsageFlags usage_flags;
	if (surface_capabilities.supportedUsageFlags() & vk::ImageUsageFlagBits::eTransferDst)
	{
		usage_flags |= vk::ImageUsageFlagBits::eTransferDst;
		usage_flags |= vk::ImageUsageFlagBits::eColorAttachment;
	}
	else
		throw std::exception("Unsupported Transfer Dst");

	vk::SurfaceTransformFlagBitsKHR transform_flags;
	if (surface_capabilities.supportedTransforms() & vk::SurfaceTransformFlagBitsKHR::eIdentity)
		transform_flags = vk::SurfaceTransformFlagBitsKHR::eIdentity;
	else
		transform_flags = surface_capabilities.currentTransform();


	auto present_mode = std::find_if(surface_present_modes.begin(), surface_present_modes.end(), [](vk::PresentModeKHR pmode) { return pmode == vk::PresentModeKHR::eMailbox; });
	if (present_mode == surface_present_modes.end())
		present_mode = std::find_if(surface_present_modes.begin(), surface_present_modes.end(), [](vk::PresentModeKHR pmode) { return pmode == vk::PresentModeKHR::eFifo; });
	if (present_mode == surface_present_modes.end())
		throw std::exception("Unsupported present mode");

	if (!gpu.getSurfaceSupportKHR(_renderer->graphics_family_index(), _surface))
	{
		throw std::exception("Unsupported present mode");
	}

	vk::SwapchainKHR old_swapchain_khr = _swapchain;
	vk::SwapchainCreateInfoKHR swapchain_ci(vk::SwapchainCreateFlagsKHR{}, _surface, image_count, format.format(), format.colorSpace(), swap_chain_extent, 1, usage_flags, vk::SharingMode::eExclusive, 0, nullptr, transform_flags, vk::CompositeAlphaFlagBitsKHR::eOpaque, *present_mode, VK_TRUE, old_swapchain_khr);

	if(old_swapchain_khr)
	{
		device.destroySwapchainKHR(old_swapchain_khr);
	}

	_image_available_semaphore = device.createSemaphore({});
	_swapchain = device.createSwapchainKHR(swapchain_ci);
	
	// Get real image count
	device.getSwapchainImagesKHR(_swapchain, &_image_count, nullptr);

	_present_queue_command_pool = device.createCommandPool(vk::CommandPoolCreateInfo{ {}, _renderer->graphics_family_index() });

	vk::CommandBufferAllocateInfo cmd_alloc_info{ _present_queue_command_pool, vk::CommandBufferLevel::ePrimary, _image_count };

	_present_queue_cmd_buffers = device.allocateCommandBuffers(cmd_alloc_info);

	_swapchain_images = device.getSwapchainImagesKHR(_swapchain);


	_swapchain_format = format.format();
}

void vulkan_window::window_size_changed()
{
	recreate_swapchain(_image_count, _width, _height);
}



vulkan_window::~vulkan_window()
{
	
}


