#pragma once

#include "vulkan_include.h"

class renderer;

class render_pass
{
public:
	render_pass(const std::vector<vk::AttachmentDescription>& attachments, renderer& _renderer);
	
	uint32_t push_subpass(const std::vector<vk::AttachmentReference> input_attachments,
		const std::vector<uint32_t> color_attachments,
		const std::vector<vk::AttachmentReference> resolve_attachment,
		uint32_t depth_attachment,
		const std::vector<uint32_t> preserve_attachments);
	
	void add_dependency(const vk::SubpassDependency& dep) { _dependencies.push_back(dep); }
	
	void finalize_render_pass();

	vk::AttachmentDescription attachment(uint32_t id) { return _attachments[id]; }


	operator vk::RenderPass() const{ return _render_pass; }
private:
	

	struct subpass_info
	{
		std::vector<vk::AttachmentReference> input_attachments;

		subpass_info(const std::vector<vk::AttachmentReference>& input_attachments, const std::vector<vk::AttachmentReference>& color_attachments, const std::vector<vk::AttachmentReference>& resolve_attachment, const vk::AttachmentReference& depth_attachment, const std::vector<uint32_t>& preserve_attachments)
			: input_attachments(input_attachments),
			  color_attachments(color_attachments),
			  resolve_attachment(resolve_attachment),
			  depth_attachment(depth_attachment),
			  preserve_attachments(preserve_attachments)
		{
		}

		std::vector<vk::AttachmentReference> color_attachments;
		std::vector<vk::AttachmentReference> resolve_attachment;
		vk::AttachmentReference depth_attachment;
		std::vector<uint32_t> preserve_attachments;
	};


	vk::RenderPass _render_pass;
	uint32_t _subpass_count;
	std::vector<vk::AttachmentDescription> _attachments;
	std::vector<subpass_info> _subpasses_data;
	std::vector<vk::SubpassDescription> _subpasses;
	std::vector<vk::SubpassDependency> _dependencies;

	renderer& _renderer;
};
