#pragma once

#include "Lite/Assets/Shader.h"
#include "Lite/Core/Layer.h"
#include "Lite/Renderer/Material.h"
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
	Lite::Ref<Lite::Shader> m_VertexShader;
	Lite::Ref<Lite::Shader> m_FragmentShader;
	Lite::Ref<Lite::Material> m_Material;
};
