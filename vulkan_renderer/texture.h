#pragma once

#include "vulkan_include.h"

class renderer;

class texture
{
public:

	struct description
	{
		vk::Format format;
		vk::Extent2D size;
		vk::ImageLayout layout;
		vk::ImageUsageFlags usage;
		vk::ImageAspectFlags view_mask;
		vk::MemoryPropertyFlags memory_flags;
		
	};

	texture(const std::string& filepath, renderer& renderer);
	texture(const description& desc, renderer& renderer);


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