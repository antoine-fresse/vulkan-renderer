#include "texture.h"

#include "renderer.h"

#include "vulkan_helpers.h"

#define STB_IMAGE_IMPLEMENTATION
#define STBI_ASSERT(x)
#include "stb/stb_image.h"

texture::texture(const std::string& filepath, renderer& renderer) : _renderer(renderer), _image_layout(vk::ImageLayout::eShaderReadOnlyOptimal)
{
	int x, y, comp;
	unsigned char *data = stbi_load(("data/" + filepath).c_str(), &x, &y, &comp, 4);

	if (!data)
	{
		data = stbi_load("data/missing_texture.png", &x, &y, &comp, 4);
	}

	_width = x;
	_height = y;
	_mip_levels = 1;

	description desc{ vk::Format::eR8G8B8A8Unorm , {_width, _height}, vk::ImageLayout::eShaderReadOnlyOptimal, vk::ImageUsageFlagBits::eSampled | vk::ImageUsageFlagBits::eTransferDst, vk::ImageAspectFlagBits::eColor, vk::AccessFlagBits::eShaderRead };
	init_image(desc);

	auto device = _renderer.device();
	vk::Buffer staging_buffer = device.createBuffer(vk::BufferCreateInfo{ {}, _width*_height * 4, vk::BufferUsageFlagBits::eTransferSrc, vk::SharingMode::eExclusive,0,nullptr });
	auto mem_reqs = device.getBufferMemoryRequirements(staging_buffer);
	vk::DeviceMemory staging_memory = device.allocateMemory(vk::MemoryAllocateInfo{ mem_reqs.size(), _renderer.find_adequate_memory(mem_reqs, vk::MemoryPropertyFlagBits::eHostVisible) });
	device.bindBufferMemory(staging_buffer, staging_memory, 0);
	
	void * mapped_memory = device.mapMemory(staging_memory, 0, mem_reqs.size(), {});
	memcpy(mapped_memory, data, _width*_height * 4);
	device.unmapMemory(staging_memory);

	vk::BufferImageCopy copy{ 0, 0, 0, vk::ImageSubresourceLayers{vk::ImageAspectFlagBits::eColor, 0, 0, 1}, {}, {_width, _height, 1 } };

	auto cmd = _renderer.setup_cmd_buffer();
	set_image_layout(cmd, _image, vk::ImageAspectFlagBits::eColor, vk::ImageLayout::ePreinitialized, vk::ImageLayout::eTransferDstOptimal);
	cmd.copyBufferToImage(staging_buffer, _image, vk::ImageLayout::eTransferDstOptimal, { copy });
	set_image_layout(cmd, _image, vk::ImageAspectFlagBits::eColor, vk::ImageLayout::eTransferDstOptimal, _image_layout);
	_renderer.flush_setup();

	device.destroyBuffer(staging_buffer);
	device.freeMemory(staging_memory);
}


texture::texture(const description& desc, renderer& renderer) : _renderer(renderer)
{
	init_image(desc);
	auto cmd = _renderer.setup_cmd_buffer();
	set_image_layout(cmd, _image, desc.view_mask, vk::ImageLayout::ePreinitialized, _image_layout);
}
texture::~texture()
{
	vk::Device device = _renderer.device();

	device.destroyImageView(_image_view);
	//device.destroySampler(_sampler);
	device.destroyImage(_image);
	device.freeMemory(_memory);
}

vk::DescriptorImageInfo texture::descriptor_image_info() const
{
	return vk::DescriptorImageInfo{ VK_NULL_HANDLE, _image_view, _image_layout };
}

void texture::init_image(const description& desc)
{
	_image_layout = desc.layout;
	_width = desc.size.width();
	_height = desc.size.height();
	_mip_levels = 1;

	vk::ImageCreateInfo image_ci{ {},
		vk::ImageType::e2D,
		desc.format,
		{ desc.size.width(), desc.size.height(), 1 },
		1,
		1,
		desc.samples,
		vk::ImageTiling::eOptimal,
		desc.usage,
		vk::SharingMode::eExclusive,
		0, nullptr,
		vk::ImageLayout::ePreinitialized
	};

	_image = _renderer.device().createImage(image_ci);
	auto mem_reqs = _renderer.device().getImageMemoryRequirements(_image);

	vk::MemoryAllocateInfo mem_alloc{ mem_reqs.size(), _renderer.find_adequate_memory(mem_reqs, vk::MemoryPropertyFlagBits::eDeviceLocal) };

	_memory = _renderer.device().allocateMemory(mem_alloc);
	_renderer.device().bindImageMemory(_image, _memory, 0);

	vk::ImageViewCreateInfo view_ci{ {}, _image, vk::ImageViewType::e2D, desc.format,{}, vk::ImageSubresourceRange{ desc.view_mask, 0, 1, 0, 1 } };
	_image_view = _renderer.device().createImageView(view_ci);
}
