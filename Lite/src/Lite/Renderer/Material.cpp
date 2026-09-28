#include "Material.h"

#include "Renderer.h"
#include "Lite/Assets/Shader.h"
#include "Lite/Core/Logger.h"

namespace Lite {

	Ref<Material> Material::Create(const Shader& vertex, const Shader& fragment, const VertexLayout& layout, bool blend, const Ref<Texture>& texture)
	{
		auto material = Ref<Material>(new Material());
		if (!material->Init(vertex, fragment, layout, blend, texture))
			return nullptr;

		return material;
	}

	Material::~Material()
	{
		Destroy();
	}

	bool Material::Init(const Shader& vertex, const Shader& fragment, const VertexLayout& layout, bool blend, const Ref<Texture>& texture)
	{
		m_Texture = texture;

		if (!m_Uniforms.Create(sizeof(MaterialUniform), 1, VK_SHADER_STAGE_FRAGMENT_BIT))
			return false;

		VkDescriptorSetLayout textureLayout = m_Texture ? m_Texture->GetSetLayout() : VK_NULL_HANDLE;
		if (!m_Shader.Create(vertex, fragment, layout, blend, m_Uniforms.GetLayout(), textureLayout))
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

	void Material::SetTiling(const Vec2& tiling)
	{
		m_Tiling = tiling;
	}

	void Material::Bind()
	{
		m_Shader.Bind();

		MaterialUniform uniform;
		uniform.Color = m_Color;
		uniform.Tiling = m_Tiling;
		m_Uniforms.SetData(&uniform, sizeof(uniform));
		VkCommandBuffer commandBuffer = Renderer::GetCommandBuffer();
		m_Uniforms.Bind(commandBuffer, m_Shader.GetLayout());
		if (m_Texture)
			m_Texture->Bind(commandBuffer, m_Shader.GetLayout());
	}

}
