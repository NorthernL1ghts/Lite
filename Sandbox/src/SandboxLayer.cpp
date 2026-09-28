#include <SandboxLayer.h>

#include <Lite/Core/Events/MouseEvent.h>
#include <Lite/Core/Log/Logger.h>
#include <Lite/Core/Profile/Profiler.h>
#include <Lite/Renderer/Renderer.h>

SandboxLayer::SandboxLayer()
	: Lite::Layer("Sandbox")
{
}

void SandboxLayer::OnAttach()
{
	LITE_CLIENT_INFO("Layer attached");
	if (m_Player.Open("assets/scenes/Sandbox.scene"))
	{
		if (Lite::Scene* scene = m_Player.GetScene())
			scene->Play();
	}
}

void SandboxLayer::OnUpdate(Lite::Timestep timestep)
{
	LITE_PROFILE_SCOPE("Sandbox Update");
	m_Player.Update(timestep.GetSeconds());
}

void SandboxLayer::OnRender()
{
	LITE_PROFILE_SCOPE("Sandbox Render");
	VkExtent2D extent = Lite::Renderer::GetExtent();
	m_Player.Render(static_cast<float>(extent.width), static_cast<float>(extent.height));
}

void SandboxLayer::OnDetach()
{
	m_Player.Close();
	LITE_CLIENT_INFO("Layer detached");
}

void SandboxLayer::OnEvent(Lite::Event& event)
{
	Lite::EventDispatcher dispatcher(event);
	dispatcher.Dispatch<Lite::MouseScrolledEvent>([this](Lite::MouseScrolledEvent& scroll)
	{
		m_Player.Zoom(scroll.GetYOffset());
		return false;
	});

	if (event.GetType() == Lite::EventType::KeyPressed)
		LITE_CLIENT_TRACE("{}", event.ToString());
}
