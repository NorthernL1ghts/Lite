#pragma once

#include "Lite/Assets/Texture.h"
#include "Lite/Core/Base.h"
#include "Lite/Math/Math.h"
#include "Renderer.h"
#include "ShaderLibrary.h"
#include "UniformBuffer.h"

namespace Lite {

	class Material;
	class VertexArray;

	struct Sprite
	{
		Transform Transform {};
		MaterialUniform Uniform {};
		Ref<Texture> Texture {};
	};

	class LITE_API Renderer2D
	{
	public:
		static void Init(void* window);
		static void Shutdown();
		static ShaderLibrary& GetShaderLibrary();
		static void SetViewProjection(const Mat4& viewProjection);
		static void SetTransform(const Transform& transform);

		static void BeginFrame();
		static void Draw(const VertexArray& vertexArray, Material& material);
		static void Draw(const VertexArray& vertexArray, Material& material, const Transform& transform);
		static void Draw(const VertexArray& vertexArray, Material& material, const Sprite& sprite);
		static void EndFrame();
		static void OnResize(int width, int height);
		static bool IsFrameActive();
	};

}
