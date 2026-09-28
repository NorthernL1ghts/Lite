#pragma once

#include "VulkanUtils.h"
#include "Lite/Math/Math.h"

namespace Lite {

	class Shader;

	class VulkanPipeline
	{
	public:
		bool Create(VkDevice device, VkRenderPass renderPass, const Shader& vertexShader, const Shader& fragmentShader);
		void Destroy();
		void Bind(VkCommandBuffer commandBuffer, VkExtent2D extent, const Mat4& viewProjection) const;

		VkPipeline Get() const { return m_Pipeline; }
		VkPipelineLayout GetLayout() const { return m_Layout; }

	private:
		VkDevice m_Device = VK_NULL_HANDLE;
		VkPipelineLayout m_Layout = VK_NULL_HANDLE;
		VkPipeline m_Pipeline = VK_NULL_HANDLE;
	};

}
