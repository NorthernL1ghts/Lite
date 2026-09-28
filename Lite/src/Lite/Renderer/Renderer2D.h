#pragma once

#include "Lite/Core/Base.h"
#include "Lite/Math/Math.h"
#include "Renderer.h"

namespace Lite {

	class Material;
	class VertexArray;

	class LITE_API Renderer2D
	{
	public:
		static void Init(void* window);
		static void Shutdown();
		static void SetViewProjection(const Mat4& viewProjection);

		static void BeginFrame();
		static void Draw(const VertexArray& vertexArray, Material& material);
		static void EndFrame();
		static void OnResize(int width, int height);
		static bool IsFrameActive();
	};

}
