#version 450

layout(location = 0) in vec4 color;
layout(location = 1) in vec2 uv;
layout(location = 0) out vec4 outColor;

layout(set = 1, binding = 0) uniform MaterialBlock
{
	vec4 color;
	vec2 tiling;
} material;

layout(set = 2, binding = 0) uniform sampler2D image;

void main()
{
	vec2 sampleUv = uv * max(material.tiling, vec2(1.0));
	vec4 tint = color * material.color;
	vec4 texel = texture(image, sampleUv);
	float alpha = texel.a * tint.a;
	vec3 rgb = texel.rgb * tint.rgb * alpha;
	outColor = vec4(rgb, alpha);
}
