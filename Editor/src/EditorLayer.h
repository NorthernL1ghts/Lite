#pragma once

#include "Lite/Core/Layer.h"
#include "Lite/Renderer/OrthographicCamera.h"
#include "Lite/Scene/Scene.h"

#include <string>

class EditorLayer final : public Lite::Layer
{
public:
	EditorLayer();

	void OnAttach() override;
	void OnDetach() override;
	void OnUpdate(Lite::Timestep timestep) override;
	void OnRender() override;
	void OnImGuiRender() override;
	void OnEvent(Lite::Event& event) override;

private:
	void DrawScene();
	void DrawViewport();
	void DrawInspector();
	void DrawConsole();
	void DrawInstrumentation();

	void NewScene();
	void OpenScene(const std::string& path);
	void SaveScene();
	void SyncName();

	Lite::OrthographicCamera m_Camera;
	float m_ViewSize = 2.0f;
	Lite::Scope<Lite::Scene> m_Scene;
	Lite::Scene* m_NamedScene = nullptr;
	char m_Name[128] {};
	std::string m_Selection = "Triangle";
	bool m_ShowInfo = false;
	bool m_InfoPlaced = false;
};
