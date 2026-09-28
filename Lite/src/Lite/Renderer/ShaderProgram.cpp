#include "ShaderProgram.h"

#include "Renderer.h"
#include "Lite/Assets/Shader.h"

namespace Lite {

	bool ShaderProgram::Create(const Shader& vertex, const Shader& fragment)
	{
		if (!m_Uniforms.Create(sizeof(CameraUniform)))
			return false;

		if (!m_Pipeline.Create(Renderer::GetDevice(), Renderer::GetRenderPass(), vertex, fragment, m_Uniforms.GetLayout()))
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
		uniform.ViewProjection = m_ViewProjection;
		m_Uniforms.SetData(&uniform, sizeof(uniform));

		VkCommandBuffer commandBuffer = Renderer::GetCommandBuffer();
		m_Pipeline.Bind(commandBuffer, Renderer::GetExtent());
		m_Uniforms.Bind(commandBuffer, m_Pipeline.GetLayout());
	}

	void ShaderProgram::SetViewProjection(const Mat4& viewProjection)
	{
		m_ViewProjection = viewProjection;
	}

}
