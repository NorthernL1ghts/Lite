#pragma once

#include "Lite/Core/Base.h"
#include "Lite/Math/Math.h"
#include "Renderer.h"

namespace Lite {

	class Shader;

	class LITE_API Renderer2D
	{
	public:
		static void Init(void* window);
		static void Shutdown();
		static bool SetShaders(const Shader& vertex, const Shader& fragment);
		static void SetViewProjection(const Mat4& viewProjection);

		static void BeginFrame();
		static void Draw();
		static void EndFrame();
		static void OnResize(int width, int height);
		static bool IsFrameActive();
	};

}
