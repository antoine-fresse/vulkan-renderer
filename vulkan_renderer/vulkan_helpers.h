#pragma once

#include "vulkan_include.h"

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