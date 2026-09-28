#pragma once

#include <Lite/Assets/Texture.h>
#include <Lite/Core/Base.h>
#include <Lite/Math/Math.h>
#include <Lite/Renderer/Renderer.h>
#include <Lite/Renderer/Shader/ShaderLibrary.h>

namespace Lite {

	class LITE_API Renderer2D
	{
	public:
		static void Init(void* window);
		static void Shutdown();
		static ShaderLibrary& GetShaderLibrary();
		static void SetViewProjection(const Mat4& viewProjection);

		static void BeginFrame();
		static void DrawQuad(const Transform& transform, const Vec4& color);
		static void DrawQuad(const Transform& transform, const Ref<Texture>& texture, const Vec2& tiling = Vec2(1.0f, 1.0f), const Vec4& tint = Vec4(1.0f, 1.0f, 1.0f, 1.0f));
		static void DrawQuad(const Transform& transform, const Ref<Texture>& texture, const Vec2& tiling, const Vec4& bottomLeft, const Vec4& bottomRight, const Vec4& topRight, const Vec4& topLeft);
		static void DrawTriangle(const Transform& transform, const Vec4& first, const Vec4& second, const Vec4& third);
		static void Flush();
		static void EndFrame();
		static void OnResize(int width, int height);
		static bool IsFrameActive();
	};

}
