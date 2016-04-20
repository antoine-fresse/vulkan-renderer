#include "vulkan_renderer.h"

int main()
{
	

	std::vector<const char*> instance_layers;
	std::vector<const char*> device_layers;
	std::vector<const char*> instance_extensions;
	std::vector<const char*> device_extensions;

	

	vulkan_renderer renderer{ instance_layers , instance_extensions, device_layers, device_extensions };
	auto instance = renderer.instance();
	auto device = renderer.device();
	auto& window = renderer.window();
	
	renderer.open_window(800, 600);

	auto cmd_buffers = window.present_queue_cmd_buffers();
	auto swapchain_images = window.swapchain_images();
	vk::CommandBufferBeginInfo cmd_bi{ vk::CommandBufferUsageFlagBits::eSimultaneousUse, nullptr };

	vk::ClearColorValue clear_color[] = {	{ std::array<float, 4>{1.0f, 0.8f, 0.4f, 0.0f} },
											{ std::array<float, 4>{0.8f, 0.4f, 1.0f, 0.0f} },
											{ std::array<float, 4>{0.4f, 1.0f, 0.8f, 0.0f} }};

	vk::ImageSubresourceRange img_subresource_range{ vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1 };

	for (uint32_t i = 0; i < cmd_buffers.size(); ++i)
	{
		vk::ImageMemoryBarrier barrier_present_to_clear{ vk::AccessFlagBits::eMemoryRead, vk::AccessFlagBits::eTransferWrite, vk::ImageLayout::eUndefined, vk::ImageLayout::eTransferDstOptimal, renderer.graphics_family_index(), renderer.graphics_family_index(), swapchain_images[i], img_subresource_range };
		vk::ImageMemoryBarrier barrier_clear_to_present{ vk::AccessFlagBits::eTransferWrite, vk::AccessFlagBits::eMemoryRead, vk::ImageLayout::eTransferDstOptimal, vk::ImageLayout::ePresentSrcKHR, renderer.graphics_family_index(), renderer.graphics_family_index(), swapchain_images[i], img_subresource_range };


		cmd_buffers[i].begin(cmd_bi);

		cmd_buffers[i].pipelineBarrier(vk::PipelineStageFlagBits::eTransfer, vk::PipelineStageFlagBits::eTransfer, vk::DependencyFlags{}, 0, nullptr, 0, nullptr, 1, &barrier_present_to_clear);

		cmd_buffers[i].clearColorImage(swapchain_images[i], vk::ImageLayout::eTransferDstOptimal, clear_color[i % 3], img_subresource_range);

		cmd_buffers[i].pipelineBarrier(vk::PipelineStageFlagBits::eTransfer, vk::PipelineStageFlagBits::eBottomOfPipe, vk::DependencyFlags{}, 0, nullptr, 0, nullptr, 1, &barrier_clear_to_present);
		
		cmd_buffers[i].end();
	}
	
	vk::AttachmentDescription attachment_descriptions[]{ { {}, window.format(), vk::SampleCountFlagBits::e1, vk::AttachmentLoadOp::eClear, vk::AttachmentStoreOp::eStore, vk::AttachmentLoadOp::eDontCare, vk::AttachmentStoreOp::eDontCare, vk::ImageLayout::ePresentSrcKHR, vk::ImageLayout::ePresentSrcKHR } };
	vk::AttachmentReference color_attachment_references[]{ { 0, vk::ImageLayout::eColorAttachmentOptimal } };
	vk::SubpassDescription subpass_descriptions[]{ { {}, vk::PipelineBindPoint::eGraphics, 0, nullptr, 1, color_attachment_references,nullptr,nullptr,0,nullptr } };

	vk::RenderPassCreateInfo render_pass_create_info;

	auto render_pass = device.createRenderPass({ {}, 1, attachment_descriptions, 1, subpass_descriptions, 0, nullptr });

	std::vector<vk::ImageView> views(cmd_buffers.size());
	std::vector<vk::Framebuffer> framebuffers(cmd_buffers.size());
	for (uint32_t i = 0; i < cmd_buffers.size(); ++i)
	{
		vk::ImageViewCreateInfo	view_create_info{ {}, swapchain_images[i], vk::ImageViewType::e2D, window.format(), vk::ComponentMapping{}, img_subresource_range };

		views[i] = device.createImageView(view_create_info);


		vk::FramebufferCreateInfo framebuffer_create_info{ {}, render_pass, 1, &views[i], 300, 300, 1 };

		framebuffers[i] = device.createFramebuffer(framebuffer_create_info);
	}
	


	while(true)
	{
		uint32_t image_index = 0;
		auto result = device.acquireNextImageKHR(window.swapchain(), UINT64_MAX, window.image_available_semaphore(), VK_NULL_HANDLE, &image_index);
		if (result == vk::Result::eSuboptimalKHR || result == vk::Result::eErrorOutOfDateKHR)
			window.window_size_changed();

		vk::PipelineStageFlags pipeline_stage_flags{ vk::PipelineStageFlagBits::eTransfer };
		vk::SubmitInfo submit_info{ 1, &window.image_available_semaphore(), &pipeline_stage_flags, 1, &cmd_buffers[image_index], 1, &renderer.rendering_finished_semaphore() };

		renderer.queue().submit(submit_info, VK_NULL_HANDLE);

		renderer.queue().presentKHR({ 1, &renderer.rendering_finished_semaphore(), 1, &window.swapchain(), &image_index, nullptr });

	}
}
