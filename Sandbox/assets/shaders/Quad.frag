#version 450

layout(location = 0) in vec2 uv;
layout(location = 0) out vec4 outColor;

layout(set = 1, binding = 0) uniform MaterialBlock
{
	vec4 color;
	vec2 tiling;
} material;

layout(set = 2, binding = 0) uniform sampler2D image;

void main()
{
	vec4 texel = texture(image, uv * material.tiling);
	float alpha = texel.a * material.color.a;
	vec3 rgb = texel.rgb * material.color.rgb * alpha;
	outColor = vec4(rgb, alpha);
}
