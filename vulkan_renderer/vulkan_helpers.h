#pragma once

#include "vulkan_include.h"

class renderer;

void set_image_layout(	vk::CommandBuffer cmdbuffer,
						vk::Image image,
						vk::ImageLayout oldImageLayout,
						vk::ImageLayout newImageLayout,
						vk::ImageSubresourceRange subresourceRange);

inline void set_image_layout(vk::CommandBuffer cmdbuffer,
	vk::Image image,
	vk::ImageAspectFlags aspectMask,
	vk::ImageLayout oldImageLayout,
	vk::ImageLayout newImageLayout)
{
	set_image_layout(cmdbuffer, image, oldImageLayout, newImageLayout, vk::ImageSubresourceRange{ aspectMask, 0, 1, 0, 1 });
}


class staging_buffer
{
public:
	staging_buffer(renderer& renderer, size_t size);
	~staging_buffer();

	void* data() const
	{
		return _mapped_memory;
	}
	operator vk::Buffer() const { return _buffer; }

private:
	renderer& _renderer;
	vk::DeviceMemory _memory = VK_NULL_HANDLE;
	vk::Buffer _buffer = VK_NULL_HANDLE;
	vk::DeviceSize _size;
	void * _mapped_memory;
};