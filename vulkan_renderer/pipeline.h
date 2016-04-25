#pragma once
#include "vulkan_include.h"
#include "texture.h"

#include <memory>

class renderer;
class managed_descriptor_set;

class pipeline
{
public:

	struct description
	{
		std::vector<vk::VertexInputAttributeDescription> vertex_input_attributes;
		vk::VertexInputBindingDescription vertex_input_bindings;
		vk::Viewport viewport;
		vk::Rect2D scissor;
		std::vector<std::vector<vk::DescriptorSetLayoutBinding>> descriptor_set_layouts_description;
		std::vector<uint32_t> descriptor_sets_pool_sizes;
	};

	pipeline(renderer& renderer, vk::RenderPass render_pass, const description& description);

	operator vk::Pipeline() const { return _pipeline; }
	auto pipeline_layout() const { return _pipeline_layout; }
	auto& descriptor_set_layouts() const { return _descriptor_set_layouts;	}

	void recycle(uint32_t index, vk::DescriptorSet set)
	{
		_descriptor_pools[index].free_sets.push_back(set);
	}
	std::unique_ptr<managed_descriptor_set> allocate(uint32_t index);
	texture* depth_buffer() const { return _depth_buffer.get(); }
	void create_depth_buffer(const texture::description& desc);

	~pipeline();
	
private:
	renderer& _renderer;

	vk::PipelineLayout _pipeline_layout;
	vk::Pipeline _pipeline;
	vk::RenderPass _render_pass;
	
	std::vector<vk::DescriptorSetLayout> _descriptor_set_layouts;
	std::vector<uint32_t> _pool_sizes;

	class descriptor_pool_manager
	{
	public:
		descriptor_pool_manager()
		{
		}
		vk::DescriptorPool pool;
		uint32_t allocated = 0;
		std::vector<vk::DescriptorSet> free_sets;
	};

	std::vector<descriptor_pool_manager> _descriptor_pools;

	std::unique_ptr<texture> _depth_buffer = nullptr;
};



class managed_descriptor_set
{
public:
	managed_descriptor_set(const vk::DescriptorSet& vk_descriptor_set_t, uint32_t set_index, pipeline& owner) : _descriptor_set(vk_descriptor_set_t), _set_index(set_index), _owner(owner)
	{
	}

	uint32_t set_index() const { return _set_index; }
	operator vk::DescriptorSet() const { return _descriptor_set; }
	~managed_descriptor_set()
	{
		if (_descriptor_set)
			release();
	}

private:
	
	void release()
	{
		_owner.recycle(_set_index, _descriptor_set);
		_descriptor_set = VK_NULL_HANDLE;
	}

	vk::DescriptorSet _descriptor_set;
	uint32_t _set_index;
	pipeline& _owner;
};