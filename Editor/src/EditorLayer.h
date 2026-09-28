#pragma once

#include "Lite/Core/Layer.h"

class EditorLayer final : public Lite::Layer
{
public:
	EditorLayer();

	void OnImGuiRender() override;
	void OnEvent(Lite::Event& event) override;

private:
	enum class Selection
	{
		Checkerboard,
		Triangle,
		Quad
	};

	void DrawScene();
	void DrawViewport();
	void DrawInspector();
	void DrawConsole();
	void DrawInstrumentation();

	Selection m_Selection = Selection::Triangle;
	bool m_ShowInfo = false;
	bool m_InfoPlaced = false;
};
