#pragma once

#include "vulkan_include.h"
#include "math_include.h"
#include "ubo.h"

struct input_state;
class renderer;
class pipeline;
class managed_descriptor_set;

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
	
	void update(double dt, const input_state& inputs);

	vk::DescriptorBufferInfo descriptor_buffer_info() const;

	void attach(pipeline& pipeline, uint32_t set_index);

	vk::DescriptorSet descriptor_set() const;

	bool cull_sphere(std::pair<glm::vec3, float> bsphere) const;
private:
	renderer& _renderer;

	bool _dirty = false;

	void recompute_cache()
	{
		_projection = glm::perspective(_angle, _ratio, _near, _far);
		_view = glm::lookAt(_camera_position, _camera_position+_view_vector, _up_vector);

		_right_vector = glm::cross(_view_vector, _up_vector);
		_up_vector = glm::cross(_right_vector, _view_vector);

		_cached_vpc = _clip * _projection * _view; 
		_dirty = false;

		float hang = _angle / 2;
		_tan_angle = glm::tan(hang);
		_sphere_factor_y = 1.0f / glm::cos(hang);
		_sphere_factor_x = 1.0f / glm::cos(glm::atan(_tan_angle * _ratio));
	};

	glm::mat4 _cached_vpc;

	glm::mat4 _clip;
	glm::mat4 _projection;
	glm::mat4 _view;

	// Proj
	float _near, _far;
	float _angle;
	float _ratio;

	// View
	glm::vec3 _view_vector; // Z
	glm::vec3 _up_vector; // Y
	glm::vec3 _right_vector; // X
	glm::vec3 _camera_position;

	single_ubo<glm::mat4, false> _ubo;

	std::shared_ptr<managed_descriptor_set> _descriptor_set = nullptr;

	// Culling Info
	glm::vec3 _cam_x, _cam_y, _cam_z;
	float _sphere_factor_x, _sphere_factor_y;
	float _tan_angle;

};