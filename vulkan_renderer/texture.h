#pragma once

#include "vulkan_include.h"

class renderer;

class texture
{
public:
	texture(const std::string& filepath, renderer& renderer);
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

	uint32_t _width, _height, _components;
	uint32_t _mip_levels;
	
};