#pragma once
#include "texture.h"

#include <unordered_map>
#include <memory>

class renderer;

class texture_manager
{
public:
	explicit texture_manager(renderer& renderer)
		: _renderer(renderer)
	{
	}

	std::shared_ptr<texture> find_texture(const std::string& path);

private:
	std::unordered_map<std::string, std::shared_ptr<texture>> _textures;
	renderer& _renderer;
};
