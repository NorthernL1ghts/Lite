#include "SandboxLayer.h"

#include "Lite/Assets/AssetRegistry.h"
#include "Lite/Core/Events/MouseEvent.h"
#include "Lite/Core/Logger.h"
#include "Lite/Core/Profiler.h"
#include "Lite/Input/Input.h"
#include "Lite/Renderer/Renderer.h"
#include "Lite/Renderer/Renderer2D.h"

#include <cmath>

SandboxLayer::SandboxLayer()
	: Lite::Layer("Sandbox")
{
}

void SandboxLayer::OnAttach()
{
	LITE_CLIENT_INFO("Layer attached");
	// Lite::Time::SetFPS(60.0f);

	m_Checkerboard = Lite::AssetRegistry::Get().Load<Lite::Texture>("assets/Checkerboard.png");
	if (!m_Checkerboard)
		LITE_CLIENT_ERROR("Failed to load Sandbox texture");

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

	Lite::Transform background;
	background.Scale = { 8.0f, 8.0f, 1.0f };
	Lite::Renderer2D::DrawQuad(background, m_Checkerboard, { 8.0f, 8.0f });

	Lite::Transform triangle;
	Lite::Renderer2D::DrawTriangle(
		triangle,
		{ 0.93f, 0.22f, 0.28f, 1.0f },
		{ 0.08f, 0.78f, 0.42f, 1.0f },
		{ 0.16f, 0.36f, 0.96f, 1.0f });

	Lite::Transform redQuad;
	redQuad.Position = { 0.78f, 0.58f, 0.0f };
	redQuad.Scale = { 0.34f, 0.34f, 1.0f };
	Lite::Renderer2D::DrawQuad(redQuad, { 0.86f, 0.16f, 0.18f, 1.0f });
}

void SandboxLayer::OnDetach()
{
	m_Checkerboard.reset();
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
