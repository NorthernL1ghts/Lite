#include <Lite/Scene/ScenePlayer.h>

#include <Lite/Renderer/Renderer2D.h>
#include <Lite/Scene/SceneCamera.h>

namespace Lite {

	bool ScenePlayer::Open(std::string_view relativePath)
	{
		Close();
		std::string path = Scene::Locate(relativePath);
		if (path.empty())
			return false;

		m_Scene = Scene::Open(path);
		if (!m_Scene)
			return false;

		Scene::SetActive(m_Scene.get());
		return true;
	}

	void ScenePlayer::Close()
	{
		Scene::Close(m_Scene);
	}

	void ScenePlayer::Update(float seconds)
	{
		if (!m_Scene)
			return;

		Entity camera = m_Scene->GetPrimaryCamera();
		if (TransformComponent* transform = camera.Get<TransformComponent>())
		{
			float rotation = transform->Local.GetRotationZ();
			TurnCamera(rotation, seconds);
			transform->Local.SetRotationZ(rotation);
		}
		else
		{
			float rotation = m_Camera.GetRotation();
			TurnCamera(rotation, seconds);
			m_Camera.SetRotation(rotation);
		}

		m_Scene->Update(seconds);
	}

	void ScenePlayer::Render(float width, float height)
	{
		if (!m_Scene)
			return;

		CameraComponent camera;
		camera.Size = m_ViewSize;
		Renderer2D::SetViewProjection(m_Scene->ViewProjection(AspectRatio(width, height), m_Camera.GetTransform(), camera));
		m_Scene->Render();
	}

	void ScenePlayer::Zoom(float steps)
	{
		CameraComponent* component = m_Scene ? m_Scene->GetPrimaryCamera().Get<CameraComponent>() : nullptr;
		float& size = component != nullptr ? component->Size : m_ViewSize;
		ZoomCamera(size, steps);
	}

}
