#include "Renderer2D.h"

#include "Material.h"
#include "RendererAPI.h"
#include "VertexArray.h"

namespace Lite {

	namespace {

		ShaderLibrary s_ShaderLibrary;

	}

	void Renderer2D::Init(void* window)
	{
		s_ShaderLibrary.Clear();
		Renderer::Init(window);
	}

	ShaderLibrary& Renderer2D::GetShaderLibrary()
	{
		return s_ShaderLibrary;
	}

	void Renderer2D::SetViewProjection(const Mat4& viewProjection)
	{
		Renderer::SetViewProjection(viewProjection);
	}

	void Renderer2D::Shutdown()
	{
		s_ShaderLibrary.Clear();
		Renderer::Shutdown();
	}

	void Renderer2D::BeginFrame()
	{
		Renderer::BeginFrame();
	}

	void Renderer2D::Draw(const VertexArray& vertexArray, Material& material)
	{
		if (!Renderer::IsFrameActive())
			return;

		material.Bind();
		RendererAPI::DrawIndexed(vertexArray);
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
