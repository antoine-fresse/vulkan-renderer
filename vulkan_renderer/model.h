#pragma once
#include "vulkan_include.h"
#include "math_include.h"
#include "texture.h"

#include <memory>
#include <chrono>

class pipeline;
class renderer;
class managed_descriptor_set;

class model
{
public:
	explicit model(const std::string& filepath, renderer& renderer, float scale = 1.0f);
	~model();

	static vk::VertexInputBindingDescription binding_description(uint32_t bind_id);
	static std::vector<vk::VertexInputAttributeDescription> attribute_descriptions(uint32_t bind_id = 0);
	static uint32_t vertex_stride() { return sizeof(vertex); }

	vk::DescriptorBufferInfo descriptor_buffer_info() const { return vk::DescriptorBufferInfo{ _buffer, _uniform_buffer_offset, sizeof(uniform_object) }; }

	void draw(const vk::CommandBuffer& cmd, pipeline& pipeline, uint32_t bind_id = 0) const;
	
	void attach_textures(pipeline& pipeline, uint32_t set_index);
	void update(double dt);

private:
	
	struct vertex
	{
		glm::vec3 position;
		glm::vec3 normal;
		glm::vec3 tangent;
		glm::vec2 uv;
	};
	static_assert(sizeof(vertex) == 11 * sizeof(float), "vertex is not packed");

	struct material
	{
		std::shared_ptr<texture> diffuse_texture;
		std::shared_ptr<texture> normal_texture;
		std::shared_ptr<texture> specular_texture;
		std::shared_ptr<managed_descriptor_set> textures_set;
	};

	std::vector<material> _materials;

	class mesh
	{
	public:
		mesh(vk::DeviceSize vertex_buffer_offset, vk::DeviceSize index_buffer_offset, vk::DeviceSize index_count, uint32_t material_index)
			: _vertex_buffer_offset(vertex_buffer_offset),
			  _index_buffer_offset(index_buffer_offset),
			  _index_count(index_count),
			  _material_index(material_index)
		{
		}

		vk::DeviceSize vertex_buffer_offset() const { return _vertex_buffer_offset; }
		vk::DeviceSize index_buffer_offset() const { return _index_buffer_offset; }
		vk::DeviceSize index_count() const { return _index_count; }
		uint32_t material_index() const { return _material_index; }

		void vertex_buffer_offset(vk::DeviceSize val) { _vertex_buffer_offset = val; }
		void index_buffer_offset(vk::DeviceSize val) { _index_buffer_offset = val; }
		void index_count(vk::DeviceSize val) { _index_count = val; }


	private:
		vk::DeviceSize _vertex_buffer_offset;
		vk::DeviceSize _index_buffer_offset;
		vk::DeviceSize _index_count;
		uint32_t _material_index;
	};

	struct uniform_object
	{
		glm::mat4 model_matrix;
	} _uniform_object;


	void load_model(const std::string& filepath, float scale = 1.0f);

	std::vector<mesh> _meshes;

	vk::Buffer _buffer;
	vk::DeviceMemory _memory;
	vk::MemoryRequirements _memory_reqs;

	vk::DeviceSize _uniform_buffer_offset;

	renderer& _renderer;
};
