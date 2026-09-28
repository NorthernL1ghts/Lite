#pragma once

#include "Lite/Core/Layer.h"
#include "Lite/Renderer/OrthographicCamera.h"
#include "Lite/Scene/Scene.h"

class SandboxLayer final : public Lite::Layer
{
public:
	SandboxLayer();

	void OnAttach() override;
	void OnDetach() override;
	void OnUpdate(Lite::Timestep timestep) override;
	void OnRender() override;
	void OnEvent(Lite::Event& event) override;

private:
	Lite::OrthographicCamera m_Camera;
	float m_ViewSize = 2.0f;
	Lite::Scope<Lite::Scene> m_Scene;
};
