#include "renderer.h"
#include "camera.h"
#include "model.h"
#include "texture.h"
#include "pipeline.h"
#include "render_pass.h"

#include "shared.h"

#include <chrono>

static input_state g_input_state = {};

void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods)
{
	switch(key)
	{
	case GLFW_KEY_UP:
		if (action == GLFW_PRESS)
			g_input_state.up = true;
		else if(action == GLFW_RELEASE)
			g_input_state.up = false;
		break;
	case GLFW_KEY_DOWN:
		if (action == GLFW_PRESS)
			g_input_state.down = true;
		else if (action == GLFW_RELEASE)
			g_input_state.down = false;
		break;
	case GLFW_KEY_LEFT:
		if (action == GLFW_PRESS)
			g_input_state.left = true;
		else if (action == GLFW_RELEASE)
			g_input_state.left = false;
		break;
	case GLFW_KEY_RIGHT:
		if (action == GLFW_PRESS)
			g_input_state.right = true;
		else if (action == GLFW_RELEASE)
			g_input_state.right = false;
		break;
	default:
		break;
	}
}

int main()
{
	

	std::vector<const char*> instance_layers;
	std::vector<const char*> device_layers;
	std::vector<const char*> instance_extensions;
	std::vector<const char*> device_extensions;

#if 0
	instance_layers.push_back("VK_LAYER_RENDERDOC_Capture");
	device_layers.push_back("VK_LAYER_RENDERDOC_Capture");
#endif

	//try
	{
		renderer renderer{ 800, 600, 3, instance_layers , instance_extensions, device_layers, device_extensions };

		
		camera cam(renderer);
		
		glfwSetKeyCallback(renderer.window_handle(), key_callback);


		vk::Device device = renderer.device();
		auto swapchain_images = renderer.swapchain_images();


		vk::ImageSubresourceRange img_subresource_range{ vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1 };

		vk::SampleCountFlagBits multisampling_count = vk::SampleCountFlagBits::e4;
		std::vector<vk::AttachmentDescription> attachment_descriptions{ 
			vk::AttachmentDescription{ {}, renderer.format(), multisampling_count, vk::AttachmentLoadOp::eClear, vk::AttachmentStoreOp::eDontCare, vk::AttachmentLoadOp::eDontCare, vk::AttachmentStoreOp::eDontCare, vk::ImageLayout::eColorAttachmentOptimal, vk::ImageLayout::eColorAttachmentOptimal },
			vk::AttachmentDescription{ {}, renderer.depth_format(), multisampling_count, vk::AttachmentLoadOp::eClear, vk::AttachmentStoreOp::eDontCare, vk::AttachmentLoadOp::eDontCare, vk::AttachmentStoreOp::eDontCare, vk::ImageLayout::eDepthStencilAttachmentOptimal, vk::ImageLayout::eDepthStencilAttachmentOptimal },

			vk::AttachmentDescription{ {}, renderer.format(), vk::SampleCountFlagBits::e1, vk::AttachmentLoadOp::eDontCare, vk::AttachmentStoreOp::eStore, vk::AttachmentLoadOp::eDontCare, vk::AttachmentStoreOp::eDontCare, vk::ImageLayout::eColorAttachmentOptimal, vk::ImageLayout::eColorAttachmentOptimal },
			// vk::AttachmentDescription{ {}, renderer.depth_format(), vk::SampleCountFlagBits::e1, vk::AttachmentLoadOp::eDontCare, vk::AttachmentStoreOp::eStore, vk::AttachmentLoadOp::eDontCare, vk::AttachmentStoreOp::eDontCare, vk::ImageLayout::eDepthStencilAttachmentOptimal, vk::ImageLayout::eDepthStencilAttachmentOptimal },
		};
	
		render_pass render_pass{ attachment_descriptions , renderer};
		render_pass.push_subpass({}, { 0 }, { {2, vk::ImageLayout::eColorAttachmentOptimal} }, 1, {});
		render_pass.finalize_render_pass();

		pipeline::description pipeline_desc;
		pipeline_desc.vertex_input_attributes = model::attribute_descriptions(0);
		pipeline_desc.vertex_input_bindings = model::binding_description(0);
		pipeline_desc.viewport = vk::Viewport{ 0.0f, 0.0f, 800.0f, 600.0f, 0.0f, 1.0f };
		pipeline_desc.scissor = vk::Rect2D{ { 0,0 },{ 800, 600 } };
		pipeline_desc.descriptor_set_layouts_description = {
			{ vk::DescriptorSetLayoutBinding{ 0, vk::DescriptorType::eUniformBuffer, 1, vk::ShaderStageFlagBits::eVertex, nullptr } },
			{ vk::DescriptorSetLayoutBinding{ 0, vk::DescriptorType::eUniformBuffer, 1, vk::ShaderStageFlagBits::eVertex, nullptr } },
			{ vk::DescriptorSetLayoutBinding{ 0, vk::DescriptorType::eCombinedImageSampler, 1, vk::ShaderStageFlagBits::eFragment, nullptr } },
		};
		pipeline_desc.descriptor_sets_pool_sizes = { 1, 64, 128 };
		pipeline_desc.samples = multisampling_count;


		pipeline forward_rendering_pipeline{ renderer, render_pass, pipeline_desc };
		//forward_rendering_pipeline.create_depth_buffer(texture::description{ renderer.depth_format(),{ 800,600 }, vk::ImageLayout::eDepthStencilAttachmentOptimal, vk::ImageUsageFlagBits::eDepthStencilAttachment | vk::ImageUsageFlagBits::eTransferSrc, vk::ImageAspectFlagBits::eDepth | vk::ImageAspectFlagBits::eStencil, vk::MemoryPropertyFlagBits::eDeviceLocal, vk::AccessFlagBits::eDepthStencilAttachmentWrite });
		//texture* depth_buffer = forward_rendering_pipeline.depth_buffer();

		std::vector<vk::ImageView> views(swapchain_images.size());
		std::vector<vk::Framebuffer> framebuffers(swapchain_images.size());

		std::vector<std::unique_ptr<texture>> multisampled_color_buffers;
		std::vector<std::unique_ptr<texture>> multisampled_depth_buffers;
		for (uint32_t i = 0; i < swapchain_images.size(); ++i)
		{
			vk::ImageViewCreateInfo	view_create_info{ {}, swapchain_images[i], vk::ImageViewType::e2D, renderer.format(), vk::ComponentMapping{}, img_subresource_range };

			views[i] = device.createImageView(view_create_info);
			
			multisampled_color_buffers.push_back(std::make_unique<texture>(texture::description{ renderer.format(),{ 800,600 }, vk::ImageLayout::eColorAttachmentOptimal, vk::ImageUsageFlagBits::eColorAttachment | vk::ImageUsageFlagBits::eTransientAttachment, vk::ImageAspectFlagBits::eColor, vk::MemoryPropertyFlagBits::eDeviceLocal, vk::AccessFlagBits::eColorAttachmentWrite, multisampling_count }, renderer));
			multisampled_depth_buffers.push_back(std::make_unique<texture>(texture::description{ renderer.depth_format(),{ 800,600 }, vk::ImageLayout::eDepthStencilAttachmentOptimal, vk::ImageUsageFlagBits::eDepthStencilAttachment | vk::ImageUsageFlagBits::eTransientAttachment, vk::ImageAspectFlagBits::eDepth | vk::ImageAspectFlagBits::eStencil, vk::MemoryPropertyFlagBits::eDeviceLocal, vk::AccessFlagBits::eDepthStencilAttachmentWrite, multisampling_count }, renderer));

			std::vector<vk::ImageView> fb_views { multisampled_color_buffers[i]->image_view(), multisampled_depth_buffers[i]->image_view(), views[i] };
			vk::FramebufferCreateInfo framebuffer_create_info{ {}, render_pass, (uint32_t)fb_views.size(), fb_views.data(), 800, 600, 1 };

			framebuffers[i] = device.createFramebuffer(framebuffer_create_info);
		}

		model nanosuit{ "data/nanosuit.obj", renderer, 3.0f };
		model sponza{ "data/sponza.obj", renderer, 1.0f };
		cam.attach(forward_rendering_pipeline, 0);
		
		nanosuit.attach_textures(forward_rendering_pipeline, 2);
		sponza.attach_textures(forward_rendering_pipeline, 2);

		auto nanosuit_descriptor = forward_rendering_pipeline.allocate(1);
		auto sponza_descriptor = forward_rendering_pipeline.allocate(1);

		auto descriptor_buffer_info_nanosuit = nanosuit.descriptor_buffer_info();
		auto descriptor_buffer_info_sponza = sponza.descriptor_buffer_info();

		std::vector<vk::WriteDescriptorSet> descriptor_writes{ 
			vk::WriteDescriptorSet{ *nanosuit_descriptor, 0, 0, 1, vk::DescriptorType::eUniformBuffer, nullptr, &descriptor_buffer_info_nanosuit, nullptr },
			vk::WriteDescriptorSet{ *sponza_descriptor, 0, 0, 1, vk::DescriptorType::eUniformBuffer, nullptr, &descriptor_buffer_info_sponza, nullptr },
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

			vk::ImageMemoryBarrier barrier_present_to_draw{ vk::AccessFlagBits::eMemoryRead, vk::AccessFlagBits::eColorAttachmentWrite, vk::ImageLayout::eUndefined, render_pass.attachment(0).initialLayout(), src_queue, dst_queue, swapchain_images[i], img_subresource_range };
			cmd.pipelineBarrier(vk::PipelineStageFlagBits::eColorAttachmentOutput, vk::PipelineStageFlagBits::eColorAttachmentOutput, vk::DependencyFlags{}, 0, nullptr, 0, nullptr, 1, &barrier_present_to_draw);

			
			vk::ClearValue clear_value[]{ vk::ClearColorValue{ std::array<float, 4>{1.0f, 0.0f, 1.0f, 0.0f}}, vk::ClearDepthStencilValue{ 1.0f, 0 } };
			vk::RenderPassBeginInfo render_pass_bi{ render_pass, framebuffers[i], vk::Rect2D{ { 0,0 },{ 800, 600 } }, 2, clear_value };

			cmd.beginRenderPass(render_pass_bi, vk::SubpassContents::eInline);

			cmd.bindPipeline(vk::PipelineBindPoint::eGraphics, forward_rendering_pipeline);

			std::vector<vk::DescriptorSet> tobind { cam.descriptor_set(), *nanosuit_descriptor };
			cmd.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, forward_rendering_pipeline.pipeline_layout(), 0, (uint32_t)tobind.size(), tobind.data(), 0, nullptr);
			nanosuit.draw(cmd, forward_rendering_pipeline, cam);


			tobind = { *sponza_descriptor };
			cmd.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, forward_rendering_pipeline.pipeline_layout(), 1, (uint32_t)tobind.size(), tobind.data(), 0, nullptr);
			sponza.draw(cmd, forward_rendering_pipeline, cam);


			cmd.endRenderPass();


			std::swap(src_queue, dst_queue);
			
			vk::ImageMemoryBarrier barrier_draw_to_present{ vk::AccessFlagBits::eColorAttachmentWrite, vk::AccessFlagBits::eMemoryRead, render_pass.attachment(0).finalLayout(), vk::ImageLayout::ePresentSrcKHR, src_queue, dst_queue, swapchain_images[i], img_subresource_range };
			cmd.pipelineBarrier(vk::PipelineStageFlagBits::eColorAttachmentOutput, vk::PipelineStageFlagBits::eBottomOfPipe, vk::DependencyFlags{}, 0, nullptr, 0, nullptr, 1, &barrier_draw_to_present);

			cmd.end();
		}
		
		
		auto last_time = std::chrono::steady_clock::now();
		while (!glfwWindowShouldClose(renderer.window_handle()))
		{
			auto current_time = std::chrono::steady_clock::now();

			std::chrono::duration<double> dt = current_time - last_time;
			last_time = current_time;
			cam.update(dt.count(), g_input_state);
			nanosuit.update(dt.count());

			renderer.render();
			renderer.present();

			char title[256];
			snprintf(title, 256, "frame time : %f ms", dt.count()*1000.0);
			glfwSetWindowTitle(renderer.window_handle(), title);
			glfwPollEvents();
		}
	}
	
}
