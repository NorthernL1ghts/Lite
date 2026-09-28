#version 450

layout(location = 0) in vec2 inPosition;
layout(location = 1) in vec3 inBarycentric;

layout(push_constant) uniform Constants
{
	mat4 viewProjection;
} constants;

layout(location = 0) out vec3 bary;

void main()
{
	gl_Position = constants.viewProjection * vec4(inPosition, 0.0, 1.0);
	bary = inBarycentric;
}
