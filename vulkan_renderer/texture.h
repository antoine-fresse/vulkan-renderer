#pragma once

#include "vulkan_include.h"

class renderer;

class texture
{
public:

	struct description
	{
		description(vk::Format format, const vk::Extent2D& size, vk::ImageLayout layout, const vk::ImageUsageFlags& usage, const vk::ImageAspectFlags& view_mask, const vk::AccessFlags& access_flags, vk::SampleCountFlagBits samples= vk::SampleCountFlagBits::e1)
			: format(format),
			size(size),
			layout(layout),
			usage(usage),
			view_mask(view_mask),
			access_flags(access_flags),
			samples(samples)
		{
		}
		vk::Format format;
		vk::Extent2D size;
		vk::ImageLayout layout;
		vk::ImageUsageFlags usage;
		vk::ImageAspectFlags view_mask;
		vk::AccessFlags access_flags;
		vk::SampleCountFlagBits samples;
	};

	texture(const void* data, uint32_t size, renderer& renderer);
	texture(const void* data, uint32_t width, uint32_t height, renderer& renderer);
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

	void init_image(const description& desc);
	
	renderer& _renderer;
	vk::Image _image;
	vk::ImageLayout _image_layout = vk::ImageLayout::eUndefined;
	vk::ImageView _image_view;
	vk::DeviceMemory _memory;
	vk::Sampler _sampler;
	vk::MemoryRequirements _image_mem_reqs;

	uint32_t _width, _height;
	uint32_t _mip_levels;
	
};