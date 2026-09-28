#include "Renderer2D.h"

#include "RendererAPI.h"

#include "Lite/Assets/Shader.h"

namespace Lite {

	void Renderer2D::Init(void* window)
	{
		Renderer::Init(window);
	}

	bool Renderer2D::SetShaders(const Shader& vertex, const Shader& fragment)
	{
		return Renderer::SetShaders(vertex, fragment);
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

	void Renderer2D::Draw()
	{
		if (!Renderer::IsFrameActive())
			return;

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
