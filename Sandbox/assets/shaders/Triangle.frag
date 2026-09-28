#version 450

layout(location = 0) in vec3 bary;
layout(location = 0) out vec4 outColor;

layout(set = 1, binding = 0) uniform MaterialBlock
{
	vec4 color;
} material;

void main()
{
	vec3 albedo = vec3(0.93, 0.32, 0.38) * bary.x
		+ vec3(0.18, 0.78, 0.58) * bary.y
		+ vec3(0.28, 0.46, 0.96) * bary.z;

	float edge = min(min(bary.x, bary.y), bary.z);
	float rim = 1.0 - smoothstep(0.0, 0.04, edge);
	float core = smoothstep(0.02, 0.30, edge);

	albedo += vec3(1.0, 0.96, 0.90) * core * 0.28;
	albedo = mix(albedo, vec3(0.96, 0.97, 1.0), rim * 0.45);

	outColor = vec4(albedo * material.color.rgb, material.color.a);
}
