#version 450

layout(location = 0) in vec2 uv;
layout(location = 0) out vec4 outColor;

layout(set = 1, binding = 0) uniform MaterialBlock
{
	vec4 color;
} material;

layout(set = 2, binding = 0) uniform sampler2D image;

void main()
{
	vec4 texel = texture(image, uv);
	outColor = vec4(texel.rgb * material.color.rgb, texel.a * material.color.a);
}
