#version 400

#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_shading_language_420pack : enable

// layout (binding = 1) uniform sampler2D texture_sampler;

layout (location = 0) in vec2 inUV;
layout (location = 1) in vec3 inNormal;

layout(location = 0) out vec4 out_Color;

void main() 
{  
	out_Color = vec4(inNormal, 1.0); // texture(texture_sampler, inUV);
}
