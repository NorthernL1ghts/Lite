#pragma once

#include <Explorer.h>
#include <Inspector.h>
#include <SceneBrowser.h>

#include <Lite/Core/Layer.h>
#include <Lite/Renderer/OrthographicCamera.h>
#include <Lite/Scene/Scene.h>

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
	void NewScene();
	void OpenScene(const std::string& path);
	void SaveScene();
	void DuplicateSelected();
	void DeleteSelected();
	void SyncName();
	bool SceneMatches(const std::filesystem::path& path) const;
	void PickObject(float mouseX, float mouseY);
	void ApplyPlayCamera();
	bool BeginGizmo(float mouseX, float mouseY);
	void ApplyGizmo(float mouseX, float mouseY);
	void DrawGizmo();

	enum class GizmoAction
	{
		None,
		Move,
		Rotate,
		Scale
	};

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
	Lite::Scene* m_NamedScene = nullptr;
	char m_Name[128] {};
	uint32_t m_Selected = 0;
	GizmoAction m_Gizmo = GizmoAction::None;
	int m_GizmoCorner = 0;
	float m_GizmoAngle = 0.0f;
	float m_GizmoSpin = 0.0f;
	Lite::Vec2 m_GizmoMouse {};
	Lite::Vec3 m_GizmoPosition {};
	float m_GizmoRotation = 0.0f;
	Lite::Vec3 m_GizmoScale { 1.0f, 1.0f, 1.0f };
	Inspector m_Inspector;
	SceneBrowser m_Browser;
	Explorer m_Explorer;
	bool m_ShowInfo = false;
	bool m_InfoPlaced = false;
};
