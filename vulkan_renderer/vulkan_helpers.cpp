#include "vulkan_helpers.h"

#include "vulkan_include.h"

void set_image_layout(	vk::CommandBuffer cmdbuffer,
						vk::Image image,
						vk::ImageLayout oldImageLayout,
						vk::ImageLayout newImageLayout,
						vk::ImageSubresourceRange subresourceRange)
{
	// Create an image barrier object
	vk::ImageMemoryBarrier imageMemoryBarrier{{},{}, oldImageLayout, newImageLayout, VK_QUEUE_FAMILY_IGNORED, VK_QUEUE_FAMILY_IGNORED , image, subresourceRange};

	// Source layouts (old)

	// Undefined layout
	// Only allowed as initial layout!
	// Make sure any writes to the image have been finished
	if (oldImageLayout == vk::ImageLayout::ePreinitialized)
	{
		imageMemoryBarrier.srcAccessMask(vk::AccessFlagBits::eHostWrite | vk::AccessFlagBits::eTransferWrite);
	}

	// Old layout is color attachment
	// Make sure any writes to the color buffer have been finished
	if (oldImageLayout == vk::ImageLayout::eColorAttachmentOptimal)
	{
		imageMemoryBarrier.srcAccessMask(vk::AccessFlagBits::eColorAttachmentWrite);
	}

	// Old layout is depth/stencil attachment
	// Make sure any writes to the depth/stencil buffer have been finished
	if (oldImageLayout == vk::ImageLayout::eDepthStencilAttachmentOptimal)
	{
		imageMemoryBarrier.srcAccessMask(vk::AccessFlagBits::eDepthStencilAttachmentWrite);
	}

	// Old layout is transfer source
	// Make sure any reads from the image have been finished
	if (oldImageLayout == vk::ImageLayout::eTransferSrcOptimal)
	{
		imageMemoryBarrier.srcAccessMask(vk::AccessFlagBits::eTransferRead);
	}

	// Old layout is shader read (sampler, input attachment)
	// Make sure any shader reads from the image have been finished
	if (oldImageLayout == vk::ImageLayout::eShaderReadOnlyOptimal)
	{
		imageMemoryBarrier.srcAccessMask(vk::AccessFlagBits::eShaderRead);
	}

	// Target layouts (new)

	// New layout is transfer destination (copy, blit)
	// Make sure any copyies to the image have been finished
	if (newImageLayout == vk::ImageLayout::eTransferDstOptimal)
	{
		imageMemoryBarrier.dstAccessMask(vk::AccessFlagBits::eTransferWrite);
	}

	// New layout is transfer source (copy, blit)
	// Make sure any reads from and writes to the image have been finished
	if (newImageLayout == vk::ImageLayout::eTransferSrcOptimal)
	{
		imageMemoryBarrier.srcAccessMask() |= vk::AccessFlagBits::eTransferRead;
		imageMemoryBarrier.dstAccessMask(vk::AccessFlagBits::eTransferRead);
	}

	// New layout is color attachment
	// Make sure any writes to the color buffer hav been finished
	if (newImageLayout == vk::ImageLayout::eColorAttachmentOptimal)
	{
		imageMemoryBarrier.dstAccessMask(vk::AccessFlagBits::eColorAttachmentWrite);
		imageMemoryBarrier.srcAccessMask() |= vk::AccessFlagBits::eTransferRead;
	}

	// New layout is depth attachment
	// Make sure any writes to depth/stencil buffer have been finished
	if (newImageLayout == vk::ImageLayout::eDepthStencilAttachmentOptimal)
	{
		imageMemoryBarrier.dstAccessMask() |= vk::AccessFlagBits::eDepthStencilAttachmentWrite;
	}

	// New layout is shader read (sampler, input attachment)
	// Make sure any writes to the image have been finished
	if (newImageLayout == vk::ImageLayout::eShaderReadOnlyOptimal)
	{
		imageMemoryBarrier.srcAccessMask(vk::AccessFlagBits::eHostWrite | vk::AccessFlagBits::eTransferWrite);
		imageMemoryBarrier.dstAccessMask(vk::AccessFlagBits::eShaderRead);
	}

	cmdbuffer.pipelineBarrier(vk::PipelineStageFlagBits::eTopOfPipe, vk::PipelineStageFlagBits::eTopOfPipe, {}, {}, {}, { imageMemoryBarrier });
}
