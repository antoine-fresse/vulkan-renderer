#include "texture.h"

#include "renderer.h"

#define STB_IMAGE_IMPLEMENTATION
#define STBI_ASSERT(x)
#include "stb/stb_image.h"

texture::texture(const std::string& filepath, renderer& renderer): _renderer(renderer), _image_layout(vk::ImageLayout::eShaderReadOnlyOptimal)
{
	
	
	int x,y,comp;
	unsigned char *data = stbi_load(("data/" + filepath).c_str(), &x, &y, &comp, 4);
	
	_width = x;
	_height = y;
	_components = comp;
	_mip_levels = 1;
	
	vk::Format format = _components == 4 ? vk::Format::eR8G8B8A8Unorm : vk::Format::eR8G8B8Unorm;
	vk::ImageCreateInfo image_ci{ {},
		vk::ImageType::e2D,
		format,
		{_width, _height, 1},
		_mip_levels,
		1,
		vk::SampleCountFlagBits::e1,
		vk::ImageTiling::eLinear,
		vk::ImageUsageFlagBits::eSampled,
		vk::SharingMode::eExclusive,
		0, nullptr,
		vk::ImageLayout::ePreinitialized };


	vk::Device device = _renderer.device();
	
	_image = device.createImage(image_ci);

	_image_mem_reqs = device.getImageMemoryRequirements(_image);

	vk::MemoryAllocateInfo mem_allocate_info{_image_mem_reqs.size(), _renderer.find_adequate_memory(_image_mem_reqs, vk::MemoryPropertyFlagBits::eHostVisible)};
	_memory = device.allocateMemory(mem_allocate_info);

	device.bindImageMemory(_image, _memory, 0);

	vk::ImageSubresource img_sub_res{ vk::ImageAspectFlagBits::eColor, 0, 0 };
	vk::SubresourceLayout sub_res{};


	void* mapped_data;
	sub_res = device.getImageSubresourceLayout(_image, img_sub_res);

	mapped_data = device.mapMemory(_memory, 0, _image_mem_reqs.size(), {});
	
	memcpy(mapped_data, data, _width*_height*_components);

	device.unmapMemory(_memory);
	stbi_image_free(data);

	// Sampler

	vk::SamplerCreateInfo sampler_ci{ {},
		vk::Filter::eLinear,
		vk::Filter::eLinear,
		vk::SamplerMipmapMode::eLinear,
		vk::SamplerAddressMode::eRepeat,
		vk::SamplerAddressMode::eRepeat,
		vk::SamplerAddressMode::eRepeat,
		0.0f,
		VK_TRUE,
		8,
		VK_FALSE,
		vk::CompareOp::eNever,
		0.0f,
		0.0f,
		vk::BorderColor::eFloatOpaqueWhite,
		VK_FALSE
	};
	_sampler = device.createSampler(sampler_ci);

	vk::ImageViewCreateInfo image_view_ci{
		{},
		_image,
		vk::ImageViewType::e2D,
		format,
		vk::ComponentMapping{ vk::ComponentSwizzle::eR, vk::ComponentSwizzle::eG, vk::ComponentSwizzle::eB, _components == 4 ? vk::ComponentSwizzle::eA : vk::ComponentSwizzle::eOne },
		vk::ImageSubresourceRange{vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1}
	};
	_image_view = device.createImageView(image_view_ci);
}

texture::~texture()
{
	vk::Device device = _renderer.device();

	device.destroyImageView(_image_view);
	device.destroySampler(_sampler);
	device.destroyImage(_image);
	device.freeMemory(_memory);
}

vk::DescriptorImageInfo texture::descriptor_image_info() const
{
	return vk::DescriptorImageInfo{ _sampler, _image_view, _image_layout };
}
