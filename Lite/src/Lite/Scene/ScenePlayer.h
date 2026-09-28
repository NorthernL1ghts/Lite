#pragma once

#include <Lite/Renderer/OrthographicCamera.h>
#include <Lite/Scene/Scene.h>

#include <string_view>

namespace Lite {

	class LITE_API ScenePlayer
	{
	public:
		bool Open(std::string_view relativePath);
		void Close();
		void Update(float seconds);
		void Render(float width, float height);
		void Zoom(float steps);

		Scene* GetScene() { return m_Scene.get(); }

	private:
		Scope<Scene> m_Scene;
		OrthographicCamera m_Camera;
		float m_ViewSize = 2.0f;
	};

}
