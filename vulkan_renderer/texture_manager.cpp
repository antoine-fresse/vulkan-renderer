#include "texture_manager.h"

std::shared_ptr<texture> texture_manager::find_texture(const std::string& path)
{
	auto it = _textures.find(path);
	if (it != _textures.end()) return it->second;
	return _textures.emplace(path, std::make_shared<texture>(path, _renderer)).first->second;
}
