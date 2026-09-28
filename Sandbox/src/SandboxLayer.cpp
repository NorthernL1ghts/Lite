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

	Lite::Ref<Lite::Texture> checkerboard = Lite::AssetRegistry::Get().Load<Lite::Texture>("assets/Checkerboard.png");
	if (!checkerboard)
		LITE_CLIENT_ERROR("Failed to load Sandbox texture");

	m_Scene = Lite::CreateScope<Lite::Scene>("Sandbox");

	Lite::SceneObject background;
	background.Name = "Checkerboard";
	background.Kind = Lite::SceneObjectKind::Sprite;
	background.Transform.Scale = { 8.0f, 8.0f, 1.0f };
	background.Tiling = { 8.0f, 8.0f };
	background.Texture = checkerboard;
	background.UseCornerColors = true;
	background.CornerColors[0] = { 0.16f, 0.28f, 0.62f, 1.0f };
	background.CornerColors[1] = { 0.12f, 0.52f, 0.58f, 1.0f };
	background.CornerColors[2] = { 0.93f, 0.58f, 0.24f, 1.0f };
	background.CornerColors[3] = { 0.52f, 0.26f, 0.72f, 1.0f };
	m_Scene->AddObject(std::move(background));

	Lite::SceneObject triangle;
	triangle.Name = "Triangle";
	triangle.Kind = Lite::SceneObjectKind::Triangle;
	triangle.CornerColors[0] = { 0.93f, 0.22f, 0.28f, 1.0f };
	triangle.CornerColors[1] = { 0.08f, 0.78f, 0.42f, 1.0f };
	triangle.CornerColors[2] = { 0.16f, 0.36f, 0.96f, 1.0f };
	m_Scene->AddObject(std::move(triangle));

	Lite::SceneObject quad;
	quad.Name = "Quad";
	quad.Kind = Lite::SceneObjectKind::Quad;
	quad.Transform.Position = { 1.22f, -0.48f, 0.0f };
	quad.Transform.Scale = { 0.62f, 0.62f, 1.0f };
	quad.Color = { 0.86f, 0.16f, 0.18f, 1.0f };
	m_Scene->AddObject(std::move(quad));

	m_Scene->Load();
	m_Scene->Start();
	Lite::Scene::SetActive(m_Scene.get());

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

	m_QuadRotation += 1.15f * timestep.GetSeconds();
	if (Lite::SceneObject* quad = m_Scene->Find("Quad"))
		quad->Transform.SetRotationZ(m_QuadRotation);
}

void SandboxLayer::OnRender()
{
	LITE_PROFILE_SCOPE("Sandbox Render");
	VkExtent2D extent = Lite::Renderer::GetExtent();
	float aspect = extent.height > 0 ? static_cast<float>(extent.width) / static_cast<float>(extent.height) : 1.0f;
	m_Camera.SetProjection(m_ViewSize, aspect);
	Lite::Renderer2D::SetViewProjection(m_Camera.GetViewProjection());

	for (const Lite::SceneObject& object : m_Scene->GetObjects())
		DrawObject(object);
}

void SandboxLayer::DrawObject(const Lite::SceneObject& object)
{
	switch (object.Kind)
	{
		case Lite::SceneObjectKind::Sprite:
			if (object.UseCornerColors)
			{
				Lite::Renderer2D::DrawQuad(
					object.Transform,
					object.Texture,
					object.Tiling,
					object.CornerColors[0],
					object.CornerColors[1],
					object.CornerColors[2],
					object.CornerColors[3]);
			}
			else
			{
				Lite::Renderer2D::DrawQuad(object.Transform, object.Texture, object.Tiling, object.Color);
			}
			break;
		case Lite::SceneObjectKind::Triangle:
			Lite::Renderer2D::DrawTriangle(
				object.Transform,
				object.CornerColors[0],
				object.CornerColors[1],
				object.CornerColors[2]);
			break;
		case Lite::SceneObjectKind::Quad:
			Lite::Renderer2D::DrawQuad(object.Transform, object.Color);
			break;
	}
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
