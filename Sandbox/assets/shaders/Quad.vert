#version 450

layout(location = 0) in vec2 inPosition;
layout(location = 1) in vec2 inUv;

layout(set = 0, binding = 0) uniform Camera
{
	mat4 viewProjection;
} camera;

layout(location = 0) out vec2 uv;

void main()
{
	gl_Position = camera.viewProjection * vec4(inPosition, 0.0, 1.0);
	uv = inUv;
}
