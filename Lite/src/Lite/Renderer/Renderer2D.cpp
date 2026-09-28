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

	void Renderer2D::SetTransform(const Transform& transform)
	{
		Renderer::SetTransform(transform);
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
		Transform identity;
		Draw(vertexArray, material, identity);
	}

	void Renderer2D::Draw(const VertexArray& vertexArray, Material& material, const Transform& transform)
	{
		Sprite sprite;
		sprite.Transform = transform;
		sprite.Uniform = material.GetUniform();
		sprite.Texture = material.GetTexture();
		Draw(vertexArray, material, sprite);
	}

	void Renderer2D::Draw(const VertexArray& vertexArray, Material& material, const Sprite& sprite)
	{
		if (!Renderer::IsFrameActive())
			return;

		Renderer::SetTransform(sprite.Transform);
		material.SetUniform(sprite.Uniform);
		if (sprite.Texture)
			material.SetTexture(sprite.Texture);

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
