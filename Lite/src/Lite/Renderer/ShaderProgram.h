#pragma once

#include "Lite/Core/Base.h"
#include "UniformBuffer.h"
#include "Vulkan/VulkanPipeline.h"

namespace Lite {

	class Shader;

	class LITE_API ShaderProgram
	{
	public:
		bool Create(const Shader& vertex, const Shader& fragment, VkDescriptorSetLayout materialLayout);
		void Destroy();
		void Bind();

		VkPipelineLayout GetLayout() const { return m_Pipeline.GetLayout(); }
		bool IsReady() const { return m_Pipeline.Get() != VK_NULL_HANDLE; }

	private:
		VulkanPipeline m_Pipeline;
		UniformBuffer m_Uniforms;
	};

}
