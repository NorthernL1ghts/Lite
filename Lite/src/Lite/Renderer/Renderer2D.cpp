#include "Renderer2D.h"

#include "Material.h"
#include "RendererAPI.h"
#include "VertexArray.h"
#include "VertexLayout.h"
#include "Vulkan/VulkanSync.h"

#include "Lite/Core/Logger.h"
#include "Lite/Core/Profiler.h"

#include <array>
#include <vector>

namespace Lite {

	namespace {

		constexpr uint32_t MaxQuads = 4096;
		constexpr uint32_t MaxVertices = MaxQuads * 4;
		constexpr uint32_t MaxIndices = MaxQuads * 6;

		struct BatchVertex
		{
			float x = 0.0f;
			float y = 0.0f;
			float r = 1.0f;
			float g = 1.0f;
			float b = 1.0f;
			float a = 1.0f;
			float u = 0.0f;
			float v = 0.0f;
		};

		ShaderLibrary s_ShaderLibrary;
		std::vector<BatchVertex> s_Vertices;
		std::vector<uint16_t> s_Indices;
		Ref<Texture> s_White;
		Ref<Texture> s_Texture;
		Ref<Material> s_Material;
		std::array<VertexArray, VulkanSync::FramesInFlight> s_Meshes;
		bool s_Ready = false;

		VertexLayout BatchLayout()
		{
			VertexLayout layout;
			layout.Stride = sizeof(BatchVertex);
			layout.Count = 3;
			layout.Attributes[0] = { 0, VertexFormat::Float2, 0 };
			layout.Attributes[1] = { 1, VertexFormat::Float4, sizeof(float) * 2 };
			layout.Attributes[2] = { 2, VertexFormat::Float2, sizeof(float) * 6 };
			return layout;
		}

		void StartBatch(const Ref<Texture>& texture)
		{
			Ref<Texture> next = texture ? texture : s_White;
			if (!s_Vertices.empty() && s_Texture.get() != next.get())
				Renderer2D::Flush();

			s_Texture = next;
		}

		void PushQuad(const Transform& transform, const Vec4& color, const Vec2& tiling)
		{
			if (s_Vertices.size() + 4 > MaxVertices || s_Indices.size() + 6 > MaxIndices)
				Renderer2D::Flush();

			const Vec2 corners[4] = {
				{ -0.5f, -0.5f },
				{  0.5f, -0.5f },
				{  0.5f,  0.5f },
				{ -0.5f,  0.5f }
			};
			const Vec2 uvs[4] = {
				{ 0.0f, 0.0f },
				{ tiling.x, 0.0f },
				{ tiling.x, tiling.y },
				{ 0.0f, tiling.y }
			};

			uint16_t base = static_cast<uint16_t>(s_Vertices.size());
			for (int corner = 0; corner < 4; ++corner)
			{
				Vec3 world = transform.TransformPoint({ corners[corner].x, corners[corner].y, 0.0f });
				s_Vertices.push_back({
					world.x, world.y,
					color.x, color.y, color.z, color.w,
					uvs[corner].x, uvs[corner].y
				});
			}

			s_Indices.push_back(base + 0);
			s_Indices.push_back(base + 1);
			s_Indices.push_back(base + 2);
			s_Indices.push_back(base + 2);
			s_Indices.push_back(base + 3);
			s_Indices.push_back(base + 0);
		}

	}

	void Renderer2D::Init(void* window)
	{
		s_ShaderLibrary.Clear();
		s_Ready = false;
		Renderer::Init(window);
		if (!Renderer::GetDevice())
			return;

		const uint8_t whitePixel[4] = { 255, 255, 255, 255 };
		s_White = Texture::Create(1, 1, whitePixel);

		auto& shaders = s_ShaderLibrary;
		Ref<Shader> vertex = shaders.Load("assets/shaders/Batch.vert.spv");
		Ref<Shader> fragment = shaders.Load("assets/shaders/Batch.frag.spv");
		if (!s_White || !vertex || !fragment)
		{
			LITE_ERROR("Renderer2D batch shaders or white texture failed to load");
			s_White.reset();
			return;
		}

		std::vector<BatchVertex> vertices(MaxVertices);
		std::vector<uint16_t> indices(MaxIndices);
		for (VertexArray& mesh : s_Meshes)
		{
			if (!mesh.Create(vertices.data(), static_cast<uint32_t>(vertices.size() * sizeof(BatchVertex)), indices.data(), MaxIndices))
			{
				LITE_ERROR("Renderer2D batch mesh failed to allocate");
				return;
			}
		}

		s_Material = Material::Create(*vertex, *fragment, BatchLayout(), true, s_White);
		if (!s_Material)
			return;

		s_Vertices.reserve(MaxVertices);
		s_Indices.reserve(MaxIndices);
		s_Ready = true;
		LITE_INFO("Renderer2D batch ready ({} quads)", MaxQuads);
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
		s_Ready = false;
		s_Vertices.clear();
		s_Indices.clear();
		s_Texture.reset();
		s_Material.reset();
		s_White.reset();
		for (VertexArray& mesh : s_Meshes)
			mesh.Destroy();
		s_ShaderLibrary.Clear();
		Renderer::Shutdown();
	}

	void Renderer2D::BeginFrame()
	{
		Renderer::BeginFrame();
		s_Vertices.clear();
		s_Indices.clear();
		s_Texture.reset();
	}

	void Renderer2D::DrawQuad(const Transform& transform, const Vec4& color)
	{
		if (!s_Ready || !Renderer::IsFrameActive())
			return;

		StartBatch({});
		PushQuad(transform, color, { 1.0f, 1.0f });
	}

	void Renderer2D::DrawQuad(const Transform& transform, const Ref<Texture>& texture, const Vec2& tiling, const Vec4& tint)
	{
		if (!s_Ready || !Renderer::IsFrameActive())
			return;

		StartBatch(texture);
		PushQuad(transform, tint, tiling);
	}

	void Renderer2D::DrawTriangle(const Transform& transform, const Vec4& first, const Vec4& second, const Vec4& third)
	{
		if (!s_Ready || !Renderer::IsFrameActive())
			return;

		if (s_Vertices.size() + 3 > MaxVertices || s_Indices.size() + 3 > MaxIndices)
			Flush();

		StartBatch({});

		const Vec2 corners[3] = {
			{  0.00f, -0.72f },
			{ -0.78f,  0.58f },
			{  0.78f,  0.58f }
		};
		const Vec4 colors[3] = { first, second, third };
		uint16_t base = static_cast<uint16_t>(s_Vertices.size());
		for (int corner = 0; corner < 3; ++corner)
		{
			Vec3 world = transform.TransformPoint({ corners[corner].x, corners[corner].y, 0.0f });
			s_Vertices.push_back({
				world.x, world.y,
				colors[corner].x, colors[corner].y, colors[corner].z, colors[corner].w,
				0.0f, 0.0f
			});
			s_Indices.push_back(base + static_cast<uint16_t>(corner));
		}
	}

	void Renderer2D::Flush()
	{
		LITE_PROFILE_SCOPE("Batch Flush");
		if (!s_Ready || !Renderer::IsFrameActive() || s_Vertices.empty() || !s_Material || !s_Texture)
			return;

		uint32_t frame = Renderer::GetFrameIndex();
		if (frame >= s_Meshes.size())
			return;

		if (!s_Meshes[frame].Upload(
			s_Vertices.data(),
			static_cast<uint32_t>(s_Vertices.size() * sizeof(BatchVertex)),
			s_Indices.data(),
			static_cast<uint32_t>(s_Indices.size())))
			return;

		Renderer::SetModel(Mat4::Identity());
		s_Material->SetColor({ 1.0f, 1.0f, 1.0f, 1.0f });
		s_Material->SetTiling({ 1.0f, 1.0f });
		s_Material->SetTexture(s_Texture);
		s_Material->Bind();
		RendererAPI::DrawIndexed(s_Meshes[frame]);

		s_Vertices.clear();
		s_Indices.clear();
	}

	void Renderer2D::EndFrame()
	{
		if (Renderer::IsFrameActive())
			Flush();

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
