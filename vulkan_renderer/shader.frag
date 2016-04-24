#version 400

#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_shading_language_420pack : enable

layout (set = 2, binding = 0) uniform sampler2D diffuse_map;
// layout (set = 2, binding = 1) uniform sampler2D normal_map;
// layout (set = 2, binding = 2) uniform sampler2D specular_map;

layout (location = 0) in vec2 inUV;
layout (location = 1) in vec3 inNormal;
layout (location = 2) in vec3 inFragPos;

layout(location = 0) out vec4 out_Color;

void main() 
{  
	vec3 light_pos = vec3(0,10,0);

	vec3 norm = normalize(inNormal);
	vec3 light_dir = normalize(light_pos - inFragPos);
	float diff = max(dot(norm, light_dir), 0.0);

	float ambient = 0.1f;

	//out_Color = vec4(inNormal, 1.0);
	//out_Color = (ambient+diff)*texture(diffuse_map, inUV);
	out_Color = texture(diffuse_map, inUV);
}
