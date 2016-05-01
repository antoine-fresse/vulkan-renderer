#include "model.h"

#include "shared.h"

#include "renderer.h"
#include "pipeline.h"
#include "camera.h"

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

	uint32_t flags = aiProcess_CalcTangentSpace | aiProcess_Triangulate | aiProcess_GenNormals | aiProcess_SortByPType;
	const aiScene* scene = importer.ReadFile(filepath, flags);
	
	if (!scene)
		throw renderer_exception("Cannot load mesh from file : " + filepath);

	auto model = glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, 0.0f, 0.0f));
	_uniform_object.model_matrix = model;

	std::vector<vertex> vertices;
	std::vector<uint32_t> indices;

	std::vector<int32_t> material_assoc(scene->mNumMaterials, -1);

	for (uint32_t i = 0; i < scene->mNumTextures; ++i)
	{
		if(scene->mTextures[i]->mHeight == 0)
			_renderer.tex_manager().create_texture_from_file_buffer(filepath + "*" + std::to_string(i), scene->mTextures[i]->pcData, scene->mTextures[i]->mWidth);
		else
			_renderer.tex_manager().create_texture_from_rgba_buffer(filepath + "*" + std::to_string(i), scene->mTextures[i]->pcData, scene->mTextures[i]->mWidth, scene->mTextures[i]->mHeight);
	}

	for (uint32_t i = 0; i < scene->mNumMaterials; ++i)
	{
		aiMaterial* mat = scene->mMaterials[i];
		aiString diffuse_path;
		aiString spec_path;
		aiString normal_path;

		int success = 3;

		material_assoc[i] = _materials.size();

		_materials.emplace_back();

		auto& material = _materials.back();
		material.mat_info.normal_map_intensity = -1.0f;
		material.mat_info.specular_intensity = -1.0f;

		if (mat->GetTexture(aiTextureType_DIFFUSE, 0, &diffuse_path) == AI_SUCCESS)
		{
			const char* path = diffuse_path.C_Str();
			material.diffuse_texture = _renderer.tex_manager().create_texture_from_file(path[0] == '*' ? filepath + path : path);
			material.mat_info.diffuse_color.a = 1.0f;
		}
		else
		{
			material.diffuse_texture = _renderer.tex_manager().create_texture_from_file("missing_texture.png");
			material.mat_info.diffuse_color.a = -1.0f;
		}
		if (mat->GetTexture(aiTextureType_NORMALS, 0, &normal_path) == AI_SUCCESS)
		{
			const char* path = normal_path.C_Str();
			material.normal_texture = _renderer.tex_manager().create_texture_from_file(path[0] == '*' ? filepath + path : path);
			material.mat_info.normal_map_intensity = 1.0f;
		}
		else
		{
			material.normal_texture = _renderer.tex_manager().create_texture_from_file("missing_texture.png");
		}
		if (mat->GetTexture(aiTextureType_SPECULAR, 0, &spec_path) == AI_SUCCESS)
		{
			const char* path = spec_path.C_Str();
			material.specular_texture = _renderer.tex_manager().create_texture_from_file(path[0] == '*' ? filepath + path : path);
			mat->Get(AI_MATKEY_SHININESS, material.mat_info.specular_intensity);
		}
		else
		{
			material.specular_texture = _renderer.tex_manager().create_texture_from_file("missing_texture.png");
		}

		aiColor3D color(1.0f,1.0f,1.0f);
		if(mat->Get(AI_MATKEY_COLOR_AMBIENT, color) == AI_SUCCESS)
		{
			material.mat_info.ambient_color.r = color.r;
			material.mat_info.ambient_color.g = color.g;
			material.mat_info.ambient_color.b = color.b;
		}
		
		color = { 1.0f, 1.0f, 1.0f };
		if (mat->Get(AI_MATKEY_COLOR_DIFFUSE, color) == AI_SUCCESS)
		{
			material.mat_info.diffuse_color.r = color.r;
			material.mat_info.diffuse_color.g = color.g;
			material.mat_info.diffuse_color.b = color.b;
		}

		color = { 1.0f, 1.0f, 1.0f };
		if (mat->Get(AI_MATKEY_COLOR_SPECULAR, color) == AI_SUCCESS)
		{
			material.mat_info.specular_color.r = color.r;
			material.mat_info.specular_color.g = color.g;
			material.mat_info.specular_color.b = color.b;
			material.mat_info.specular_color.a = 1.0f;
		}

	}
	

	for (uint32_t i = 0; i < scene->mNumMeshes; ++i)
	{
		aiMesh* i_mesh = scene->mMeshes[i];
		
		if ((i_mesh->mPrimitiveTypes & aiPrimitiveType_TRIANGLE) == 0) continue;


		const float fmax = std::numeric_limits<float>::max();
		const float fmin = std::numeric_limits<float>::min();

		std::pair<glm::vec3, glm::vec3> bbox(glm::vec3(fmax, fmax, fmax), glm::vec3(fmin, fmin, fmin));
		
		uint32_t current_mesh_vertex_offset = vertices.size();
		for (uint32_t k = 0; k < i_mesh->mNumVertices; ++k)
		{
			vertex vert;
			memcpy(&vert.position, &i_mesh->mVertices[k], sizeof(glm::vec3));
			vert.position *= scale;

			bbox.first = glm::min(vert.position, bbox.first);
			bbox.second = glm::max(vert.position, bbox.second);

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

			/* 
			// Manual calc of tangents
			vertex& v0 = vertices[current_mesh_vertex_offset + i_mesh->mFaces[k].mIndices[0]];
			vertex& v1 = vertices[current_mesh_vertex_offset + i_mesh->mFaces[k].mIndices[1]];
			vertex& v2 = vertices[current_mesh_vertex_offset + i_mesh->mFaces[k].mIndices[2]];
			glm::vec3 edge_1 = v1.position - v0.position;
			glm::vec3 edge_2 = v2.position - v0.position;

			float delta_u1 = v1.uv.x - v0.uv.x;
			float delta_v1 = v1.uv.y - v0.uv.y;
			float delta_u2 = v2.uv.x - v0.uv.x;
			float delta_v2 = v2.uv.y - v0.uv.y;

			float f = 1.0f / (delta_u1*delta_v2 - delta_u2*delta_v1);

			glm::vec3 tangent;
			tangent.x = f*(delta_v2*edge_1.x - delta_v1*edge_2.x);
			tangent.y = f*(delta_v2*edge_1.y - delta_v1*edge_2.y);
			tangent.z = f*(delta_v2*edge_1.z - delta_v1*edge_2.z);

			v0.tangent += tangent;
			v1.tangent += tangent;
			v2.tangent += tangent;
			*/

		}
		
		std::pair<glm::vec3, float> bsphere((bbox.first + bbox.second)*0.5f, glm::distance(bbox.first, bbox.second)/2 );
		_meshes.emplace_back(current_mesh_vertex_offset*sizeof(vertex), current_mesh_index_offset*sizeof(uint32_t), i_mesh->mNumFaces * 3, material_assoc[i_mesh->mMaterialIndex], bsphere, bbox);
	}

	for (auto& vert : vertices)
	{
		vert.tangent = glm::normalize(vert.tangent);
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
		m.vertex_buffer_offset += vertex_buffer_global_offset;

	// Indices
	memcpy(dst, indices.data(), index_count*sizeof(uint32_t));
	// Vertices
	memcpy(dst + vertex_buffer_global_offset, vertices.data(), vertices.size()*sizeof(vertex));
	// Uniform
	memcpy(dst + _uniform_buffer_offset, &_uniform_object, sizeof(uniform_object));

	device.unmapMemory(_memory);

	device.bindBufferMemory(_buffer, _memory, 0);

	std::sort(_meshes.begin(), _meshes.end(), [](const mesh& m1, const mesh& m2) { return m1.material_index < m2.material_index; });
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

void model::draw(const vk::CommandBuffer& cmd, pipeline& pipeline, const camera& camera, uint32_t bind_id) const
{
	int last_m_index = -1;
	for(auto& m : _meshes)
	{
		if (camera.cull_sphere(m.bounding_sphere)) continue;

		//if (m.material_index != last_m_index)
		{
			vk::DescriptorSet set = *_materials[m.material_index].textures_set;
			cmd.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, pipeline.pipeline_layout(), 2, 1, &set, 0, nullptr);
			cmd.pushConstant(pipeline.pipeline_layout(), vk::ShaderStageFlagBits::eFragment, 0, _materials[m.material_index].mat_info);
			last_m_index = m.material_index;
		}

		cmd.bindVertexBuffer(bind_id, _buffer, m.vertex_buffer_offset);
		cmd.bindIndexBuffer(_buffer, m.index_buffer_offset, vk::IndexType::eUint32);
		cmd.drawIndexed(m.index_count, 1, 0, 0, 0);
	}
}

void model::attach_textures(pipeline& pipeline, uint32_t set_index)
{
	for(auto& mat : _materials)
	{
		mat.textures_set = pipeline.allocate(set_index);
		std::vector<vk::DescriptorImageInfo> image_info;
		image_info.reserve(3);

		std::vector<vk::WriteDescriptorSet> writes;
		if (mat.diffuse_texture)
		{
			image_info.push_back(mat.diffuse_texture->descriptor_image_info());
			writes.push_back(vk::WriteDescriptorSet{ *mat.textures_set, 0, 0, 1, vk::DescriptorType::eCombinedImageSampler, &image_info.back(), nullptr, nullptr });
		}
			
		if (mat.normal_texture)
		{
			image_info.push_back(mat.normal_texture->descriptor_image_info());
			writes.push_back(vk::WriteDescriptorSet{ *mat.textures_set, 1, 0, 1, vk::DescriptorType::eCombinedImageSampler, &image_info.back(), nullptr, nullptr });

		}
		if (mat.specular_texture)
		{
			image_info.push_back(mat.specular_texture->descriptor_image_info());
			writes.push_back(vk::WriteDescriptorSet{ *mat.textures_set, 2, 0, 1, vk::DescriptorType::eCombinedImageSampler, &image_info.back(), nullptr, nullptr });
		}

		_renderer.device().updateDescriptorSets((uint32_t)writes.size(), writes.data(), 0, nullptr);
	}
}
