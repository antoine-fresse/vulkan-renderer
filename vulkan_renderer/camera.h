#pragma once

#include "vulkan_include.h"
#include "math_include.h"
#include "ubo.h"

class renderer;

class camera
{
public:
	camera(renderer& renderer);

	const glm::mat4& matrix()
	{
		if (_dirty) recompute_cache();
		return _cached_vpc;
	}

	const glm::mat4& clip() const
	{
		return _clip;
	}

	void clip(const glm::mat4& mat)
	{
		_clip = mat;
		_dirty = true;
	}

	const glm::mat4& projection() const
	{
		return _projection;
	}

	void projection(const glm::mat4& mat)
	{
		_projection = mat;
		_dirty = true;
	}

	const glm::mat4& view() const
	{
		return _view;
	}

	void view(const glm::mat4& mat)
	{
		_view = mat;
		_dirty = true;
	}

	vk::DescriptorBufferInfo descriptor_buffer_info() const;


private:
	renderer& _renderer;

	bool _dirty = false;

	void recompute_cache()
	{
		_cached_vpc = _clip * _projection * _view; 
		_dirty = false;
	};

	glm::mat4 _cached_vpc;

	glm::mat4 _clip;
	glm::mat4 _projection;
	glm::mat4 _view;

	single_ubo<glm::mat4, false> _ubo;
};