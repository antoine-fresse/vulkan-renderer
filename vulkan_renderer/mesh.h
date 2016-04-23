#pragma once
#include "vulkan_include.h"
#include "math_include.h"

#include <string>

class vulkan_renderer;

class mesh
{
public:
	explicit mesh(const std::string& filepath, const vulkan_renderer& renderer, float scale = 1.0f);
	~mesh();

	static vk::VertexInputBindingDescription binding_description(uint32_t bind_id);
	static std::vector<vk::VertexInputAttributeDescription> attribute_descriptions(uint32_t bind_id = 0);
	static uint32_t vertex_stride() { return sizeof(vertex); }

	vk::DescriptorBufferInfo descriptor_buffer_info() const { return vk::DescriptorBufferInfo{ _buffer, 0, sizeof(glm::mat4) }; }

	void bind(vk::CommandBuffer cmd, uint32_t bind_id) const;
	void draw(vk::CommandBuffer cmd) const;
private:
	
	struct vertex
	{
		glm::vec3 position;
		glm::vec3 normal;
		glm::vec3 tangent;
		glm::vec2 uv;
	};

	static_assert(sizeof(vertex) == 11 * sizeof(float), "sizeof vertex is incorrect");

	void load_mesh(const std::string& filepath, float scale = 1.0f);
	

	vk::Buffer _buffer;
	vk::DeviceMemory _memory;
	vk::MemoryRequirements _memory_reqs;

	const vulkan_renderer& _renderer;

	uint32_t _vertex_buffer_offset;
	uint32_t _index_buffer_offset;
	uint32_t _index_count;
};
