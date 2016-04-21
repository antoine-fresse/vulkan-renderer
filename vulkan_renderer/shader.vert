#version 400

#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_shading_language_420pack : enable

layout(push_constant) uniform push_block 
{
	mat4 mvp;
} constants;

// layout(location = 0) in vec4 pos;

void main() 
{
	vec2 pos[3] = vec2[3]( vec2(-2, 2), vec2(0.0, -2), vec2(2, 2) );

	gl_Position = constants.mvp * vec4(pos[gl_VertexIndex], 0.0, 1.0);
}
