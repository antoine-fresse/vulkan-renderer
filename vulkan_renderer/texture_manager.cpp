#include "texture_manager.h"
#include "renderer.h"


std::shared_ptr<texture> texture_manager::find_texture(const std::string& path)
{
	auto it = _textures.find(path);
	if (it != _textures.end()) return it->second;
	return _textures.emplace(path, std::make_shared<texture>(path, _renderer)).first->second;
}

void texture_manager::init()
{
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
	_default_sampler = _renderer.device().createSampler(sampler_ci);
}

texture_manager::texture_manager(renderer& renderer) : _renderer(renderer)
{
}
