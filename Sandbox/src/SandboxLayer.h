#pragma once

#include "Lite/Assets/Shader.h"
#include "Lite/Assets/Texture.h"
#include "Lite/Core/Layer.h"
#include "Lite/Renderer/Material.h"
#include "Lite/Renderer/OrthographicCamera.h"
#include "Lite/Renderer/VertexArray.h"

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
	Lite::VertexArray m_Triangle;
	Lite::VertexArray m_Background;
	Lite::Ref<Lite::Shader> m_TriangleVertex;
	Lite::Ref<Lite::Shader> m_TriangleFragment;
	Lite::Ref<Lite::Shader> m_QuadVertex;
	Lite::Ref<Lite::Shader> m_QuadFragment;
	Lite::Ref<Lite::Texture> m_Checkerboard;
	Lite::Ref<Lite::Material> m_TriangleMaterial;
	Lite::Ref<Lite::Material> m_BackgroundMaterial;
};
