#pragma once

#include "Lite/Core/Base.h"

namespace Lite {

	class VulkanContext;

	class LITE_API Renderer2D
	{
	public:
		static void Init(void* window);
		static void Shutdown();

		static void BeginFrame();
		static void EndFrame();
		static void OnResize(int width, int height);
		static bool IsFrameActive();

		static VulkanContext& GetVulkanContext();
	};

}
