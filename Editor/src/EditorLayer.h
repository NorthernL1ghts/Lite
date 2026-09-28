#pragma once

#include <Lite/Core/Layer.h>
#include <Lite/Renderer/OrthographicCamera.h>
#include <Lite/Scene/Scene.h>

#include <cstdint>
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
	void DrawOpenDialog();
	void DrawSaveDialog();
	void OpenBrowser();
	void DrawFileBrowser(bool save);
	void ApplyDirectoryText();
	std::string BrowserSelection() const;
	void DrawScene();
	void DrawViewport();
	void DrawInspector();
	void DrawConsole();
	void DrawInstrumentation();

	void NewScene();
	void OpenScene(const std::string& path);
	void SaveScene();
	void SyncName();
	void PickObject(float mouseX, float mouseY);
	void SyncEntityFields(Lite::Entity entity);
	void ApplyPlayCamera();

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
	Lite::Scene* m_NamedScene = nullptr;
	char m_Name[128] {};
	char m_ObjectName[128] {};
	char m_ShaderText[128] {};
	char m_TextureText[260] {};
	uint32_t m_Selected = 0;
	uint32_t m_SyncedId = 0;
	std::string m_BrowserDirectory;
	char m_DirectoryText[512] {};
	char m_FileName[256] {};
	bool m_ShowOpen = false;
	bool m_ShowSave = false;
	bool m_FocusFile = false;
	bool m_ShowInfo = false;
	bool m_InfoPlaced = false;
};
