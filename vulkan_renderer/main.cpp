#include "vulkan_renderer.h"

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

int main()
{
	glfwInit();

	if (GLFW_FALSE == glfwVulkanSupported())
	{
		// not supported
		glfwTerminate();
		return -1;
	}

	std::vector<const char*> instance_layers;
	std::vector<const char*> device_layers;
	std::vector<const char*> instance_extensions;
	std::vector<const char*> device_extensions;

	device_extensions.push_back(VK_KHR_SWAPCHAIN_EXTENSION_NAME);

	uint32_t instance_extension_count = 0;
	const char ** instance_extensions_buffer = glfwGetRequiredInstanceExtensions(&instance_extension_count);
	for (uint32_t i = 0; i < instance_extension_count; ++i)
	{
		// Push back required instance extensions as well
		instance_extensions.push_back(instance_extensions_buffer[i]);
	}


	vulkan_renderer renderer{ instance_layers , instance_extensions, device_layers, device_extensions };
	vk::Instance instance = renderer.get_instance();
	int width = 800;
	int height = 600;
	glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);		// This tells GLFW to not create an OpenGL context with the window
	auto window = glfwCreateWindow(width, height, "vulkan_test", nullptr, nullptr);

	// make sure we indeed get the surface size we want.
	glfwGetFramebufferSize(window, &width, &height);

	VkSurfaceKHR surface = VK_NULL_HANDLE;
	VkResult ret = glfwCreateWindowSurface(instance, window, nullptr, &surface);
	if (VK_SUCCESS != ret)
	{
		glfwTerminate();
		return -1;
	}
	
	instance.destroySurfaceKHR(surface);
	glfwDestroyWindow(window);
}
