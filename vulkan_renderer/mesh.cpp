#include "mesh.h"

#include "shared.h"
#include "vulkan_renderer.h"


#include <assimp/Importer.hpp>
#include <assimp/scene.h>           // Output data structure
#include <assimp/postprocess.h>     // Post processing flags


mesh::mesh(const std::string& filepath, const vulkan_renderer& renderer, float scale) : _renderer(renderer)
{
	load_mesh(filepath, scale);
}

mesh::~mesh()
{
	if(_buffer)
	{
		_renderer.device().destroyBuffer(_buffer);
		_renderer.device().freeMemory(_memory);
	}
}



void mesh::load_mesh(const std::string& filepath, float scale)
{
	Assimp::Importer importer;

	uint32_t flags = aiProcess_CalcTangentSpace | aiProcess_Triangulate | aiProcess_GenNormals | aiProcess_OptimizeMeshes | aiProcess_SortByPType;
	const aiScene* scene = importer.ReadFile(filepath, flags);

	if (!scene)
		throw renderer_exception("Cannot load mesh from file : " + filepath);


	auto projection = glm::perspective(glm::radians(45.0f), 800.0f / 600.0f, 0.1f, 300.0f);
	auto view = glm::lookAt(glm::vec3(0, 0, -10.0f), glm::vec3(), glm::vec3(0, 1, 0));
	auto model = glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, 0.0f, 10.0f));
	auto mvp = projection*view*model;

	std::vector<vertex> vertices;
	std::vector<uint32_t> indices;


	for (uint32_t i = 0; i < scene->mNumMeshes; ++i)
	{
		aiMesh* i_mesh = scene->mMeshes[i];
		if ((i_mesh->mPrimitiveTypes & aiPrimitiveType_TRIANGLE) == 0) continue;


		for (uint32_t k = 0; k < i_mesh->mNumVertices; ++k)
		{
			vertex vert;
			memcpy(&vert.position, &i_mesh->mVertices[k], sizeof(glm::vec3));
			vert.position *= scale;
			memcpy(&vert.normal, &i_mesh->mNormals[k], sizeof(glm::vec3));
			memcpy(&vert.tangent, &i_mesh->mTangents[k], sizeof(glm::vec3));
			memcpy(&vert.uv, &i_mesh->mTextureCoords[0][k], sizeof(glm::vec2));
			vertices.push_back(vert);
		}

		uint32_t base_index = indices.size();
		for (uint32_t k = 0; k < i_mesh->mNumFaces; ++k)
		{
			indices.push_back(i_mesh->mFaces[k].mIndices[0] + base_index);
			indices.push_back(i_mesh->mFaces[k].mIndices[1] + base_index);
			indices.push_back(i_mesh->mFaces[k].mIndices[2] + base_index);
		}
	}

	vk::Device device = _renderer.device();
	

	vk::DeviceSize size = vertices.size()*sizeof(vertex) + indices.size()*sizeof(uint32_t) + sizeof(glm::mat4);

	vk::BufferUsageFlags usage_flags = vk::BufferUsageFlagBits::eUniformBuffer | vk::BufferUsageFlagBits::eVertexBuffer | vk::BufferUsageFlagBits::eIndexBuffer;
	vk::BufferCreateInfo buffer_ci{ {}, size, usage_flags, vk::SharingMode::eExclusive, 0, nullptr };
	_buffer = device.createBuffer(buffer_ci);
	_memory_reqs = device.getBufferMemoryRequirements(_buffer);

	vk::MemoryAllocateInfo mem_allocate_info{ _memory_reqs.size(), _renderer.find_adequate_memory(_memory_reqs, vk::MemoryPropertyFlagBits::eHostVisible) };
	_memory = device.allocateMemory(mem_allocate_info);


	char* dst = (char*)device.mapMemory(_memory, 0, _memory_reqs.size(), {});

	// Uniform
	memcpy(dst, &mvp, sizeof(glm::mat4));

	_index_buffer_offset = sizeof(glm::mat4);
	_vertex_buffer_offset = sizeof(glm::mat4) + indices.size()*sizeof(uint32_t);
	_index_count = indices.size();

	memcpy(dst + _index_buffer_offset, indices.data(), indices.size()*sizeof(uint32_t));
	memcpy(dst + _vertex_buffer_offset, vertices.data(), vertices.size()*sizeof(vertex));

	device.unmapMemory(_memory);

	device.bindBufferMemory(_buffer, _memory, 0);
}

vk::VertexInputBindingDescription mesh::binding_description(uint32_t bind_id)
{
	return { bind_id, sizeof(vertex), vk::VertexInputRate::eVertex };
}

std::vector<vk::VertexInputAttributeDescription> mesh::attribute_descriptions(uint32_t bind_id)
{
	std::vector<vk::VertexInputAttributeDescription> attribute_description = {
		// Position
		vk::VertexInputAttributeDescription{ 0, bind_id, vk::Format::eR32G32B32Sfloat, offsetof(vertex, position) },
		// Normal
		vk::VertexInputAttributeDescription{ 1, bind_id, vk::Format::eR32G32B32Sfloat, offsetof(vertex, normal) },
		// Tangent
		vk::VertexInputAttributeDescription{ 2, bind_id, vk::Format::eR32G32B32Sfloat, offsetof(vertex, tangent) },
		// UV
		vk::VertexInputAttributeDescription{ 3, bind_id, vk::Format::eR32G32Sfloat, offsetof(vertex, uv) }
	};
	return attribute_description;
}

void mesh::bind(vk::CommandBuffer cmd, uint32_t bind_id) const
{
	cmd.bindVertexBuffer(bind_id, _buffer, _vertex_buffer_offset);
	cmd.bindIndexBuffer(_buffer, _index_buffer_offset, vk::IndexType::eUint32);
}

void mesh::draw(vk::CommandBuffer cmd) const
{
	cmd.drawIndexed(_index_count, 1, 0, 0, 0);
}
