#pragma once

#include "vulkan_include.h"

class renderer;

class texture
{
public:

	struct description
	{
		description(vk::Format format, const vk::Extent2D& size, vk::ImageLayout layout, const vk::ImageUsageFlags& usage, const vk::ImageAspectFlags& view_mask, vk::MemoryPropertyFlagBits memory_flags, const vk::AccessFlags& access_flags)
			: format(format),
			size(size),
			layout(layout),
			usage(usage),
			view_mask(view_mask),
			memory_flags(memory_flags),
			access_flags(access_flags)
		{
		}
		vk::Format format;
		vk::Extent2D size;
		vk::ImageLayout layout;
		vk::ImageUsageFlags usage;
		vk::ImageAspectFlags view_mask;
		vk::MemoryPropertyFlagBits memory_flags;
		vk::AccessFlags access_flags;

		
	};

	texture(const std::string& filepath, renderer& renderer);
	texture(const description& desc, renderer& renderer);

	vk::ImageView image_view() const { return _image_view; };
	vk::Image image() const { return _image; };
	vk::Sampler sampler() const { return _sampler; };
	vk::ImageLayout image_layout() const { return _image_layout; };

	uint32_t width() const { return _width; }
	uint32_t height() const { return _height; }

	~texture();

	vk::DescriptorImageInfo descriptor_image_info() const;


private:
	
	renderer& _renderer;
	vk::Image _image;
	vk::ImageLayout _image_layout;
	vk::ImageView _image_view;
	vk::DeviceMemory _memory;
	vk::Sampler _sampler;
	vk::MemoryRequirements _image_mem_reqs;

	uint32_t _width, _height;
	uint32_t _mip_levels;
	
};