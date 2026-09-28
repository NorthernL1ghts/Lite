#pragma once

#include <memory>

namespace Lite {

	class RenderContext
	{
	public:
		virtual ~RenderContext() = default;

		virtual void Init(void* window) = 0;
		virtual void BeginFrame() = 0;
		virtual void EndFrame() = 0;
		virtual void OnResize(int width, int height) = 0;
		virtual bool IsFrameActive() const = 0;

		static std::unique_ptr<RenderContext> Create();
	};

}
