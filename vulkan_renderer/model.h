#pragma once
#include "vulkan_include.h"
#include "math_include.h"
#include "texture.h"
#include "ubo.h"

#include <memory>
#include <chrono>

class camera;
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

	void draw(const vk::CommandBuffer& cmd, pipeline& pipeline, const camera& camera, uint32_t bind_id = 0) const;
	
	void attach_textures(pipeline& pipeline, uint32_t set_index);
	void update(double dt);

	struct material
	{
		std::shared_ptr<texture> diffuse_texture;
		std::shared_ptr<texture> normal_texture;
		std::shared_ptr<texture> specular_texture;
		std::shared_ptr<managed_descriptor_set> textures_set;
		struct info
		{
			glm::vec4 ambient_color;
			glm::vec4 diffuse_color;
			glm::vec4 specular_color;
			float specular_intensity;
			float normal_map_intensity;
		} info;
	};

private:
	
	struct vertex
	{
		glm::vec3 position;
		glm::vec3 normal;
		glm::vec3 tangent;
		glm::vec2 uv;
	};
	static_assert(sizeof(vertex) == 11 * sizeof(float), "vertex is not packed");

	

	

	std::vector<material> _materials;

	class mesh
	{
	public:
		

		mesh(vk::DeviceSize vertex_buffer_offset, vk::DeviceSize index_buffer_offset, vk::DeviceSize index_count, uint32_t material_index, const std::pair<glm::vec3, float>& bounding_sphere, const std::pair<glm::vec3, glm::vec3>& bounding_box)
			: vertex_buffer_offset(vertex_buffer_offset),
			  index_buffer_offset(index_buffer_offset),
			  index_count(index_count),
			  material_index(material_index),
			  bounding_sphere(bounding_sphere),
			  bounding_box(bounding_box)
		{
		}
		vk::DeviceSize vertex_buffer_offset;
		vk::DeviceSize index_buffer_offset;
		vk::DeviceSize index_count;
		uint32_t material_index;

		std::pair<glm::vec3, float> bounding_sphere;
		std::pair<glm::vec3, glm::vec3> bounding_box;
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
