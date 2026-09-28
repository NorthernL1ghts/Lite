#pragma once

#include "Lite/Assets/Shader.h"
#include "Lite/Core/Layer.h"

#include <memory>

class SandboxLayer final : public Lite::Layer
{
public:
	SandboxLayer();

	void OnAttach() override;
	void OnDetach() override;
	void OnEvent(Lite::Event& event) override;

private:
	std::shared_ptr<Lite::Shader> m_VertexShader;
	std::shared_ptr<Lite::Shader> m_FragmentShader;
};
