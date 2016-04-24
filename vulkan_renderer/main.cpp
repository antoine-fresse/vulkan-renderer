#include "renderer.h"
#include "camera.h"
#include "model.h"
#include "texture.h"
#include "pipeline.h"

#include <chrono>
#include "shared.h"

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

		camera cam(renderer);

		vk::Device device = renderer.device();
		auto swapchain_images = renderer.swapchain_images();

		vk::ClearColorValue clear_color[] = { { std::array<float, 4>{1.0f, 0.0f, 1.0f, 0.0f} },
												{ std::array<float, 4>{1.0f, 0.0f, 1.0f, 0.0f} },
												{ std::array<float, 4>{1.0f, 0.0f, 1.0f, 0.0f} } };

		vk::ImageSubresourceRange img_subresource_range{ vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1 };


		vk::AttachmentDescription attachment_descriptions[]{ vk::AttachmentDescription{ {}, renderer.format(), vk::SampleCountFlagBits::e1, vk::AttachmentLoadOp::eClear, vk::AttachmentStoreOp::eStore, vk::AttachmentLoadOp::eDontCare, vk::AttachmentStoreOp::eDontCare, vk::ImageLayout::ePresentSrcKHR, vk::ImageLayout::ePresentSrcKHR } };
		vk::AttachmentReference color_attachment_references[]{ vk::AttachmentReference{ 0, vk::ImageLayout::eColorAttachmentOptimal } };
		vk::SubpassDescription subpass_descriptions[]{ vk::SubpassDescription{ {}, vk::PipelineBindPoint::eGraphics, 0, nullptr, 1, color_attachment_references, nullptr,nullptr,0,nullptr } };

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

		pipeline::description pipeline_desc;
		pipeline_desc.vertex_input_attributes = model::attribute_descriptions(0);
		pipeline_desc.vertex_input_bindings = model::binding_description(0);
		pipeline_desc.viewport = vk::Viewport{ 0.0f, 0.0f, 800, 600.0f, 0.0f, 1.0f };
		pipeline_desc.scissor = vk::Rect2D{ { 0,0 },{ 800, 600 } };
		pipeline_desc.descriptor_set_layouts_description = {
			{ vk::DescriptorSetLayoutBinding{ 0, vk::DescriptorType::eUniformBuffer, 1, vk::ShaderStageFlagBits::eVertex, nullptr } },
			{ vk::DescriptorSetLayoutBinding{ 0, vk::DescriptorType::eUniformBuffer, 1, vk::ShaderStageFlagBits::eVertex, nullptr } },
			{ vk::DescriptorSetLayoutBinding{ 0, vk::DescriptorType::eCombinedImageSampler, 1, vk::ShaderStageFlagBits::eFragment, nullptr } },
		};
		pipeline_desc.descriptor_sets_pool_sizes = { 1, 64, 128 };
		
		pipeline forward_rendering_pipeline{ renderer, render_pass, pipeline_desc };
		
		// TODO(antoine) destroy
		auto command_pool = device.createCommandPool({ {}, renderer.graphics_family_index() });

		cam.attach(forward_rendering_pipeline);
		test_mesh.attach_textures(forward_rendering_pipeline, 2);

		auto object_descriptor = forward_rendering_pipeline.allocate(1);

		auto descriptor_buffer_info_object = test_mesh.descriptor_buffer_info();

		std::vector<vk::WriteDescriptorSet> descriptor_writes{ 
			vk::WriteDescriptorSet{ *object_descriptor, 0, 0, 1, vk::DescriptorType::eUniformBuffer, nullptr, &descriptor_buffer_info_object, nullptr },
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

			cmd.bindPipeline(vk::PipelineBindPoint::eGraphics, forward_rendering_pipeline);

			vk::DescriptorSet tobind[] = { cam.descriptor_set(), *object_descriptor };
			cmd.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, forward_rendering_pipeline.pipeline_layout(), 0, array_sizeof(tobind), tobind, 0, nullptr);

			test_mesh.draw(cmd, forward_rendering_pipeline);

			cmd.endRenderPass();


			std::swap(src_queue, dst_queue);
			
			vk::ImageMemoryBarrier barrier_draw_to_present{ vk::AccessFlagBits::eColorAttachmentWrite, vk::AccessFlagBits::eMemoryRead, vk::ImageLayout::ePresentSrcKHR, vk::ImageLayout::ePresentSrcKHR, src_queue, dst_queue, swapchain_images[i], img_subresource_range };
			cmd.pipelineBarrier(vk::PipelineStageFlagBits::eColorAttachmentOutput, vk::PipelineStageFlagBits::eBottomOfPipe, vk::DependencyFlags{}, 0, nullptr, 0, nullptr, 1, &barrier_draw_to_present);

			cmd.end();
		}
		
		
		auto last_time = std::chrono::steady_clock::now();
		while (!glfwWindowShouldClose(renderer.window_handle()))
		{
			auto current_time = std::chrono::steady_clock::now();

			std::chrono::duration<double> dt = current_time - last_time;
			last_time = current_time;
			test_mesh.update(dt.count());

			renderer.render();
			renderer.present();
			glfwPollEvents();
		}
	}
	
}
