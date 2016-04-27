#version 400

#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_shading_language_420pack : enable

layout(location = 0) in vec3 position;
layout(location = 1) in vec3 normal;
layout(location = 2) in vec3 tangent;
layout(location = 3) in vec2 uv;


layout(set = 0, binding = 0) uniform UBO_view
{
	mat4 mat;
} ubo_view;

layout(set = 1, binding = 0) uniform UBO_model
{
	mat4 mat;
} ubo_model;

layout (location = 0) out vec2 outUV;
layout (location = 1) out vec3 outNormal;
layout (location = 2) out vec3 outFragPos;

void main() 
{
	outUV = uv;
	outNormal = vec3(ubo_model.mat * vec4(normal, 0.0));
	vec4 world_pos = ubo_model.mat * vec4(position, 1.0);
	outFragPos = vec3(world_pos);
	gl_Position = ubo_view.mat * world_pos;
}
