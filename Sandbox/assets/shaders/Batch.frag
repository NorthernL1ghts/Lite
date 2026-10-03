#version 450
#extension GL_EXT_nonuniform_qualifier : require

layout(location = 0) in vec4 color;
layout(location = 1) in vec2 uv;
layout(location = 2) in vec2 meshUv;
layout(location = 3) flat in int textureIndex;
layout(location = 4) flat in vec4 surface;
layout(location = 0) out vec4 outColor;

layout(set = 1, binding = 0) uniform MaterialBlock
{
	vec4 color;
} material;

layout(set = 2, binding = 0) uniform sampler2D textures[16];

void main()
{
	vec4 tint = color * material.color;
	vec4 texel = texture(textures[nonuniformEXT(textureIndex)], uv);
	float shade = dot(texel.rgb, vec3(0.299, 0.587, 0.114));
	vec3 graded = mix(tint.rgb * 0.28, tint.rgb, shade);
	float alpha = texel.a * tint.a;

	float roughness = clamp(surface.x, 0.0, 1.0);
	float metallic = clamp(surface.y, 0.0, 1.0);
	float emission = max(surface.z, 0.0);
	if (roughness == 1.0 && metallic == 0.0 && emission == 0.0)
	{
		outColor = vec4(graded * alpha, alpha);
		return;
	}

	float smoothWeight = 1.0 - roughness;
	graded = mix(graded, tint.rgb * mix(0.72, 1.0, shade), smoothWeight * 0.65);
	vec3 metal = tint.rgb * mix(0.4, 1.25, shade);
	graded = mix(graded, metal, metallic);

	vec2 lit = vec2(meshUv.x * 2.0 - 1.0, meshUv.y * 2.0 - 1.0 + 0.35);
	float wrap = clamp(dot(lit / max(length(lit), 0.001), vec2(-0.25, 0.97)) * 0.5 + 0.5, 0.0, 1.0);
	float spec = pow(wrap, mix(48.0, 4.0, roughness)) * smoothWeight * smoothWeight;
	graded += mix(vec3(1.0), tint.rgb, metallic) * spec * 0.8;
	graded += tint.rgb * emission;

	outColor = vec4(graded * alpha, alpha);
}
