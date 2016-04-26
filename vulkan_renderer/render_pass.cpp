#include "render_pass.h"

#include "renderer.h"

render_pass::render_pass(const std::vector<vk::AttachmentDescription>& attachments, renderer& renderer) : _subpass_count(0), _attachments(attachments), _renderer(renderer)
{

}

uint32_t render_pass::push_subpass(const std::vector<vk::AttachmentReference> input_attachments,
	const std::vector<uint32_t> color_attachments,
	const std::vector<vk::AttachmentReference> resolve_attachment,
	uint32_t depth_attachment,
	const std::vector<uint32_t> preserve_attachments)
{
	_subpasses_data.emplace_back(input_attachments, std::vector<vk::AttachmentReference>{}, resolve_attachment, vk::AttachmentReference{ depth_attachment, vk::ImageLayout::eDepthStencilAttachmentOptimal }, preserve_attachments);
	_subpasses_data.back().color_attachments.resize(color_attachments.size());
	std::transform(color_attachments.begin(), color_attachments.end(), _subpasses_data.back().color_attachments.begin(), [](uint32_t id) { return vk::AttachmentReference{ id, vk::ImageLayout::eColorAttachmentOptimal }; });

	auto& subpass_data = _subpasses_data.back();
	_subpasses.push_back({ {}, vk::PipelineBindPoint::eGraphics,
		(uint32_t)subpass_data.input_attachments.size(), subpass_data.input_attachments.data(),
		(uint32_t)subpass_data.color_attachments.size(), subpass_data.color_attachments.data(),
		subpass_data.resolve_attachment.data(),
		&subpass_data.depth_attachment,
		(uint32_t)subpass_data.preserve_attachments.size(), subpass_data.preserve_attachments.data() });

	return _subpasses.size() - 1;
}

void render_pass::finalize_render_pass()
{
	_render_pass = _renderer.device().createRenderPass(vk::RenderPassCreateInfo{ {}, (uint32_t)_attachments.size(), _attachments.data(), (uint32_t)_subpasses.size(), _subpasses.data(), (uint32_t)_dependencies.size(), _dependencies.data() });
}
