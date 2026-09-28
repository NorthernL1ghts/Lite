#include "SandboxLayer.h"

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

	m_Scene = Lite::Scene::Open("assets/scenes/Sandbox.scene");
	if (m_Scene)
	{
		Lite::Scene::SetActive(m_Scene.get());
		m_Scene->Play();
	}

	m_Camera.SetProjection(m_ViewSize, 16.0f / 9.0f);
}

void SandboxLayer::OnUpdate(Lite::Timestep timestep)
{
	LITE_PROFILE_SCOPE("Sandbox Update");
	if (!m_Scene)
		return;

	Lite::Entity camera = m_Scene->GetPrimaryCamera();
	Lite::TransformComponent* transform = camera.Get<Lite::TransformComponent>();
	float rotation = transform != nullptr ? transform->Local.GetRotationZ() : m_Camera.GetRotation();
	float step = 1.6f * timestep.GetSeconds();
	if (Lite::Input::IsKeyPressed(Lite::Key::Q))
		rotation += step;
	if (Lite::Input::IsKeyPressed(Lite::Key::E))
		rotation -= step;
	if (transform != nullptr)
		transform->Local.SetRotationZ(rotation);
	else
		m_Camera.SetRotation(rotation);

	m_Scene->Update(timestep.GetSeconds());
}

void SandboxLayer::OnRender()
{
	LITE_PROFILE_SCOPE("Sandbox Render");
	if (!m_Scene)
		return;

	VkExtent2D extent = Lite::Renderer::GetExtent();
	float aspect = extent.height > 0 ? static_cast<float>(extent.width) / static_cast<float>(extent.height) : 1.0f;
	Lite::Entity camera = m_Scene->GetPrimaryCamera();
	Lite::CameraComponent* component = camera.Get<Lite::CameraComponent>();
	Lite::TransformComponent* transform = camera.Get<Lite::TransformComponent>();
	Lite::OrthographicCamera view;
	if (transform != nullptr)
	{
		view.SetPosition(transform->Local.Position);
		view.SetRotation(transform->Local.GetRotationZ());
	}
	view.SetProjection(component != nullptr ? component->Size : m_ViewSize, aspect);
	Lite::Renderer2D::SetViewProjection(view.GetViewProjection());
	m_Scene->Render();
}

void SandboxLayer::OnDetach()
{
	if (m_Scene)
	{
		m_Scene->Stop();
		if (Lite::Scene::GetActive() == m_Scene.get())
			Lite::Scene::SetActive(nullptr);
		m_Scene.reset();
	}

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

		Lite::CameraComponent* component = m_Scene ? m_Scene->GetPrimaryCamera().Get<Lite::CameraComponent>() : nullptr;
		float& size = component != nullptr ? component->Size : m_ViewSize;
		size *= std::pow(0.85f, steps);
		if (size < 0.25f)
			size = 0.25f;
		if (size > 12.0f)
			size = 12.0f;
		return false;
	});

	if (event.GetType() == Lite::EventType::KeyPressed)
		LITE_CLIENT_TRACE("{}", event.ToString());
}
