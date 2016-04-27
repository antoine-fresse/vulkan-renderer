#version 400

#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_shading_language_420pack : enable



layout(push_constant, std140) uniform PushConsts {
	vec4 ambient_color;
	vec4 diffuse_color;
	vec4 specular_color;
	float specular_intensity;
	float normal_map_intensity;
} material;

layout (set = 2, binding = 0) uniform sampler2D diffuse_map;
layout (set = 2, binding = 1) uniform sampler2D normal_map;
layout (set = 2, binding = 2) uniform sampler2D specular_map;

layout (location = 0) in vec2 inUV;
layout (location = 1) in vec3 inNormal;
layout (location = 2) in vec3 inFragPos;

layout(location = 0) out vec4 out_Color;

void main() 
{  
	vec3 light_pos = vec3(-20,30,-30);

	vec3 norm = normalize(inNormal);
	vec3 light_dir = normalize(light_pos - inFragPos);

	float diff = max(dot(norm, light_dir), 0.0);

	float ambient = 0.1f;

	//vec4 diffuse_texel = ;
	//out_Color = vec4(inNormal, 1.0);
	
	if(material.diffuse_color.a < -0.1)
		out_Color = vec4(material.diffuse_color.rgb, 1.0f);
	else
		out_Color = (ambient+diff)*texture(diffuse_map, inUV);
	// if(material.specular_intensity > -0.1)
	// 	out_Color = vec4(0.0,1.0,0.0,1.0);
	// else
	// 	out_Color = vec4(1.0,0.0,0.0,1.0);
	//else
	//	out_Color = diffuse_texel;
}
