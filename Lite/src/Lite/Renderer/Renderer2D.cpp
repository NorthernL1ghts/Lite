#include "Renderer2D.h"

#include "Material.h"
#include "RendererAPI.h"

namespace Lite {

	void Renderer2D::Init(void* window)
	{
		Renderer::Init(window);
	}

	void Renderer2D::SetViewProjection(const Mat4& viewProjection)
	{
		Renderer::SetViewProjection(viewProjection);
	}

	void Renderer2D::Shutdown()
	{
		Renderer::Shutdown();
	}

	void Renderer2D::BeginFrame()
	{
		Renderer::BeginFrame();
	}

	void Renderer2D::Draw(Material& material)
	{
		if (!Renderer::IsFrameActive())
			return;

		material.Bind();
		RendererAPI::DrawIndexed(Renderer::GetVertexArray());
	}

	void Renderer2D::EndFrame()
	{
		Renderer::EndFrame();
	}

	void Renderer2D::OnResize(int width, int height)
	{
		Renderer::OnResize(width, height);
	}

	bool Renderer2D::IsFrameActive()
	{
		return Renderer::IsFrameActive();
	}

}
