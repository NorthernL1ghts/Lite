#include "SandboxLayer.h"

#include "Lite/Assets/AssetRegistry.h"
#include "Lite/Core/Events/MouseEvent.h"
#include "Lite/Core/Logger.h"
#include "Lite/Core/Profiler.h"
#include "Lite/Input/Input.h"
#include "Lite/Renderer/Renderer.h"
#include "Lite/Renderer/Renderer2D.h"
#include "Lite/Renderer/VertexLayout.h"

#include <cmath>

namespace {

	Lite::VertexLayout TriangleLayout()
	{
		Lite::VertexLayout layout;
		layout.Stride = sizeof(float) * 5;
		layout.Count = 2;
		layout.Attributes[0] = { 0, Lite::VertexFormat::Float2, 0 };
		layout.Attributes[1] = { 1, Lite::VertexFormat::Float3, sizeof(float) * 2 };
		return layout;
	}

	Lite::VertexLayout QuadLayout()
	{
		Lite::VertexLayout layout;
		layout.Stride = sizeof(float) * 4;
		layout.Count = 2;
		layout.Attributes[0] = { 0, Lite::VertexFormat::Float2, 0 };
		layout.Attributes[1] = { 1, Lite::VertexFormat::Float2, sizeof(float) * 2 };
		return layout;
	}

	bool CreateTriangle(Lite::VertexArray& mesh)
	{
		const float vertices[] = {
			 0.00f, -0.72f, 1.0f, 0.0f, 0.0f,
			-0.78f,  0.58f, 0.0f, 1.0f, 0.0f,
			 0.78f,  0.58f, 0.0f, 0.0f, 1.0f
		};
		const uint16_t indices[] = { 0, 1, 2 };
		return mesh.Create(vertices, sizeof(vertices), indices, 3);
	}

	bool CreateBackground(Lite::VertexArray& mesh)
	{
		const float vertices[] = {
			-0.5f, -0.5f, 0.0f, 0.0f,
			 0.5f, -0.5f, 1.0f, 0.0f,
			 0.5f,  0.5f, 1.0f, 1.0f,
			-0.5f,  0.5f, 0.0f, 1.0f
		};
		const uint16_t indices[] = { 0, 1, 2, 2, 3, 0 };
		return mesh.Create(vertices, sizeof(vertices), indices, 6);
	}

}

SandboxLayer::SandboxLayer()
	: Lite::Layer("Sandbox")
{
}

void SandboxLayer::OnAttach()
{
	LITE_CLIENT_INFO("Layer attached");
	// Lite::Time::SetFPS(60.0f);

	auto& shaders = Lite::Renderer2D::GetShaderLibrary();
	m_TriangleVertex = shaders.Load("assets/shaders/Triangle.vert.spv");
	m_TriangleFragment = shaders.Load("assets/shaders/Triangle.frag.spv");
	m_QuadVertex = shaders.Load("assets/shaders/Quad.vert.spv");
	m_QuadFragment = shaders.Load("assets/shaders/Quad.frag.spv");
	m_Checkerboard = Lite::AssetRegistry::Get().Load<Lite::Texture>("assets/Checkerboard.png");
	if (!m_TriangleVertex || !m_TriangleFragment || !m_QuadVertex || !m_QuadFragment || !m_Checkerboard)
	{
		LITE_CLIENT_ERROR("Failed to load Sandbox assets");
		return;
	}

	if (!CreateTriangle(m_Triangle) || !CreateBackground(m_Background))
	{
		LITE_CLIENT_ERROR("Failed to create Sandbox meshes");
		return;
	}

	m_BackgroundMaterial = Lite::Material::Create(*m_QuadVertex, *m_QuadFragment, QuadLayout(), true, m_Checkerboard);
	m_TriangleMaterial = Lite::Material::Create(*m_TriangleVertex, *m_TriangleFragment, TriangleLayout(), true);
	if (!m_BackgroundMaterial || !m_TriangleMaterial)
	{
		LITE_CLIENT_ERROR("Failed to create Sandbox materials");
		return;
	}

	m_BackgroundSprite.Transform.Scale = { 8.0f, 8.0f, 1.0f };
	m_BackgroundSprite.Uniform.Color = { 1.0f, 1.0f, 1.0f, 1.0f };
	m_BackgroundSprite.Uniform.Tiling = { 8.0f, 8.0f };
	m_BackgroundSprite.Texture = m_Checkerboard;

	m_TriangleSprite.Uniform.Color = { 1.0f, 1.0f, 1.0f, 1.0f };
	m_Camera.SetProjection(m_ViewSize, 16.0f / 9.0f);
	Lite::Renderer2D::SetViewProjection(m_Camera.GetViewProjection());
}

void SandboxLayer::OnUpdate(Lite::Timestep timestep)
{
	LITE_PROFILE_SCOPE("Sandbox Update");
	float rotation = m_Camera.GetRotation();
	float step = 1.6f * timestep.GetSeconds();
	if (Lite::Input::IsKeyPressed(Lite::Key::Q))
		rotation += step;
	if (Lite::Input::IsKeyPressed(Lite::Key::E))
		rotation -= step;
	m_Camera.SetRotation(rotation);
}

void SandboxLayer::OnRender()
{
	LITE_PROFILE_SCOPE("Sandbox Render");
	VkExtent2D extent = Lite::Renderer::GetExtent();
	float aspect = extent.height > 0 ? static_cast<float>(extent.width) / static_cast<float>(extent.height) : 1.0f;
	m_Camera.SetProjection(m_ViewSize, aspect);
	Lite::Renderer2D::SetViewProjection(m_Camera.GetViewProjection());

	if (m_BackgroundMaterial)
		Lite::Renderer2D::Draw(m_Background, *m_BackgroundMaterial, m_BackgroundSprite);
	if (m_TriangleMaterial)
		Lite::Renderer2D::Draw(m_Triangle, *m_TriangleMaterial, m_TriangleSprite);
}

void SandboxLayer::OnDetach()
{
	m_TriangleMaterial.reset();
	m_BackgroundMaterial.reset();
	m_Triangle.Destroy();
	m_Background.Destroy();
	m_Checkerboard.reset();
	m_TriangleVertex.reset();
	m_TriangleFragment.reset();
	m_QuadVertex.reset();
	m_QuadFragment.reset();
	LITE_CLIENT_INFO("Layer detached");
}

void SandboxLayer::OnEvent(Lite::Event& event)
{
	Lite::EventDispatcher dispatcher(event);
	dispatcher.Dispatch<Lite::MouseScrolledEvent>([this](Lite::MouseScrolledEvent& scroll)
	{
		float steps = scroll.GetYOffset();
		if (steps == 0.0f)
			return false;

		m_ViewSize *= std::pow(0.85f, steps);
		if (m_ViewSize < 0.25f)
			m_ViewSize = 0.25f;
		if (m_ViewSize > 12.0f)
			m_ViewSize = 12.0f;
		return false;
	});

	if (event.GetType() == Lite::EventType::KeyPressed)
		LITE_CLIENT_TRACE("{}", event.ToString());
}
