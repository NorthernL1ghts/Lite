#pragma once

#include "Lite/Core/Layer.h"

#include <string>

class EditorLayer final : public Lite::Layer
{
public:
	EditorLayer();

	void OnImGuiRender() override;
	void OnEvent(Lite::Event& event) override;

private:
	void DrawScene();
	void DrawViewport();
	void DrawInspector();
	void DrawConsole();
	void DrawInstrumentation();

	std::string m_Selection = "Triangle";
	bool m_ShowInfo = false;
	bool m_InfoPlaced = false;
};
