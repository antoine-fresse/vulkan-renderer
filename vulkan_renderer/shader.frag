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

layout(set = 0, binding = 0) uniform UBO_view
{
	mat4 mat;
	vec3 eye_pos;
} ubo_view;

layout (location = 0) in vec2 in_uv;
layout (location = 1) in vec3 in_normal;
layout (location = 2) in vec3 in_frag_pos;
layout (location = 3) in vec3 in_tangent;

layout(location = 0) out vec4 out_color;


vec3 calc_bumped_normal()
{
	vec3 normal = normalize(in_normal);
	vec3 tangent = normalize(in_tangent);
	tangent = normalize(tangent - dot(tangent, normal) * normal);
	vec3 bitangent = cross(tangent, normal);
	vec3 bump_map_normal = texture(normal_map, in_uv).xyz;
	bump_map_normal = normalize(2.0 * bump_map_normal - vec3(1.0, 1.0, 1.0));
	mat3 TBN = mat3(tangent, bitangent, normal);
	vec3 new_normal = TBN * bump_map_normal;
	return normalize(new_normal);
}

void main() 
{  
	vec3 light_pos = vec3(-20,30,-30);
	vec3 norm = normalize(in_normal);

	vec3 diffuse_texel = texture(diffuse_map, in_uv).xyz;

	if(material.normal_map_intensity > 0.0)
		norm = calc_bumped_normal();
	

	vec3 light_dir = normalize(light_pos - in_frag_pos);

	float diffuse_factor = max(dot(norm, light_dir), 0.0);

	vec3 view_dir = normalize(ubo_view.eye_pos - in_frag_pos);
	vec3 reflect_dir = reflect(-light_dir, norm);

	float spec = pow(max(dot(view_dir, reflect_dir), 0.0), max(material.specular_intensity, 0.0));
	vec3 specular = vec3(0,0,0);
	if(material.specular_intensity > 0.0)
		specular = 0.1 * spec * texture(specular_map, in_uv).xyz;

	out_color = vec4((0.1 + diffuse_factor) * diffuse_texel + specular, 1.0);
	// out_color = vec4(in_uv, 0.0, 1.0);
}
