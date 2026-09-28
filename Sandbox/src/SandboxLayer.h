#pragma once

#include "Lite/Assets/Texture.h"
#include "Lite/Core/Layer.h"
#include "Lite/Renderer/OrthographicCamera.h"

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
	float m_QuadRotation = 0.0f;
	Lite::Ref<Lite::Texture> m_Checkerboard;
};
