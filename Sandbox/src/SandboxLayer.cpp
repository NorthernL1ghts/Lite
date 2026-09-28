#include "SandboxLayer.h"

#include "Lite/Assets/AssetRegistry.h"
#include "Lite/Core/Logger.h"
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

	Lite::Renderer2D::SetShaders(*m_VertexShader, *m_FragmentShader);
}

void SandboxLayer::OnDetach()
{
	m_VertexShader.reset();
	m_FragmentShader.reset();
	LITE_CLIENT_INFO("Layer detached");
}

void SandboxLayer::OnEvent(Lite::Event& event)
{
	if (event.GetType() == Lite::EventType::KeyPressed)
		LITE_CLIENT_TRACE("{}", event.ToString());
}
