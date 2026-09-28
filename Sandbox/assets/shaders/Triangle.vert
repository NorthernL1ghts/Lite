#version 450

layout(location = 0) in vec2 inPosition;
layout(location = 1) in vec3 inBarycentric;

layout(set = 0, binding = 0) uniform Camera
{
	mat4 viewProjection;
	mat4 model;
} camera;

layout(location = 0) out vec3 bary;

void main()
{
	gl_Position = camera.viewProjection * camera.model * vec4(inPosition, 0.0, 1.0);
	bary = inBarycentric;
}
