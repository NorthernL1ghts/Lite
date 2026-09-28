#include "SandboxLayer.h"

#include "Lite/Assets/AssetRegistry.h"
#include "Lite/Core/Logger.h"
#include "Lite/Input/Input.h"
#include "Lite/Renderer/Renderer.h"
#include "Lite/Renderer/Renderer2D.h"

SandboxLayer::SandboxLayer()
	: Lite::Layer("Sandbox")
{
}

void SandboxLayer::OnAttach()
{
	LITE_CLIENT_INFO("Layer attached");
	// Lite::Time::SetFPS(60.0f);

	m_VertexShader = Lite::AssetRegistry::Get().Load<Lite::Shader>("shaders/Triangle.vert.spv");
	m_FragmentShader = Lite::AssetRegistry::Get().Load<Lite::Shader>("shaders/Triangle.frag.spv");
	if (!m_VertexShader || !m_FragmentShader)
	{
		LITE_CLIENT_ERROR("Failed to load the triangle shader");
		return;
	}

	m_Material = Lite::Material::Create(*m_VertexShader, *m_FragmentShader);
	if (!m_Material)
	{
		LITE_CLIENT_ERROR("Failed to create the triangle material");
		return;
	}

	m_Material->SetColor({ 1.0f, 1.0f, 1.0f, 1.0f });
	m_Camera.SetProjection(2.0f, 16.0f / 9.0f);
	Lite::Renderer2D::SetViewProjection(m_Camera.GetViewProjection());
}

void SandboxLayer::OnUpdate(Lite::Timestep timestep)
{
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
	VkExtent2D extent = Lite::Renderer::GetExtent();
	float aspect = extent.height > 0 ? static_cast<float>(extent.width) / static_cast<float>(extent.height) : 1.0f;
	m_Camera.SetProjection(2.0f, aspect);
	Lite::Renderer2D::SetViewProjection(m_Camera.GetViewProjection());
	if (m_Material)
		Lite::Renderer2D::Draw(*m_Material);
}

void SandboxLayer::OnDetach()
{
	m_Material.reset();
	m_VertexShader.reset();
	m_FragmentShader.reset();
	LITE_CLIENT_INFO("Layer detached");
}

void SandboxLayer::OnEvent(Lite::Event& event)
{
	if (event.GetType() == Lite::EventType::KeyPressed)
		LITE_CLIENT_TRACE("{}", event.ToString());
}
