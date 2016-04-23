#version 400

#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_shading_language_420pack : enable

layout(location = 0) in vec3 position;
layout(location = 1) in vec3 normal;
layout(location = 2) in vec3 tangent;
layout(location = 3) in vec2 uv;

layout(binding = 0) uniform UBO
{
	mat4 mvp;
} ubo;

layout (location = 0) out vec2 outUV;
layout (location = 1) out vec3 outNormal;

void main() 
{
	outUV = uv;
	outNormal = normal;
	gl_Position = ubo.mvp * vec4(position, 1.0);
}
