#version 450

layout(location = 0) in vec2 inPosition;
layout(location = 1) in vec4 inColor;
layout(location = 2) in vec2 inUv;
layout(location = 3) in float inTextureIndex;

layout(set = 0, binding = 0) uniform Camera
{
	mat4 viewProjection;
	mat4 model;
} camera;

layout(location = 0) out vec4 color;
layout(location = 1) out vec2 uv;
layout(location = 2) flat out int textureIndex;

void main()
{
	gl_Position = camera.viewProjection * vec4(inPosition, 0.0, 1.0);
	color = inColor;
	uv = inUv;
	textureIndex = int(inTextureIndex + 0.5);
}
