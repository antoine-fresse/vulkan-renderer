#pragma once
#include "texture.h"

#include <unordered_map>
#include <memory>
#include <assimp/texture.h>

class renderer;

class texture_manager
{
public:
	explicit texture_manager(renderer& renderer);

	std::shared_ptr<texture> create_texture_from_file(const std::string& path);
	std::shared_ptr<texture> create_texture_from_file_buffer(const std::string& name, const void* data, uint32_t size);
	std::shared_ptr<texture> create_texture_from_rgba_buffer(const std::string& name, const void* data, uint32_t width, uint32_t height);

	const vk::Sampler& default_sampler() const { return _default_sampler; }


	void init();

private:
	std::unordered_map<std::string, std::shared_ptr<texture>> _textures;
	renderer& _renderer;
	vk::Sampler _default_sampler;
};
