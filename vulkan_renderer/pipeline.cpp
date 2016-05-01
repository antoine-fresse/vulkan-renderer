#include "pipeline.h"

#include "shared.h"

#include "renderer.h"
#include <set>
#include "model.h"


pipeline::pipeline(renderer& renderer, vk::RenderPass render_pass, const description& description) : _renderer(renderer), _render_pass(render_pass)
{
	vk::ShaderModule vertex_shader_module = _renderer.load_shader("vert.spv");
	vk::ShaderModule fragment_shader_module = _renderer.load_shader("frag.spv");

	std::vector<vk::PipelineShaderStageCreateInfo> shader_stages_ci{
		vk::PipelineShaderStageCreateInfo{ {}, vk::ShaderStageFlagBits::eVertex, vertex_shader_module, "main", nullptr },
		vk::PipelineShaderStageCreateInfo{ {}, vk::ShaderStageFlagBits::eFragment, fragment_shader_module, "main", nullptr },
	};

	vk::PipelineVertexInputStateCreateInfo vertex_input_state_create_info{ {}, 1, &description.vertex_input_bindings, (uint32_t)description.vertex_input_attributes.size(), description.vertex_input_attributes.data() };
	vk::PipelineInputAssemblyStateCreateInfo input_assembly_state_create_info{ {}, vk::PrimitiveTopology::eTriangleList, VK_FALSE };

	vk::PipelineViewportStateCreateInfo viewport_state_create_info{ {}, 1, &description.viewport, 1, &description.scissor };
	vk::PipelineRasterizationStateCreateInfo rasterization_state_create_info{ {}, VK_FALSE, VK_FALSE, vk::PolygonMode::eFill, vk::CullModeFlagBits::eBack, vk::FrontFace::eCounterClockwise, VK_FALSE, 0.0f, 0.0f, 0.0f, 1.0f };
	vk::PipelineMultisampleStateCreateInfo multisample_state_create_info{ {}, description.samples, VK_FALSE, 0.0f, nullptr, VK_FALSE, VK_FALSE };
	vk::PipelineColorBlendAttachmentState color_blend_attachment_state{ VK_FALSE, vk::BlendFactor::eOne, vk::BlendFactor::eZero, vk::BlendOp::eAdd, vk::BlendFactor::eOne, vk::BlendFactor::eZero, vk::BlendOp::eAdd, vk::ColorComponentFlagBits::eR | vk::ColorComponentFlagBits::eG | vk::ColorComponentFlagBits::eB | vk::ColorComponentFlagBits::eA };
	vk::PipelineColorBlendStateCreateInfo color_blend_state_create_info{ {}, VK_FALSE, vk::LogicOp::eCopy, 1, &color_blend_attachment_state,{ 0.0f,0.0f,0.0f,0.0f } };

	for(auto& layout : description.descriptor_set_layouts_description)
		_descriptor_set_layouts.push_back(_renderer.device().createDescriptorSetLayout(vk::DescriptorSetLayoutCreateInfo{ {}, (uint32_t)layout.size(), layout.data() }));

	vk::PipelineLayoutCreateInfo pipeline_layout_create_info{ {}, (uint32_t)_descriptor_set_layouts.size(), _descriptor_set_layouts.data(), (uint32_t)description.push_constants.size(), description.push_constants.data() };

	_pipeline_layout = _renderer.device().createPipelineLayout(pipeline_layout_create_info);

	vk::StencilOpState back_stencil_state{};
	back_stencil_state.compareOp(vk::CompareOp::eAlways);

	vk::PipelineDepthStencilStateCreateInfo depth_stencil_create_info{ {}, VK_TRUE, VK_TRUE, vk::CompareOp::eLessOrEqual, VK_FALSE, VK_FALSE, vk::StencilOpState{}, back_stencil_state, 0.0f, 0.0f };

	vk::GraphicsPipelineCreateInfo graphics_pipeline_create_info{ {}, (uint32_t)shader_stages_ci.size(), shader_stages_ci.data(), &vertex_input_state_create_info, &input_assembly_state_create_info, nullptr, &viewport_state_create_info,&rasterization_state_create_info, &multisample_state_create_info, &depth_stencil_create_info, &color_blend_state_create_info, nullptr, _pipeline_layout, render_pass, 0, VK_NULL_HANDLE, -1 };

	_pipeline = _renderer.device().createGraphicsPipeline(_renderer.pipeline_cache(), graphics_pipeline_create_info);

	// TODO(antoine) implement auto deleter/defer mechanism
	_renderer.device().destroyShaderModule(vertex_shader_module);
	_renderer.device().destroyShaderModule(fragment_shader_module);

	_pool_sizes = description.descriptor_sets_pool_sizes;

	for (uint32_t i = 0; i < description.descriptor_set_layouts_description.size(); ++i)
	{
		std::vector<vk::DescriptorPoolSize> pool_sizes;
		std::set<vk::DescriptorType> types_present;

		for(auto& binding : description.descriptor_set_layouts_description[i])
		{
			if(types_present.find(binding.descriptorType()) == types_present.end())
			{
				pool_sizes.push_back(vk::DescriptorPoolSize{ binding.descriptorType(), _pool_sizes[i] });
				types_present.insert(binding.descriptorType());
			}
		}

		vk::DescriptorPoolCreateInfo pool_ci{ {}, _pool_sizes[i], (uint32_t)pool_sizes.size(), pool_sizes.data() };
	
		_descriptor_pools.emplace_back();
		_descriptor_pools.back().pool = _renderer.device().createDescriptorPool(pool_ci);
	}

}

pipeline::~pipeline()
{
	for(auto& manager : _descriptor_pools)
	{
		_renderer.device().destroyDescriptorPool(manager.pool);
	}
	
	_renderer.device().destroyPipeline(_pipeline);
	_renderer.device().destroyPipelineLayout(_pipeline_layout);
	for (auto& descriptor_set_layout : _descriptor_set_layouts)
	{
		_renderer.device().destroyDescriptorSetLayout(descriptor_set_layout);
	}
}

std::unique_ptr<managed_descriptor_set> pipeline::allocate(uint32_t index)
{
	descriptor_pool_manager& manager = _descriptor_pools[index];

	vk::DescriptorSet set;

	if (manager.free_sets.size())
	{
		set = manager.free_sets.back();
		manager.free_sets.pop_back();
	}
	else
	{
		vk::Device device = _renderer.device();
		set = device.allocateDescriptorSets(vk::DescriptorSetAllocateInfo{ _descriptor_pools[index].pool, 1, &_descriptor_set_layouts[index] }).front();
		++manager.allocated;
	}

	return std::make_unique<managed_descriptor_set>(set, index, *this);
}

void pipeline::create_depth_buffer(const texture::description& desc)
{
	_depth_buffer = std::make_unique<texture>(desc, _renderer);
}
