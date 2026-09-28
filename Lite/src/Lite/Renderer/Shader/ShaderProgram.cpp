#include <Lite/Renderer/Shader/ShaderProgram.h>

#include <Lite/Renderer/Renderer.h>
#include <Lite/Assets/Shader.h>

namespace Lite {

	bool ShaderProgram::Create(const Shader& vertex, const Shader& fragment, const VertexLayout& layout, bool blend, VkDescriptorSetLayout materialLayout, VkDescriptorSetLayout textureLayout)
	{
		if (!m_Uniforms.Create(sizeof(CameraUniform), 0, VK_SHADER_STAGE_VERTEX_BIT))
			return false;

		if (!m_Pipeline.Create(Renderer::GetDevice(), Renderer::GetRenderPass(), vertex, fragment, layout, blend, m_Uniforms.GetLayout(), materialLayout, textureLayout))
		{
			m_Uniforms.Destroy();
			return false;
		}

		return true;
	}

	void ShaderProgram::Destroy()
	{
		m_Pipeline.Destroy();
		m_Uniforms.Destroy();
	}

	void ShaderProgram::Bind()
	{
		CameraUniform uniform;
		uniform.ViewProjection = Renderer::GetViewProjection();
		uniform.Model = Renderer::GetModel();
		m_Uniforms.SetData(&uniform, sizeof(uniform));

		VkCommandBuffer commandBuffer = Renderer::GetCommandBuffer();
		m_Pipeline.Bind(commandBuffer, Renderer::GetExtent());
		m_Uniforms.Bind(commandBuffer, m_Pipeline.GetLayout());
	}

}
