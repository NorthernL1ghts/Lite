#version 450

layout(location = 0) in vec2 inPosition;
layout(location = 1) in vec4 inColor;
layout(location = 2) in vec2 inUv;
layout(location = 3) in vec2 inMeshUv;
layout(location = 4) in float inTextureIndex;
layout(location = 5) in vec4 inSurface;

layout(set = 0, binding = 0) uniform Camera
{
	mat4 viewProjection;
	mat4 model;
} camera;

layout(location = 0) out vec4 color;
layout(location = 1) out vec2 uv;
layout(location = 2) out vec2 meshUv;
layout(location = 3) flat out int textureIndex;
layout(location = 4) flat out vec4 surface;

void main()
{
	gl_Position = camera.viewProjection * vec4(inPosition, 0.0, 1.0);
	color = inColor;
	uv = inUv;
	meshUv = inMeshUv;
	textureIndex = int(inTextureIndex + 0.5);
	surface = inSurface;
}
