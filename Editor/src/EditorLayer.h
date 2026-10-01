#pragma once

#include <Explorer.h>
#include <Inspector.h>
#include <SceneBrowser.h>
#include <SceneHistory.h>

#include <Lite/Core/Layer.h>
#include <Lite/Renderer/OrthographicCamera.h>
#include <Lite/Scene/Scene.h>
#include <Lite/Scene/SceneGizmo.h>

#include <cstdint>
#include <filesystem>
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
	void DrawMenu();
	void DrawScene();
	void DrawViewport();
	void DrawConsole();

	void NewProject();
	void OpenProject(const std::filesystem::path& path);
	bool SaveProjectTo(const std::filesystem::path& path);
	void SaveProject();
	void AssignScriptModule(const std::filesystem::path& path);
	void DrawScriptModule();
	void NewScene();
	void OpenScene(const std::string& path);
	void SaveScene();
	void DuplicateSelected();
	void DeleteSelected();
	void UndoSelected();
	void RedoSelected();
	void SyncName();
	bool SceneMatches(const std::filesystem::path& path) const;
	void PickObject(float mouseX, float mouseY);
	void DropSprite(const std::string& path, float mouseX, float mouseY);
	void SaveSelectedPrefab();
	void SaveSelectedMaterial();
	void PlacePrefab(const std::string& path, float mouseX, float mouseY, bool atMouse);
	void ApplyPlayCamera();
	void DrawGizmo();

	Lite::OrthographicCamera m_Camera;
	Lite::Mat4 m_ViewProjection = Lite::Mat4::Identity();
	float m_ViewSize = 2.0f;
	float m_ViewportX = 0.0f;
	float m_ViewportY = 0.0f;
	float m_ViewportW = 0.0f;
	float m_ViewportH = 0.0f;
	float m_WindowW = 0.0f;
	float m_WindowH = 0.0f;
	Lite::Scope<Lite::Scene> m_Scene;
	std::filesystem::path m_ProjectPath;
	std::string m_PlaneNotice;
	std::string m_ScriptSynced;
	std::string m_ScriptBrowserDir;
	char m_ScriptModule[512] {};
	bool m_PickScript = false;
	Lite::Scene* m_NamedScene = nullptr;
	char m_Name[128] {};
	uint32_t m_Selected = 0;
	Lite::GizmoDrag m_Gizmo;
	Inspector m_Inspector;
	SceneHistory m_History;
	SceneBrowser m_Browser;
	Explorer m_Explorer;
	bool m_ShowInfo = false;
	bool m_InfoPlaced = false;
};
