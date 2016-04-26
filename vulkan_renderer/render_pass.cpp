#include "render_pass.h"

render_pass::render_pass(const std::vector<vk::AttachmentDescription>& attachments) : _attachments(attachments)
{

}

uint32_t render_pass::push_subpass(const std::vector<vk::AttachmentReference> input_attachments, const std::vector<vk::AttachmentReference> color_attachments, const std::vector<vk::AttachmentReference> resolve_attachment, const std::vector<vk::AttachmentReference> depth_attachments, const std::vector<uint32_t> preserve_attachments)
{
	subpass_info subpass{ input_attachments, color_attachments, resolve_attachment, depth_attachments, preserve_attachments };

	subpass.description = vk::SubpassDescription{ {}, vk::PipelineBindPoint::eGraphics, (uint32_t)subpass.input_attachments.size(), subpass.input_attachments.data(), (uint32_t)color_attachments.size(), subpass.color_attachments.data(), subpass.resolve_attachment.data(), (uint32_t)subpass.depth_attachments.size(), subpass.depth_attachments.data(), (uint32_t)subpass.preserve_attachments.size(), subpass.preserve_attachments.data() };

	_subpasses.push_back(std::move(subpass));
	return _subpasses.size() - 1;
}

void render_pass::construct_render_pass()
{



}
