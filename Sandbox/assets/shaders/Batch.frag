#version 450
#extension GL_EXT_nonuniform_qualifier : require

layout(location = 0) in vec4 color;
layout(location = 1) in vec2 uv;
layout(location = 2) flat in int textureIndex;
layout(location = 0) out vec4 outColor;

layout(set = 1, binding = 0) uniform MaterialBlock
{
	vec4 color;
	vec2 tiling;
} material;

layout(set = 2, binding = 0) uniform sampler2D textures[16];

void main()
{
	vec2 sampleUv = uv * max(material.tiling, vec2(1.0));
	vec4 tint = color * material.color;
	vec4 texel = texture(textures[nonuniformEXT(textureIndex)], sampleUv);
	float shade = dot(texel.rgb, vec3(0.299, 0.587, 0.114));
	vec3 graded = mix(tint.rgb * 0.28, tint.rgb, shade);
	float alpha = texel.a * tint.a;
	outColor = vec4(graded * alpha, alpha);
}
