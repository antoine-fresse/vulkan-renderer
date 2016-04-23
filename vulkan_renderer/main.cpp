#include "vulkan_renderer.h"

#include "math_include.h"
#include "mesh.h"

#include <chrono>

int main()
{
	

	std::vector<const char*> instance_layers;
	std::vector<const char*> device_layers;
	std::vector<const char*> instance_extensions;
	std::vector<const char*> device_extensions;

	//instance_layers.push_back("VK_LAYER_RENDERDOC_Capture");
	//device_layers.push_back("VK_LAYER_RENDERDOC_Capture");
	
	//try
	{
		vulkan_renderer renderer{ 800, 600, 3, instance_layers , instance_extensions, device_layers, device_extensions };
		mesh test_mesh{ "data/nanosuit.obj", renderer, 0.5f };
		
		auto device = renderer.device();
		auto swapchain_images = renderer.swapchain_images();

		vk::ClearColorValue clear_color[] = {   { std::array<float, 4>{1.0f, 0.0f, 1.0f, 0.0f} },
												{ std::array<float, 4>{1.0f, 0.0f, 1.0f, 0.0f} },
												{ std::array<float, 4>{1.0f, 0.0f, 1.0f, 0.0f} } };

		vk::ImageSubresourceRange img_subresource_range{ vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1 };


		vk::AttachmentDescription attachment_descriptions[]{ { {}, renderer.format(), vk::SampleCountFlagBits::e1, vk::AttachmentLoadOp::eClear, vk::AttachmentStoreOp::eStore, vk::AttachmentLoadOp::eDontCare, vk::AttachmentStoreOp::eDontCare, vk::ImageLayout::ePresentSrcKHR, vk::ImageLayout::ePresentSrcKHR } };
		vk::AttachmentReference color_attachment_references[]{ { 0, vk::ImageLayout::eColorAttachmentOptimal } };
		vk::SubpassDescription subpass_descriptions[]{ { {}, vk::PipelineBindPoint::eGraphics, 0, nullptr, 1, color_attachment_references,nullptr,nullptr,0,nullptr } };

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

		auto attribute_desc = mesh::attribute_descriptions(0);
		auto binding_desc = mesh::binding_description(0);

		vk::PipelineVertexInputStateCreateInfo vertex_input_state_create_info{ {}, 1, &binding_desc, (uint32_t)attribute_desc.size(), attribute_desc.data() };

		vk::PipelineInputAssemblyStateCreateInfo input_assembly_state_create_info{ {}, vk::PrimitiveTopology::eTriangleList, VK_FALSE };


		vk::Viewport viewport{ 0.0f, 0.0f, 800, 600.0f, 0.0f, 1.0f };
		
		vk::Rect2D scissor{ {0,0},{800, 600} };

		vk::PipelineViewportStateCreateInfo viewport_state_create_info{ {}, 1, &viewport, 1, &scissor };

		vk::PipelineRasterizationStateCreateInfo rasterization_state_create_info{ {}, VK_FALSE, VK_FALSE, vk::PolygonMode::eFill, vk::CullModeFlagBits::eBack, vk::FrontFace::eCounterClockwise, VK_FALSE, 0.0f, 0.0f, 0.0f, 1.0f };

		vk::PipelineMultisampleStateCreateInfo multisample_state_create_info{ {}, vk::SampleCountFlagBits::e1, VK_FALSE, 1.0f, nullptr, VK_FALSE, VK_FALSE };

		vk::PipelineColorBlendAttachmentState color_blend_attachment_state{ VK_FALSE, vk::BlendFactor::eOne, vk::BlendFactor::eZero, vk::BlendOp::eAdd, vk::BlendFactor::eOne, vk::BlendFactor::eZero, vk::BlendOp::eAdd, vk::ColorComponentFlagBits::eR | vk::ColorComponentFlagBits::eG | vk::ColorComponentFlagBits::eB| vk::ColorComponentFlagBits::eA };

		vk::PipelineColorBlendStateCreateInfo color_blend_state_create_info{ {}, VK_FALSE, vk::LogicOp::eCopy, 1, &color_blend_attachment_state, {0.0f,0.0f,0.0f,0.0f} };



		std::vector<vk::DescriptorSetLayoutBinding> descriptor_set_layout_bindings{ 
			vk::DescriptorSetLayoutBinding{ 0, vk::DescriptorType::eUniformBuffer, 1, vk::ShaderStageFlagBits::eVertex, nullptr },
			//vk::DescriptorSetLayoutBinding{ 1, vk::DescriptorType::eCombinedImageSampler, 1, vk::ShaderStageFlagBits::eFragment, nullptr },
		};
		
		vk::DescriptorSetLayoutCreateInfo descriptor_set_layout_create_info{ {}, (uint32_t)descriptor_set_layout_bindings.size(), descriptor_set_layout_bindings.data() };

		// TODO(antoine) destroy
		vk::DescriptorSetLayout descriptor_set_layout = device.createDescriptorSetLayout(descriptor_set_layout_create_info);


	
		
		

		// vk::PushConstantRange push_constant_range{ vk::ShaderStageFlagBits::eVertex, 0, sizeof(glm::mat4) };

		vk::PipelineLayoutCreateInfo pipeline_layout_create_info{ {}, 1, &descriptor_set_layout, 0, nullptr };

		// TODO(antoine) destroy
		auto pipeline_layout = device.createPipelineLayout(pipeline_layout_create_info);

		vk::GraphicsPipelineCreateInfo graphics_pipeline_create_info{ {}, (uint32_t)shader_stages_ci.size(), shader_stages_ci.data(), &vertex_input_state_create_info, &input_assembly_state_create_info, nullptr, &viewport_state_create_info,&rasterization_state_create_info, &multisample_state_create_info, nullptr, &color_blend_state_create_info, nullptr, pipeline_layout, render_pass, 0, VK_NULL_HANDLE, -1 };
		
		// TODO(antoine) destroy
		auto graphics_pipeline = device.createGraphicsPipeline(VK_NULL_HANDLE, graphics_pipeline_create_info);
		
		// TODO(antoine) destroy
		auto command_pool = device.createCommandPool({ {}, renderer.graphics_family_index() });

		

		std::vector<vk::DescriptorPoolSize> descriptor_pool_size{ 
			{ vk::DescriptorType::eUniformBuffer, 1},
			//{ vk::DescriptorType::eCombinedImageSampler, 1 },
		};
		
		vk::DescriptorPoolCreateInfo descriptor_pool_create_info{ {}, 1, (uint32_t)descriptor_pool_size.size(), descriptor_pool_size.data() };

		// TODO(antoine) destroy
		auto descriptor_pool = device.createDescriptorPool(descriptor_pool_create_info);

		vk::DescriptorSetAllocateInfo descriptor_set_allocate_info{ descriptor_pool, 1, &descriptor_set_layout };

		
			
		auto descriptor_set = device.allocateDescriptorSets(descriptor_set_allocate_info)[0];
		
		auto descriptor_buffer_info = test_mesh.descriptor_buffer_info();

		vk::WriteDescriptorSet descriptor_write{ descriptor_set, 0, 0, 1, vk::DescriptorType::eUniformBuffer, nullptr, &descriptor_buffer_info, nullptr };
		
		device.updateDescriptorSets(1, &descriptor_write, 0, nullptr);


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

			cmd.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, pipeline_layout, 0, 1, &descriptor_set, 0, nullptr);

			test_mesh.bind(cmd, 0);
			test_mesh.draw(cmd);

			cmd.endRenderPass();


			std::swap(src_queue, dst_queue);
			
			vk::ImageMemoryBarrier barrier_draw_to_present{ vk::AccessFlagBits::eColorAttachmentWrite, vk::AccessFlagBits::eMemoryRead, vk::ImageLayout::ePresentSrcKHR, vk::ImageLayout::ePresentSrcKHR, src_queue, dst_queue, swapchain_images[i], img_subresource_range };
			cmd.pipelineBarrier(vk::PipelineStageFlagBits::eColorAttachmentOutput, vk::PipelineStageFlagBits::eBottomOfPipe, vk::DependencyFlags{}, 0, nullptr, 0, nullptr, 1, &barrier_draw_to_present);

			cmd.end();
		}
		
		
		



		long long timings[10] = {};
		int timing_index=0;
		long long timing_sum = 0;
		auto last_time = std::chrono::high_resolution_clock::now();
		while (!glfwWindowShouldClose(renderer.window_handle()))
		{
			renderer.render();

			renderer.present();

			auto current_time = std::chrono::high_resolution_clock::now();

			auto dt = std::chrono::duration_cast<std::chrono::microseconds>(current_time - last_time);
			
			timing_sum -= timings[timing_index % 10];
			timings[timing_index%10] = dt.count();
			timing_sum += timings[timing_index % 10];

			timing_index++;

			last_time = current_time;

			char title[256];
			snprintf(title, 256, "Frame Time %f ms (fps : %f)", timing_sum/10000.0, 10000000.0/timing_sum);
			glfwSetWindowTitle(renderer.window_handle(), title);

			glfwPollEvents();
		}
	}
	
}
