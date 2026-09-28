#pragma once

#include <Lite/Core/Layer.h>
#include <Lite/Scene/ScenePlayer.h>

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
	Lite::ScenePlayer m_Player;
};
