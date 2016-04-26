#pragma once

#include "vulkan_include.h"

class render_pass
{
public:
	render_pass(const std::vector<vk::AttachmentDescription>& attachments);
	
	uint32_t push_subpass(const std::vector<vk::AttachmentReference> input_attachments, const std::vector<vk::AttachmentReference> color_attachments, const std::vector<vk::AttachmentReference> depth_attachments, const std::vector<uint32_t> preserve_attachments);
	
	void add_dependency(const vk::SubpassDependency& dep) { _dependencies.push_back(dep); }
	
	void construct_render_pass();

	operator vk::RenderPass() const{ return _render_pass; }
private:
	

	struct subpass_info
	{
		std::vector<vk::AttachmentReference> input_attachments;
		std::vector<vk::AttachmentReference> color_attachments;
		std::vector<vk::AttachmentReference> resolve_attachment;
		std::vector<vk::AttachmentReference> depth_attachments;
		std::vector<uint32_t> preserve_attachments;
		vk::SubpassDescription description;
	};

	vk::RenderPass _render_pass;
	uint32_t _subpass_count;
	std::vector<vk::AttachmentDescription> _attachments;
	std::vector<subpass_info> _subpasses;
	std::vector<vk::SubpassDependency> _dependencies;

};