#include "model.h"

#include "shared.h"

#include "renderer.h"
#include "pipeline.h"

#include <assimp/Importer.hpp>
#include <assimp/scene.h>           // Output data structure
#include <assimp/postprocess.h>     // Post processing flags


model::model(const std::string& filepath, renderer& renderer, float scale) : _renderer(renderer)
{
	load_model(filepath, scale);
}

model::~model()
{
	if(_buffer)
	{
		_renderer.device().destroyBuffer(_buffer);
		_renderer.device().freeMemory(_memory);
	}
}


void model::update(double dt)
{
	vk::Device device = _renderer.device();
	_uniform_object.model_matrix = glm::rotate(_uniform_object.model_matrix, (float)(dt*glm::pi<double>()/8.0), glm::vec3(0, 1, 0));
	void* mapped_ubo = device.mapMemory(_memory, _uniform_buffer_offset, sizeof(uniform_object), {});
	memcpy(mapped_ubo, &_uniform_object, sizeof(uniform_object));
	device.unmapMemory(_memory);
}

void model::load_model(const std::string& filepath, float scale)
{
	Assimp::Importer importer;

	uint32_t flags = aiProcess_CalcTangentSpace | aiProcess_Triangulate | aiProcess_GenNormals | aiProcess_OptimizeMeshes | aiProcess_SortByPType | aiProcess_RemoveRedundantMaterials;
	const aiScene* scene = importer.ReadFile(filepath, flags);

	if (!scene)
		throw renderer_exception("Cannot load mesh from file : " + filepath);

	auto model = glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, 0.0f, 0.0f));
	_uniform_object.model_matrix = model;

	std::vector<vertex> vertices;
	std::vector<uint32_t> indices;

	std::vector<int32_t> material_assoc(scene->mNumMaterials, -1);

	for (uint32_t i = 0; i < scene->mNumMaterials; ++i)
	{
		aiMaterial* mat = scene->mMaterials[i];
		aiString diffuse_path;
		aiString spec_path;
		aiString normal_path;

		int success = 3;

		
		if (mat->GetTexture(aiTextureType_DIFFUSE, 0, &diffuse_path) != AI_SUCCESS) --success;
		if (mat->GetTexture(aiTextureType_NORMALS, 0, &normal_path) != AI_SUCCESS) --success;
		if (mat->GetTexture(aiTextureType_SPECULAR, 0, &spec_path) != AI_SUCCESS) --success;
		

		material_assoc[i] = _materials.size();
		//_materials.push_back({_renderer.tex_manager().find_texture(diffuse_path.C_Str()), _renderer.tex_manager().find_texture(normal_path.C_Str()) ,_renderer.tex_manager().find_texture(spec_path.C_Str()) , VK_NULL_HANDLE});
		_materials.push_back({_renderer.tex_manager().find_texture(diffuse_path.C_Str()), nullptr , nullptr , VK_NULL_HANDLE});
	}
	

	for (uint32_t i = 0; i < scene->mNumMeshes; ++i)
	{
		aiMesh* i_mesh = scene->mMeshes[i];
		
		if ((i_mesh->mPrimitiveTypes & aiPrimitiveType_TRIANGLE) == 0) continue;

		uint32_t current_mesh_vertex_offset = vertices.size();
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

		uint32_t current_mesh_index_offset = indices.size();
		for (uint32_t k = 0; k < i_mesh->mNumFaces; ++k)
		{
			indices.push_back(i_mesh->mFaces[k].mIndices[0]);
			indices.push_back(i_mesh->mFaces[k].mIndices[1]);
			indices.push_back(i_mesh->mFaces[k].mIndices[2]);
		}

		_meshes.emplace_back(current_mesh_vertex_offset*sizeof(vertex), current_mesh_index_offset*sizeof(uint32_t), i_mesh->mNumFaces * 3, material_assoc[i_mesh->mMaterialIndex]);
	}

	vk::Device device = _renderer.device();
	
	vk::DeviceSize size = vertices.size()*sizeof(vertex) + indices.size()*sizeof(uint32_t);
	
	size = _renderer.ubo_aligned_size(size);
	
	_uniform_buffer_offset = size;
	
	size += sizeof(uniform_object);

	vk::BufferUsageFlags usage_flags = vk::BufferUsageFlagBits::eUniformBuffer | vk::BufferUsageFlagBits::eVertexBuffer | vk::BufferUsageFlagBits::eIndexBuffer;
	vk::BufferCreateInfo buffer_ci{ {}, size, usage_flags, vk::SharingMode::eExclusive, 0, nullptr };
	_buffer = device.createBuffer(buffer_ci);
	_memory_reqs = device.getBufferMemoryRequirements(_buffer);

	vk::MemoryAllocateInfo mem_allocate_info{ _memory_reqs.size(), _renderer.find_adequate_memory(_memory_reqs, vk::MemoryPropertyFlagBits::eHostVisible) };
	_memory = device.allocateMemory(mem_allocate_info);


	char* dst = (char*)device.mapMemory(_memory, 0, _memory_reqs.size(), {});

	
	
	uint32_t index_count = indices.size();
	uint32_t vertex_buffer_global_offset = index_count*sizeof(uint32_t);

	for (auto& m : _meshes)
		m.vertex_buffer_offset(m.vertex_buffer_offset() + vertex_buffer_global_offset);

	// Indices
	memcpy(dst, indices.data(), index_count*sizeof(uint32_t));
	// Vertices
	memcpy(dst + vertex_buffer_global_offset, vertices.data(), vertices.size()*sizeof(vertex));
	// Uniform
	memcpy(dst + _uniform_buffer_offset, &_uniform_object, sizeof(uniform_object));

	device.unmapMemory(_memory);

	device.bindBufferMemory(_buffer, _memory, 0);
}

vk::VertexInputBindingDescription model::binding_description(uint32_t bind_id)
{
	return { bind_id, sizeof(vertex), vk::VertexInputRate::eVertex };
}

std::vector<vk::VertexInputAttributeDescription> model::attribute_descriptions(uint32_t bind_id)
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

void model::draw(const vk::CommandBuffer& cmd, pipeline& pipeline, uint32_t bind_id) const
{
	for(auto& m : _meshes)
	{
		vk::DescriptorSet set = *_materials[m.material_index()].textures_set;
		cmd.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, pipeline.pipeline_layout(), 2, 1, &set, 0, nullptr);
		cmd.bindVertexBuffer(bind_id, _buffer, m.vertex_buffer_offset());
		cmd.bindIndexBuffer(_buffer, m.index_buffer_offset(), vk::IndexType::eUint32);
		cmd.drawIndexed(m.index_count(), 1, 0, 0, 0);
	}
}

void model::attach_textures(pipeline& pipeline, uint32_t set_index)
{
	for(auto& mat : _materials)
	{
		mat.textures_set = pipeline.allocate(set_index);
		std::vector<vk::DescriptorImageInfo> image_info{
			mat.diffuse_texture->descriptor_image_info(),
			//mat.normal_texture->descriptor_image_info(),
			//mat.specular_texture->descriptor_image_info(),
		};
		vk::WriteDescriptorSet write{ *mat.textures_set, 0, 0, (uint32_t)image_info.size(), vk::DescriptorType::eCombinedImageSampler, image_info.data(), nullptr, nullptr };
		_renderer.device().updateDescriptorSets(1, &write, 0, nullptr);
	}
}
