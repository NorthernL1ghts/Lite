#pragma once

#include "Lite/Renderer/VertexLayout.h"
#include "VulkanUtils.h"

namespace Lite {

	class Shader;

	class VulkanPipeline
	{
	public:
		bool Create(VkDevice device, VkRenderPass renderPass, const Shader& vertexShader, const Shader& fragmentShader, const VertexLayout& layout, bool blend, VkDescriptorSetLayout cameraLayout, VkDescriptorSetLayout materialLayout, VkDescriptorSetLayout textureLayout);
		void Destroy();
		void Bind(VkCommandBuffer commandBuffer, VkExtent2D extent) const;

		VkPipeline Get() const { return m_Pipeline; }
		VkPipelineLayout GetLayout() const { return m_Layout; }

	private:
		VkDevice m_Device = VK_NULL_HANDLE;
		VkPipelineLayout m_Layout = VK_NULL_HANDLE;
		VkPipeline m_Pipeline = VK_NULL_HANDLE;
	};

}
