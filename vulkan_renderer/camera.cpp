#include "camera.h"
#include "renderer.h"

camera::camera(renderer& renderer) : _renderer(renderer), _ubo(renderer, vk::BufferUsageFlagBits::eVertexBuffer)
{
	// Vulkan clip space has inverted Y and half Z.
	_clip = glm::mat4(	1.0f, 0.0f, 0.0f, 0.0f,
	                  	0.0f, -1.0f, 0.0f, 0.0f,
	                  	0.0f, 0.0f, 0.5f, 0.0f,
	                  	0.0f, 0.0f, 0.5f, 1.0f);
	_projection = glm::perspective(glm::radians(45.0f), 800.0f/600.0f, 0.1f, 100.0f);
	_view = glm::lookAt(glm::vec3(0, 3, 10), glm::vec3(), glm::vec3(0, 1, 0));

	recompute_cache();
	
	_ubo.update(_cached_vpc);
}

vk::DescriptorBufferInfo camera::descriptor_buffer_info() const
{
	return { _ubo.buffer(), 0, sizeof(glm::mat4) };
}
