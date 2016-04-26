#include "camera.h"

#include "renderer.h"
#include "pipeline.h"

#include "shared.h"

camera::camera(renderer& renderer) : _renderer(renderer), _ubo(renderer, vk::BufferUsageFlagBits::eVertexBuffer)
{
	// Vulkan clip space has inverted Y and half Z.
	_clip = glm::mat4(	1.0f, 0.0f, 0.0f, 0.0f,
	                  	0.0f, -1.0f, 0.0f, 0.0f,
	                  	0.0f, 0.0f, 0.5f, 0.0f,
	                  	0.0f, 0.0f, 0.5f, 1.0f);


	_camera_position = glm::vec3(-20, 30, -30);
	_view_vector = glm::normalize(glm::vec3(0, 30, 0) - _camera_position);
	_ratio = 800.0f / 600.0f;
	_near = 0.1f;
	_far = 1000.0f;
	_angle = glm::radians(74.0f);
	_up_vector = glm::vec3(0, 1, 0);

	recompute_cache();
	_ubo.update(_cached_vpc);
}

vk::DescriptorBufferInfo camera::descriptor_buffer_info() const
{
	return { _ubo.buffer(), 0, sizeof(glm::mat4) };
}

void camera::update(double dt, const input_state& inputs)
{
	glm::vec2 v(0,0);
	if (inputs.up)
		v.x += 1.0f;
	if (inputs.down)
		v.x -= 1.0f;
	if (inputs.right)
		v.y += 1.0f;
	if (inputs.left)
		v.y -= 1.0f;
	
	if (glm::length(v) == 0.0) return;
	
	v = glm::normalize(v)*100.0f*(float)dt;

	_camera_position += _view_vector*v.x + _right_vector*v.y;

	recompute_cache();
	_ubo.update(_cached_vpc);
}

void camera::attach(pipeline& pipeline, uint32_t set_index)
{	
	_descriptor_set = pipeline.allocate(set_index);
	auto bi = descriptor_buffer_info();
	vk::WriteDescriptorSet write{ *_descriptor_set, 0, 0, 1, vk::DescriptorType::eUniformBuffer, nullptr, &bi, nullptr };
	_renderer.device().updateDescriptorSets(1, &write, 0, nullptr);
}

vk::DescriptorSet camera::descriptor_set() const
{
	return *_descriptor_set;
}

bool camera::cull_sphere(std::pair<glm::vec3, float> bsphere) const
{
	glm::vec3 v = bsphere.first - _camera_position;

	float radius = bsphere.second;

	float az = glm::dot(v, _view_vector);
	if (az > _far + radius || az < _near - radius)
		return true;

	float ay = glm::dot(v, _up_vector);
	float d = _sphere_factor_y*radius;
	az *= _tan_angle;

	if (ay > az + d || ay < -az - d) 
		return true;

	float ax = glm::dot(v, _right_vector);
	az *= _ratio;
	d = _sphere_factor_x * radius;
	if (ax > az + d || ax < -az - d)
		return true;

	return false;
}

bool camera::cull_point(glm::vec3 pt) const
{
	glm::vec3 v = pt - _camera_position;

	float pcz = glm::dot(v, _view_vector);
	if (pcz > _far || pcz < _near) return true;

	float pcy = glm::dot(v, _up_vector);
	float aux = pcz*_tan_angle;
	if (pcy > aux || pcy < -aux) return true;

	float pcx = glm::dot(v, _right_vector);
	aux = aux * _ratio;

	if (pcx > aux || pcx < -aux) return true;

	return false;
}
