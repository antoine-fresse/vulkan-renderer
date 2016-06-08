#include "renderer.h"

#include "config_defines.h"
#include "platform.h"

#include "shared.h"

#include <algorithm>
#include <sstream>
#include <iostream>
#include <fstream>
#include <chrono>

PFN_vkCreateDebugReportCallbackEXT fvkCreateDebugReportCallbackEXT = nullptr;
PFN_vkDestroyDebugReportCallbackEXT fvkDestroyDebugReportCallbackEXT = nullptr;



renderer::renderer(uint32_t width, uint32_t height, uint32_t buffering, const std::vector<const char*>& instance_layers, const std::vector<const char*>& instance_extensions, const std::vector<const char*>& device_layers, const std::vector<const char*>& device_extensions)
: _debug_report_callback_create_info( vk::DebugReportFlagsEXT{}, nullptr, nullptr)
, _texture_manager(*this)
{
	glfwInit();
	if (GLFW_FALSE == glfwVulkanSupported())
	{
		// not supported
		glfwTerminate();
		throw std::system_error(vk::Result::eErrorInitializationFailed, "Cannot initialize GLFW, vulkan is not supported");
	}
	
	// Setup layers & extensions for debugging first
	setup_debug();

	_device_extensions.push_back(VK_KHR_SWAPCHAIN_EXTENSION_NAME);

	// Extensions required by glfw
	uint32_t instance_extension_count = 0;
	const char ** instance_extensions_buffer = glfwGetRequiredInstanceExtensions(&instance_extension_count);
	for (uint32_t i = 0; i < instance_extension_count; ++i)
	{
		// Push back required instance extensions as well
		_instance_extensions.push_back(instance_extensions_buffer[i]);
	}

	// Layers & Extensions asked by the user
	_instance_layers.insert(_instance_layers.end(), instance_layers.begin(), instance_layers.end());
	_instance_extensions.insert(_instance_extensions.end(), instance_extensions.begin(), instance_extensions.end());
	_device_layers.insert(_device_layers.end(), device_layers.begin(), device_layers.end());
	_device_extensions.insert(_device_extensions.end(), device_extensions.begin(), device_extensions.end());

	init_instance();
	init_debug();

	create_window_and_surface(width, height, buffering);

	init_device();

	recreate_swapchain(buffering, _width, _height);

	init_render_command_buffers();
	_texture_manager.init();
	_ready = true;
}

renderer::~renderer()
{
	_device.waitIdle();

	_device.destroyCommandPool(_render_command_pool);

	_device.destroySwapchainKHR(_swapchain);

	_instance.destroySurfaceKHR(_surface);
	glfwDestroyWindow(_window);

	_device.destroySemaphore(_rendering_finished_semaphore);
	_device.destroySemaphore(_image_available_semaphore);
	_device.destroy();
	
	uninit_debug();
	
	_instance.destroy();
}

void renderer::init_instance()
{
	{
		printf("Instance Layers :\n");
		for (auto layer : vk::enumerateInstanceLayerProperties())
		{
			printf("%s : %s\n", layer.layerName(), layer.description());
		}
	}
	
	vk::ApplicationInfo app_info{ "vulkan test", 1, "vulkan test", 1, VK_API_VERSION_1_0 };
	
	vk::InstanceCreateInfo ci( vk::InstanceCreateFlags {} , &app_info, _instance_layers.size(), _instance_layers.data(), _instance_extensions.size(), _instance_extensions.data() );
	ci.pNext(&_debug_report_callback_create_info);
	
	_instance = vk::createInstance(ci);
}

void renderer::init_device()
{
	std::vector<vk::PhysicalDevice> physical_devices = _instance.enumeratePhysicalDevices();
	_gpu = physical_devices[0];

	_memory_properties = _gpu.getMemoryProperties();
	_gpu_properties = _gpu.getProperties();
	
	std::vector<vk::Format> depth_formats = {
		vk::Format::eD32SfloatS8Uint,
		vk::Format::eD32Sfloat,
		vk::Format::eD24UnormS8Uint,
		vk::Format::eD16UnormS8Uint,
		vk::Format::eD16Unorm,
	};

	_depth_format = vk::Format::eUndefined;

	for (auto& format : depth_formats)
	{
		auto format_props = _gpu.getFormatProperties(format);
		if (format_props.optimalTilingFeatures() & vk::FormatFeatureFlagBits::eDepthStencilAttachment)
		{
			_depth_format = format;
			break;
		}
	}

	if (_depth_format == vk::Format::eUndefined)
		throw renderer_exception("Cannot find suitable depth format");

	std::tie(_graphics_family_index, _present_family_index) = retrieve_queues_family_index();

	float queue_priorities[] = { 1.0f };

	std::vector<vk::DeviceQueueCreateInfo> queues_ci;

	queues_ci.emplace_back(vk::DeviceQueueCreateFlags{}, _graphics_family_index, 1, queue_priorities);
	if(_graphics_family_index != _present_family_index)
		queues_ci.emplace_back(vk::DeviceQueueCreateFlags{}, _present_family_index, 1, queue_priorities);

	auto features = _gpu.getFeatures();
	features.shaderClipDistance(VK_TRUE);

	vk::DeviceCreateInfo device_ci( vk::DeviceCreateFlags{}, queues_ci.size(), queues_ci.data(), _device_layers.size(), _device_layers.data(), _device_extensions.size(), _device_extensions.data(), &features );

	{
		printf("Device Layers :\n");
		for (auto layer : _gpu.enumerateDeviceLayerProperties())
		{
			printf("%s : %s\n", layer.layerName(), layer.description());
		}
	}

	_device = _gpu.createDevice(device_ci);

	_graphics_queue = _device.getQueue(_graphics_family_index, 0);
	_present_queue = _device.getQueue(_present_family_index, 0);

	_rendering_finished_semaphore = _device.createSemaphore({});
	_image_available_semaphore = _device.createSemaphore({});

	_pipeline_cache = _device.createPipelineCache(vk::PipelineCacheCreateInfo{ {}, 0, nullptr });
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
	{
		MessageBox(nullptr, stream.str().c_str(), "Vulkan Error!", 0);
		__debugbreak();
	}
#endif
	return false;
}

void renderer::setup_debug()
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
	// _instance_layers.push_back("VK_LAYER_RENDERDOC_Capture");

	_instance_extensions.push_back(VK_EXT_DEBUG_REPORT_EXTENSION_NAME);

	_device_layers.push_back("VK_LAYER_LUNARG_standard_validation");
	// _device_layers.push_back("VK_LAYER_RENDERDOC_Capture");

}

void renderer::init_debug()
{
	fvkCreateDebugReportCallbackEXT = (PFN_vkCreateDebugReportCallbackEXT)vkGetInstanceProcAddr((VkInstance)_instance, "vkCreateDebugReportCallbackEXT");
	fvkDestroyDebugReportCallbackEXT = (PFN_vkDestroyDebugReportCallbackEXT)vkGetInstanceProcAddr((VkInstance)_instance, "vkDestroyDebugReportCallbackEXT");
	// assert(fvkCreateDebugReportCallbackEXT && fvkDestroyDebugReportCallbackEXT);

	VkDebugReportCallbackEXT dbg;
	fvkCreateDebugReportCallbackEXT((VkInstance)_instance, (VkDebugReportCallbackCreateInfoEXT*)&_debug_report_callback_create_info, nullptr, &dbg);
	_debug_report_callback = dbg;
}

void renderer::uninit_debug()
{
	fvkDestroyDebugReportCallbackEXT((VkInstance)_instance, _debug_report_callback, nullptr);
}

#else

void renderer::setup_debug() {}
void renderer::init_debug() {}
void renderer::uninit_debug() {}

#endif


void renderer::init_render_command_buffers()
{
	VkCommandPoolCreateInfo command_pool_ci = {};
	command_pool_ci.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
	command_pool_ci.queueFamilyIndex = _graphics_family_index;
	command_pool_ci.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
	_render_command_pool = _device.createCommandPool(command_pool_ci);

	VkCommandBufferAllocateInfo cmd_buffer_alloc_ci = {};
	cmd_buffer_alloc_ci.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	cmd_buffer_alloc_ci.commandPool = _render_command_pool;
	cmd_buffer_alloc_ci.commandBufferCount = _swapchain_images.size()+1;
	cmd_buffer_alloc_ci.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;

	_render_command_buffers = _device.allocateCommandBuffers(cmd_buffer_alloc_ci);
	_setup_command_buffer = _render_command_buffers.back();
	_render_command_buffers.pop_back();
}

std::pair<uint32_t, uint32_t> renderer::retrieve_queues_family_index()
{
	std::pair<uint32_t, uint32_t> result(UINT32_MAX, UINT32_MAX);
	
	auto family_properties = _gpu.getQueueFamilyProperties();

	for(uint32_t i = 0; i<family_properties.size(); ++i)
	{
		auto surface_support_khr = _gpu.getSurfaceSupportKHR(i, _surface);

		if(family_properties[i].queueCount() > 0 && bool(family_properties[i].queueFlags() & vk::QueueFlagBits::eGraphics))
		{
			if (result.first == UINT32_MAX)
				result.first = i;


			if(surface_support_khr)
			{
				result.first = i;
				result.second = i;
				return result;
			}
		}

		if (surface_support_khr && result.second == UINT32_MAX)
			result.second = i;
	}
	
	return result;
}

uint32_t renderer::find_adequate_memory(vk::MemoryRequirements mem_reqs, vk::MemoryPropertyFlagBits requirements_mask) const
{
	auto bits = mem_reqs.memoryTypeBits();

	for (uint32_t i = 0; i < 32; ++i)
	{
		if ((bits & 1) == 1)
		{
			if ((_memory_properties.memoryTypes()[i].propertyFlags() & requirements_mask) == requirements_mask)
			{
				return i;
			}
		}
		bits >>= 1;
	}
	throw renderer_exception("Cannot find suitable memory for the specified requirements");
}

void renderer::create_window_and_surface(uint32_t width, uint32_t height, uint32_t buffering)
{
	glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API); // This tells GLFW to not create an OpenGL context with the window
	GLFWwindow * window = glfwCreateWindow(width, height, "vulkan_test", nullptr, nullptr);
	if (!window)
	{
		throw std::system_error(vk::Result::eErrorInitializationFailed, "Cannot GLFW create window");
	}

	// make sure we indeed get the surface size we want.
	int width_fb = 0, height_fb = 0;
	glfwGetFramebufferSize(window, &width_fb, &height_fb);

	_width = width_fb;
	_height = height_fb;

	VkSurfaceKHR surface = VK_NULL_HANDLE;
	VkResult ret = glfwCreateWindowSurface(_instance, window, nullptr, &surface);
	if (VK_SUCCESS != ret)
	{
		glfwDestroyWindow(window);
		throw std::system_error(vk::Result::eErrorInitializationFailed, "Cannot create window surface KHR");
	}

	_window = window;
	_surface = surface;
}

vk::ShaderModule renderer::load_shader(const std::string & filename) const
{
	std::ifstream vertex_shader_file{ filename, std::ifstream::binary };

	vertex_shader_file.seekg(0, vertex_shader_file.end);
	int length = vertex_shader_file.tellg();
	vertex_shader_file.seekg(0, vertex_shader_file.beg);

	std::vector<char> vertex_shader_data(length);

	vertex_shader_file.read(vertex_shader_data.data(), length);
	auto vertex_shader_module = _device.createShaderModule(vk::ShaderModuleCreateInfo{ {}, (size_t)length, reinterpret_cast<const uint32_t*>(vertex_shader_data.data()) });

	return vertex_shader_module;
}

void renderer::recreate_swapchain(uint32_t buffering, uint32_t width, uint32_t height)
{
	vk::SurfaceCapabilitiesKHR surface_capabilities;
	_gpu.getSurfaceCapabilitiesKHR(_surface, surface_capabilities);

	auto surface_formats = _gpu.getSurfaceFormatsKHR(_surface);
	auto surface_present_modes = _gpu.getSurfacePresentModesKHR(_surface);

	int image_count = buffering;
	if (image_count < surface_capabilities.minImageCount() || image_count > surface_capabilities.maxImageCount())
		throw std::system_error(vk::Result::eErrorFeatureNotPresent, "Unsupported Buffering");

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


	vk::ImageUsageFlags usage_flags = vk::ImageUsageFlagBits::eColorAttachment;
	auto supported_usage = surface_capabilities.supportedUsageFlags();
	if (supported_usage & vk::ImageUsageFlagBits::eTransferDst)
	{
		usage_flags |= vk::ImageUsageFlagBits::eTransferDst;
	}
	else
	{
		std::cout << "Warning : TransferDst usage not supported" << std::endl;
	}

	vk::SurfaceTransformFlagBitsKHR transform_flags;
	if (surface_capabilities.supportedTransforms() & vk::SurfaceTransformFlagBitsKHR::eIdentity)
		transform_flags = vk::SurfaceTransformFlagBitsKHR::eIdentity;
	else
		transform_flags = surface_capabilities.currentTransform();


	auto present_mode = std::find_if(surface_present_modes.begin(), surface_present_modes.end(), [](vk::PresentModeKHR pmode) { return pmode == vk::PresentModeKHR::eFifo; });
	if (present_mode == surface_present_modes.end())
		present_mode = std::find_if(surface_present_modes.begin(), surface_present_modes.end(), [](vk::PresentModeKHR pmode) { return pmode == vk::PresentModeKHR::eMailbox; });
	if (present_mode == surface_present_modes.end())
		throw std::system_error(vk::Result::eErrorFeatureNotPresent, "Unsupported present mode");

	if (!_gpu.getSurfaceSupportKHR(_present_family_index, _surface))
	{
		throw std::system_error(vk::Result::eErrorFeatureNotPresent, "Unsupported present mode");
	}

	vk::SwapchainKHR old_swapchain_khr = _swapchain;
	vk::SwapchainCreateInfoKHR swapchain_ci(vk::SwapchainCreateFlagsKHR{}, _surface, image_count, format.format(), format.colorSpace(), swap_chain_extent, 1, usage_flags, vk::SharingMode::eExclusive, 0, nullptr, transform_flags, vk::CompositeAlphaFlagBitsKHR::eOpaque, *present_mode, VK_TRUE, old_swapchain_khr);
	
	if (old_swapchain_khr)
	{
		_device.destroySwapchainKHR(old_swapchain_khr);
	}

	_swapchain = _device.createSwapchainKHR(swapchain_ci);
	_swapchain_images = _device.getSwapchainImagesKHR(_swapchain);
	_swapchain_format = format.format();
}

double renderer::render(vk::Fence fence)
{
	if (_need_setup)
		flush_setup();

	auto result = _device.acquireNextImageKHR(_swapchain, UINT64_MAX, _image_available_semaphore, VK_NULL_HANDLE, &_current_image_index);
	// if (result == vk::Result::eSuboptimalKHR || result == vk::Result::eSuccess)
	{
		vk::PipelineStageFlags pipeline_stage_flags{ vk::PipelineStageFlagBits::eTransfer };
		vk::SubmitInfo submit_info{ 1, &_image_available_semaphore, &pipeline_stage_flags, 1, &_render_command_buffers[_current_image_index], 1, &_rendering_finished_semaphore };

		auto render_begin = std::chrono::steady_clock::now();
		_graphics_queue.submit(submit_info, fence);
		if(fence)
		{
			_device.waitForFence(fence, true, UINT64_MAX);
			auto render_end = std::chrono::steady_clock::now();
			std::chrono::duration<double> render_time = render_end - render_begin;
			_device.resetFence(fence);
			return render_time.count();
		}
	}
	//	renderer.window_size_changed();
	return 0.0;
}

void renderer::present() const
{
	_present_queue.presentKHR(vk::PresentInfoKHR{ 1, &_rendering_finished_semaphore, 1, &_swapchain, &_current_image_index, nullptr});
}

vk::CommandBuffer renderer::setup_cmd_buffer()
{
	if (!_need_setup)
	{
		_setup_command_buffer.begin(vk::CommandBufferBeginInfo{ vk::CommandBufferUsageFlagBits::eOneTimeSubmit, nullptr });
		_need_setup = true;
	}
	return _setup_command_buffer;
}

void renderer::flush_setup()
{
	_setup_command_buffer.end();
	vk::SubmitInfo submit_info{ 0, nullptr, nullptr, 1, &_setup_command_buffer, 0, nullptr };
	_graphics_queue.submit(1, &submit_info, VK_NULL_HANDLE);
	
	// TODO(antoine) non blocking/fast way to do setups
	_graphics_queue.waitIdle();

	_setup_command_buffer.reset({});
	_need_setup = false;
}

vk::DeviceSize renderer::ubo_aligned_size(vk::DeviceSize size) const
{
	vk::DeviceSize align = _gpu_properties.limits().minUniformBufferOffsetAlignment();
	vk::DeviceSize result = (size + align - 1) & ~(align - 1);
	return result;
}
