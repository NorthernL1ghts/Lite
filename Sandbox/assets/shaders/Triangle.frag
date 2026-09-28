#version 450

layout(location = 0) in vec3 bary;
layout(location = 0) out vec4 outColor;

layout(set = 1, binding = 0) uniform MaterialBlock
{
	vec4 color;
	vec2 tiling;
} material;

void main()
{
	vec3 albedo = vec3(0.93, 0.22, 0.28) * bary.x
		+ vec3(0.08, 0.78, 0.42) * bary.y
		+ vec3(0.16, 0.36, 0.96) * bary.z;

	float edge = min(min(bary.x, bary.y), bary.z);
	float coverage = smoothstep(0.0, 0.012, edge);
	float core = smoothstep(0.12, 0.34, edge);
	albedo += albedo * core * 0.12;

	float alpha = coverage * material.color.a;
	vec3 rgb = albedo * material.color.rgb;
	outColor = vec4(rgb * alpha, alpha);
}
