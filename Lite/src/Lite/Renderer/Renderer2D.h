#pragma once

#include <Lite/Assets/Texture.h>
#include <Lite/Core/Base.h>
#include <Lite/Math/Math.h>
#include <Lite/Renderer/Renderer.h>
#include <Lite/Renderer/Shader/ShaderLibrary.h>

namespace Lite {

	struct DrawSurface
	{
		float Roughness = 1.0f;
		float Metallic = 0.0f;
		float Emission = 0.0f;
		Vec2 Offset {};
	};

	class LITE_API Renderer2D
	{
	public:
		static void Init(void* window);
		static void Shutdown();
		static ShaderLibrary& GetShaderLibrary();
		static void SetViewProjection(const Mat4& viewProjection);

		static void BeginFrame();
		static void DrawQuad(const Transform& transform, const Vec4& color, const DrawSurface& surface = {});
		static void DrawQuad(const Transform& transform, const Ref<Texture>& texture, const Vec2& tiling, const Vec4& tint, const DrawSurface& surface = {});
		static void DrawQuad(const Transform& transform, const Ref<Texture>& texture, const Vec2& tiling, const Vec4& bottomLeft, const Vec4& bottomRight, const Vec4& topRight, const Vec4& topLeft, const DrawSurface& surface = {});
		static void DrawTriangle(const Transform& transform, const Vec4& first, const Vec4& second, const Vec4& third, const DrawSurface& surface = {});
		static void DrawTriangle(const Transform& transform, const Ref<Texture>& texture, const Vec2& tiling, const Vec4& first, const Vec4& second, const Vec4& third, const DrawSurface& surface = {});
		static void Flush();
		static void EndFrame();
		static void OnResize(int width, int height);
		static bool IsFrameActive();
	};

}
