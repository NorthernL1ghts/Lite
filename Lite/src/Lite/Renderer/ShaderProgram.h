#pragma once

#include "Lite/Core/Base.h"
#include "Lite/Math/Math.h"
#include "UniformBuffer.h"
#include "Vulkan/VulkanPipeline.h"

namespace Lite {

	class Shader;

	class LITE_API ShaderProgram
	{
	public:
		bool Create(const Shader& vertex, const Shader& fragment);
		void Destroy();
		void Bind();

		void SetViewProjection(const Mat4& viewProjection);
		const Mat4& GetViewProjection() const { return m_ViewProjection; }
		bool IsReady() const { return m_Pipeline.Get() != VK_NULL_HANDLE; }

	private:
		VulkanPipeline m_Pipeline;
		UniformBuffer m_Uniforms;
		Mat4 m_ViewProjection = Mat4::Identity();
	};

}
