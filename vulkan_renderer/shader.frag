#version 400

#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_shading_language_420pack : enable


layout(push_constant, std140) uniform push_consts {
	vec4 ambient_color;
	vec4 diffuse_color;
	vec4 specular_color;
	float specular_intensity;
	float normal_map_intensity;
} material;

layout (set = 2, binding = 0) uniform sampler2D diffuse_map;
layout (set = 2, binding = 1) uniform sampler2D normal_map;
layout (set = 2, binding = 2) uniform sampler2D specular_map;

layout (location = 0) in vec2 in_uv;
layout (location = 1) in vec3 in_normal;
layout (location = 2) in vec3 in_frag_pos;

layout(location = 0) out vec4 out_color;

void main() 
{  
	vec3 light_pos = vec3(-20,30,-30);
	vec3 norm = normalize(in_normal);
	vec3 light_dir = normalize(light_pos - in_frag_pos);

	float diff = max(dot(norm, light_dir), 0.0);

	float ambient = 0.1f;

	out_color = (ambient+diff)*texture(diffuse_map, in_uv);
	// out_color = (ambient+diff)*vec4(0.0,1.0,0.0,1.0);
}
