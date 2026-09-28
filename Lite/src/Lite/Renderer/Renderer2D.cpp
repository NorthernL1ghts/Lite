#include "Renderer2D.h"

#include "RenderContext.h"
#include "Lite/Platform/Vulkan/VulkanContext.h"

#include <memory>

namespace {

	std::unique_ptr<Lite::RenderContext> s_Context;
	Lite::VulkanContext* s_Vulkan = nullptr;

}

namespace Lite {

	void Renderer2D::Init(void* window)
	{
		s_Context = RenderContext::Create();
		s_Vulkan = static_cast<VulkanContext*>(s_Context.get());
		s_Context->Init(window);
	}

	void Renderer2D::Shutdown()
	{
		s_Vulkan = nullptr;
		s_Context.reset();
	}

	void Renderer2D::BeginFrame()
	{
		if (s_Context)
			s_Context->BeginFrame();
	}

	void Renderer2D::EndFrame()
	{
		if (s_Context)
			s_Context->EndFrame();
	}

	void Renderer2D::OnResize(int width, int height)
	{
		if (s_Context)
			s_Context->OnResize(width, height);
	}

	bool Renderer2D::IsFrameActive()
	{
		return s_Context && s_Context->IsFrameActive();
	}

	VulkanContext& Renderer2D::GetVulkanContext()
	{
		return *s_Vulkan;
	}

}
