#include "Material.h"

#include "Renderer.h"
#include "Lite/Assets/Shader.h"
#include "Lite/Core/Logger.h"

namespace Lite {

	Ref<Material> Material::Create(const Shader& vertex, const Shader& fragment)
	{
		auto material = Ref<Material>(new Material());
		if (!material->Init(vertex, fragment))
			return nullptr;

		return material;
	}

	Material::~Material()
	{
		Destroy();
	}

	bool Material::Init(const Shader& vertex, const Shader& fragment)
	{
		if (!m_Uniforms.Create(sizeof(MaterialUniform), 1, VK_SHADER_STAGE_FRAGMENT_BIT))
			return false;

		if (!m_Shader.Create(vertex, fragment, m_Uniforms.GetLayout()))
		{
			m_Uniforms.Destroy();
			return false;
		}

		LITE_INFO("Material ready");
		return true;
	}

	void Material::Destroy()
	{
		if (Renderer::GetDevice())
			vkDeviceWaitIdle(Renderer::GetDevice());

		m_Shader.Destroy();
		m_Uniforms.Destroy();
	}

	void Material::SetColor(const Vec4& color)
	{
		m_Color = color;
	}

	void Material::Bind()
	{
		m_Shader.Bind();

		MaterialUniform uniform;
		uniform.Color = m_Color;
		m_Uniforms.SetData(&uniform, sizeof(uniform));
		m_Uniforms.Bind(Renderer::GetCommandBuffer(), m_Shader.GetLayout());
	}

}
