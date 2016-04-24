#include "renderer.h"
#include "camera.h"
#include "model.h"
#include "texture.h"

#include <chrono>

int main()
{
	

	std::vector<const char*> instance_layers;
	std::vector<const char*> device_layers;
	std::vector<const char*> instance_extensions;
	std::vector<const char*> device_extensions;

	instance_layers.push_back("VK_LAYER_RENDERDOC_Capture");
	device_layers.push_back("VK_LAYER_RENDERDOC_Capture");
	
	//try
	{
		renderer renderer{ 800, 600, 3, instance_layers , instance_extensions, device_layers, device_extensions };
		model test_mesh{ "data/nanosuit.obj", renderer, 0.5f };
		texture tex{ "data/arm_dif.png", renderer };

		camera cam(renderer);

		vk::Device device = renderer.device();
		auto swapchain_images = renderer.swapchain_images();

		vk::ClearColorValue clear_color[] = {   { std::array<float, 4>{1.0f, 0.0f, 1.0f, 0.0f} },
												{ std::array<float, 4>{1.0f, 0.0f, 1.0f, 0.0f} },
												{ std::array<float, 4>{1.0f, 0.0f, 1.0f, 0.0f} } };

		vk::ImageSubresourceRange img_subresource_range{ vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1 };


		vk::AttachmentDescription attachment_descriptions[]{ vk::AttachmentDescription{ {}, renderer.format(), vk::SampleCountFlagBits::e1, vk::AttachmentLoadOp::eClear, vk::AttachmentStoreOp::eStore, vk::AttachmentLoadOp::eDontCare, vk::AttachmentStoreOp::eDontCare, vk::ImageLayout::ePresentSrcKHR, vk::ImageLayout::ePresentSrcKHR } };
		vk::AttachmentReference color_attachment_references[]{ vk::AttachmentReference{ 0, vk::ImageLayout::eColorAttachmentOptimal } };
		vk::SubpassDescription subpass_descriptions[]{ vk::SubpassDescription{ {}, vk::PipelineBindPoint::eGraphics, 0, nullptr, 1, color_attachment_references,nullptr,nullptr,0,nullptr } };

		auto render_pass = device.createRenderPass({ {}, 1, attachment_descriptions, 1, subpass_descriptions, 0, nullptr });

		std::vector<vk::ImageView> views(swapchain_images.size());
		std::vector<vk::Framebuffer> framebuffers(swapchain_images.size());
		for (uint32_t i = 0; i < swapchain_images.size(); ++i)
		{
			vk::ImageViewCreateInfo	view_create_info{ {}, swapchain_images[i], vk::ImageViewType::e2D, renderer.format(), vk::ComponentMapping{}, img_subresource_range };

			views[i] = device.createImageView(view_create_info);


			vk::FramebufferCreateInfo framebuffer_create_info{ {}, render_pass, 1, &views[i], 800, 600, 1 };

			framebuffers[i] = device.createFramebuffer(framebuffer_create_info);
		}

		// TODO(antoine) destroy both
		auto vertex_shader_module = renderer.load_shader("vert.spv");
		auto fragment_shader_module = renderer.load_shader("frag.spv");
		
		std::vector<vk::PipelineShaderStageCreateInfo> shader_stages_ci{
			vk::PipelineShaderStageCreateInfo{ {}, vk::ShaderStageFlagBits::eVertex, vertex_shader_module, "main", nullptr},
			vk::PipelineShaderStageCreateInfo{ {}, vk::ShaderStageFlagBits::eFragment, fragment_shader_module, "main", nullptr },
		};

		auto attribute_desc = model::attribute_descriptions(0);
		auto binding_desc = model::binding_description(0);

		vk::PipelineVertexInputStateCreateInfo vertex_input_state_create_info{ {}, 1, &binding_desc, (uint32_t)attribute_desc.size(), attribute_desc.data() };

		vk::PipelineInputAssemblyStateCreateInfo input_assembly_state_create_info{ {}, vk::PrimitiveTopology::eTriangleList, VK_FALSE };


		vk::Viewport viewport{ 0.0f, 0.0f, 800, 600.0f, 0.0f, 1.0f };
		
		vk::Rect2D scissor{ {0,0},{800, 600} };

		vk::PipelineViewportStateCreateInfo viewport_state_create_info{ {}, 1, &viewport, 1, &scissor };

		vk::PipelineRasterizationStateCreateInfo rasterization_state_create_info{ {}, VK_FALSE, VK_FALSE, vk::PolygonMode::eFill, vk::CullModeFlagBits::eBack, vk::FrontFace::eCounterClockwise, VK_FALSE, 0.0f, 0.0f, 0.0f, 1.0f };

		vk::PipelineMultisampleStateCreateInfo multisample_state_create_info{ {}, vk::SampleCountFlagBits::e1, VK_FALSE, 1.0f, nullptr, VK_FALSE, VK_FALSE };

		vk::PipelineColorBlendAttachmentState color_blend_attachment_state{ VK_FALSE, vk::BlendFactor::eOne, vk::BlendFactor::eZero, vk::BlendOp::eAdd, vk::BlendFactor::eOne, vk::BlendFactor::eZero, vk::BlendOp::eAdd, vk::ColorComponentFlagBits::eR | vk::ColorComponentFlagBits::eG | vk::ColorComponentFlagBits::eB| vk::ColorComponentFlagBits::eA };

		vk::PipelineColorBlendStateCreateInfo color_blend_state_create_info{ {}, VK_FALSE, vk::LogicOp::eCopy, 1, &color_blend_attachment_state, {0.0f,0.0f,0.0f,0.0f} };


	
		vk::DescriptorSetLayoutBinding desc_layout_ubo_vertex { 0, vk::DescriptorType::eUniformBuffer, 1, vk::ShaderStageFlagBits::eVertex, nullptr };
		vk::DescriptorSetLayoutBinding desc_layout_sampler_fragment { 0, vk::DescriptorType::eCombinedImageSampler, 1, vk::ShaderStageFlagBits::eFragment, nullptr };

		
		vk::DescriptorSetLayoutCreateInfo descriptor_set_layout_create_info{};

		// TODO(antoine) destroy
		std::vector<vk::DescriptorSetLayout> descriptor_set_layout{ 
			device.createDescriptorSetLayout({ {}, 1, &desc_layout_ubo_vertex }), // View
			device.createDescriptorSetLayout({ {}, 1, &desc_layout_ubo_vertex }), // Model
			device.createDescriptorSetLayout({ {}, 1, &desc_layout_sampler_fragment }) // Sampler
		};
		
		vk::PipelineLayoutCreateInfo pipeline_layout_create_info{ {}, (uint32_t)descriptor_set_layout.size(), descriptor_set_layout.data(), 0, nullptr };

		// TODO(antoine) destroy
		auto pipeline_layout = device.createPipelineLayout(pipeline_layout_create_info);

		vk::GraphicsPipelineCreateInfo graphics_pipeline_create_info{ {}, (uint32_t)shader_stages_ci.size(), shader_stages_ci.data(), &vertex_input_state_create_info, &input_assembly_state_create_info, nullptr, &viewport_state_create_info,&rasterization_state_create_info, &multisample_state_create_info, nullptr, &color_blend_state_create_info, nullptr, pipeline_layout, render_pass, 0, VK_NULL_HANDLE, -1 };
		
		// TODO(antoine) destroy
		auto graphics_pipeline = device.createGraphicsPipeline(VK_NULL_HANDLE, graphics_pipeline_create_info);
		
		// TODO(antoine) destroy
		auto command_pool = device.createCommandPool({ {}, renderer.graphics_family_index() });

		

		std::vector<vk::DescriptorPoolSize> descriptor_pool_size{ 
			vk::DescriptorPoolSize{ vk::DescriptorType::eUniformBuffer, 2},
			vk::DescriptorPoolSize{ vk::DescriptorType::eCombinedImageSampler, 1 },
		};
		
		vk::DescriptorPoolCreateInfo descriptor_pool_create_info{ {}, (uint32_t)descriptor_set_layout.size(), (uint32_t)descriptor_pool_size.size(), descriptor_pool_size.data() };

		// TODO(antoine) destroy
		auto descriptor_pool = device.createDescriptorPool(descriptor_pool_create_info);

		vk::DescriptorSetAllocateInfo descriptor_set_allocate_info{ descriptor_pool, (uint32_t)descriptor_set_layout.size(), descriptor_set_layout.data() };

		std::vector<vk::DescriptorSet> descriptor_sets = device.allocateDescriptorSets(descriptor_set_allocate_info);
		
		auto descriptor_buffer_info_model = test_mesh.descriptor_buffer_info();
		auto descriptor_buffer_info_camera = cam.descriptor_buffer_info();
		auto descriptor_image_info = tex.descriptor_image_info();

		std::vector<vk::WriteDescriptorSet> descriptor_writes{ 
			vk::WriteDescriptorSet{ descriptor_sets[0], 0, 0, 1, vk::DescriptorType::eUniformBuffer, nullptr, &descriptor_buffer_info_camera, nullptr },
			vk::WriteDescriptorSet{ descriptor_sets[1], 0, 0, 1, vk::DescriptorType::eUniformBuffer, nullptr, &descriptor_buffer_info_model, nullptr },
			vk::WriteDescriptorSet{ descriptor_sets[2], 0, 0, 1, vk::DescriptorType::eCombinedImageSampler, &descriptor_image_info, nullptr, nullptr },
		};
		device.updateDescriptorSets(descriptor_writes, {});


		auto& render_cmd_buffers = renderer.render_command_buffers();

		vk::CommandBufferBeginInfo begin_info{ vk::CommandBufferUsageFlagBits::eSimultaneousUse, nullptr };

		for (uint32_t i = 0; i < swapchain_images.size(); ++i)
		{
			const vk::CommandBuffer& cmd = render_cmd_buffers[i];

			cmd.begin(begin_info);
			// cmd.pushConstant(pipeline_layout, vk::ShaderStageFlagBits::eVertex, 0, mvp);
			
			uint32_t src_queue = VK_QUEUE_FAMILY_IGNORED;
			uint32_t dst_queue = VK_QUEUE_FAMILY_IGNORED;
			

			if (renderer.graphics_family_index() != renderer.present_family_index())
			{
				src_queue = renderer.present_family_index();
				dst_queue = renderer.graphics_family_index();
			}

			vk::ImageMemoryBarrier barrier_present_to_draw{ vk::AccessFlagBits::eMemoryRead, vk::AccessFlagBits::eColorAttachmentWrite, vk::ImageLayout::eUndefined, vk::ImageLayout::ePresentSrcKHR, src_queue, dst_queue, swapchain_images[i], img_subresource_range };
			cmd.pipelineBarrier(vk::PipelineStageFlagBits::eColorAttachmentOutput, vk::PipelineStageFlagBits::eColorAttachmentOutput, vk::DependencyFlags{}, 0, nullptr, 0, nullptr, 1, &barrier_present_to_draw);

			
			vk::ClearValue clear_value{ clear_color[i] };
			vk::RenderPassBeginInfo render_pass_bi{ render_pass, framebuffers[i], vk::Rect2D{ { 0,0 },{ 800, 600 } }, 1, &clear_value };

			cmd.beginRenderPass(render_pass_bi, vk::SubpassContents::eInline);

			cmd.bindPipeline(vk::PipelineBindPoint::eGraphics, graphics_pipeline);

			cmd.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, pipeline_layout, 0, (uint32_t)descriptor_sets.size(), descriptor_sets.data(), 0, nullptr);

			test_mesh.draw(cmd, pipeline_layout);

			cmd.endRenderPass();


			std::swap(src_queue, dst_queue);
			
			vk::ImageMemoryBarrier barrier_draw_to_present{ vk::AccessFlagBits::eColorAttachmentWrite, vk::AccessFlagBits::eMemoryRead, vk::ImageLayout::ePresentSrcKHR, vk::ImageLayout::ePresentSrcKHR, src_queue, dst_queue, swapchain_images[i], img_subresource_range };
			cmd.pipelineBarrier(vk::PipelineStageFlagBits::eColorAttachmentOutput, vk::PipelineStageFlagBits::eBottomOfPipe, vk::DependencyFlags{}, 0, nullptr, 0, nullptr, 1, &barrier_draw_to_present);

			cmd.end();
		}
		
		
		
		while (!glfwWindowShouldClose(renderer.window_handle()))
		{
			renderer.render();
			renderer.present();
			glfwPollEvents();
		}
	}
	
}
