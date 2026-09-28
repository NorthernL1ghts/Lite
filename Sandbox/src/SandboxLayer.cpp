#include <SandboxLayer.h>

#include <Lite/Core/Events/MouseEvent.h>
#include <Lite/Core/Log/Logger.h>
#include <Lite/Core/Profile/Profiler.h>
#include <Lite/Renderer/Renderer.h>
#include <Lite/Renderer/Renderer2D.h>
#include <Lite/Scene/SceneCamera.h>

SandboxLayer::SandboxLayer()
	: Lite::Layer("Sandbox")
{
}

void SandboxLayer::OnAttach()
{
	LITE_CLIENT_INFO("Layer attached");

	std::string scenePath = Lite::Scene::Locate("assets/scenes/Sandbox.scene");
	if (!scenePath.empty())
		m_Scene = Lite::Scene::Open(scenePath);
	if (m_Scene)
	{
		Lite::Scene::SetActive(m_Scene.get());
		m_Scene->Play();
	}
}

void SandboxLayer::OnUpdate(Lite::Timestep timestep)
{
	LITE_PROFILE_SCOPE("Sandbox Update");
	if (!m_Scene)
		return;

	Lite::Entity camera = m_Scene->GetPrimaryCamera();
	if (Lite::TransformComponent* transform = camera.Get<Lite::TransformComponent>())
	{
		float rotation = transform->Local.GetRotationZ();
		Lite::TurnCamera(rotation, timestep.GetSeconds());
		transform->Local.SetRotationZ(rotation);
	}
	else
	{
		float rotation = m_Camera.GetRotation();
		Lite::TurnCamera(rotation, timestep.GetSeconds());
		m_Camera.SetRotation(rotation);
	}

	m_Scene->Update(timestep.GetSeconds());
}

void SandboxLayer::OnRender()
{
	LITE_PROFILE_SCOPE("Sandbox Render");
	if (!m_Scene)
		return;

	VkExtent2D extent = Lite::Renderer::GetExtent();
	Lite::CameraComponent camera;
	camera.Size = m_ViewSize;
	float aspect = Lite::AspectRatio(static_cast<float>(extent.width), static_cast<float>(extent.height));
	Lite::Renderer2D::SetViewProjection(m_Scene->ViewProjection(aspect, m_Camera.GetTransform(), camera));
	m_Scene->Render();
}

void SandboxLayer::OnDetach()
{
	Lite::Scene::Close(m_Scene);
	LITE_CLIENT_INFO("Layer detached");
}

void SandboxLayer::OnEvent(Lite::Event& event)
{
	Lite::EventDispatcher dispatcher(event);
	dispatcher.Dispatch<Lite::MouseScrolledEvent>([this](Lite::MouseScrolledEvent& scroll)
	{
		Lite::CameraComponent* component = m_Scene ? m_Scene->GetPrimaryCamera().Get<Lite::CameraComponent>() : nullptr;
		float& size = component != nullptr ? component->Size : m_ViewSize;
		Lite::ZoomCamera(size, scroll.GetYOffset());
		return false;
	});

	if (event.GetType() == Lite::EventType::KeyPressed)
		LITE_CLIENT_TRACE("{}", event.ToString());
}
