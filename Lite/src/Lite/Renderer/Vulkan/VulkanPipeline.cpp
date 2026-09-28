#include "VulkanPipeline.h"

#include <Windows.h>

#include <filesystem>
#include <fstream>
#include <vector>

namespace {

	std::filesystem::path ShaderFile(const wchar_t* name)
	{
		wchar_t modulePath[MAX_PATH] {};
		GetModuleFileNameW(nullptr, modulePath, MAX_PATH);
		return std::filesystem::path(modulePath).parent_path() / "shaders" / name;
	}

	std::vector<uint32_t> ReadSpirv(const std::filesystem::path& path)
	{
		std::ifstream file(path, std::ios::binary | std::ios::ate);
		if (!file)
		{
			LITE_ERROR("Failed to open shader {}", path.string());
			return {};
		}

		auto size = static_cast<size_t>(file.tellg());
		if (size == 0 || size % sizeof(uint32_t) != 0)
		{
			LITE_ERROR("Shader {} is not valid SPIR-V", path.string());
			return {};
		}

		std::vector<uint32_t> code(size / sizeof(uint32_t));
		file.seekg(0);
		file.read(reinterpret_cast<char*>(code.data()), static_cast<std::streamsize>(size));
		return code;
	}

	VkShaderModule CreateShader(VkDevice device, const std::vector<uint32_t>& code)
	{
		if (code.empty())
			return VK_NULL_HANDLE;

		VkShaderModuleCreateInfo info {};
		info.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
		info.codeSize = code.size() * sizeof(uint32_t);
		info.pCode = code.data();

		VkShaderModule shader = VK_NULL_HANDLE;
		if (!Lite::CheckVk(vkCreateShaderModule(device, &info, nullptr, &shader), "create shader module"))
			return VK_NULL_HANDLE;

		return shader;
	}

}

namespace Lite {

	bool VulkanPipeline::Create(VkDevice device, VkRenderPass renderPass)
	{
		m_Device = device;

		VkShaderModule vertex = CreateShader(device, ReadSpirv(ShaderFile(L"Triangle.vert.spv")));
		VkShaderModule fragment = CreateShader(device, ReadSpirv(ShaderFile(L"Triangle.frag.spv")));
		if (!vertex || !fragment)
		{
			if (vertex)
				vkDestroyShaderModule(device, vertex, nullptr);
			if (fragment)
				vkDestroyShaderModule(device, fragment, nullptr);
			return false;
		}

		VkPipelineShaderStageCreateInfo stages[2] {};
		stages[0].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
		stages[0].stage = VK_SHADER_STAGE_VERTEX_BIT;
		stages[0].module = vertex;
		stages[0].pName = "main";
		stages[1].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
		stages[1].stage = VK_SHADER_STAGE_FRAGMENT_BIT;
		stages[1].module = fragment;
		stages[1].pName = "main";

		VkVertexInputBindingDescription binding {};
		binding.binding = 0;
		binding.stride = sizeof(float) * 5;
		binding.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

		VkVertexInputAttributeDescription attributes[2] {};
		attributes[0].location = 0;
		attributes[0].binding = 0;
		attributes[0].format = VK_FORMAT_R32G32_SFLOAT;
		attributes[0].offset = 0;
		attributes[1].location = 1;
		attributes[1].binding = 0;
		attributes[1].format = VK_FORMAT_R32G32B32_SFLOAT;
		attributes[1].offset = sizeof(float) * 2;

		VkPipelineVertexInputStateCreateInfo vertexInput {};
		vertexInput.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
		vertexInput.vertexBindingDescriptionCount = 1;
		vertexInput.pVertexBindingDescriptions = &binding;
		vertexInput.vertexAttributeDescriptionCount = 2;
		vertexInput.pVertexAttributeDescriptions = attributes;

		VkPipelineInputAssemblyStateCreateInfo inputAssembly {};
		inputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
		inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;

		VkPipelineViewportStateCreateInfo viewport {};
		viewport.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
		viewport.viewportCount = 1;
		viewport.scissorCount = 1;

		VkPipelineRasterizationStateCreateInfo raster {};
		raster.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
		raster.polygonMode = VK_POLYGON_MODE_FILL;
		raster.cullMode = VK_CULL_MODE_NONE;
		raster.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
		raster.lineWidth = 1.0f;

		VkPipelineMultisampleStateCreateInfo multisample {};
		multisample.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
		multisample.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;

		VkPipelineColorBlendAttachmentState blendAttachment {};
		blendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;

		VkPipelineColorBlendStateCreateInfo blend {};
		blend.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
		blend.attachmentCount = 1;
		blend.pAttachments = &blendAttachment;

		VkDynamicState dynamicStates[] = { VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR };
		VkPipelineDynamicStateCreateInfo dynamic {};
		dynamic.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
		dynamic.dynamicStateCount = 2;
		dynamic.pDynamicStates = dynamicStates;

		VkPushConstantRange pushConstant {};
		pushConstant.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
		pushConstant.offset = 0;
		pushConstant.size = sizeof(float);

		VkPipelineLayoutCreateInfo layoutInfo {};
		layoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
		layoutInfo.pushConstantRangeCount = 1;
		layoutInfo.pPushConstantRanges = &pushConstant;
		if (!CheckVk(vkCreatePipelineLayout(device, &layoutInfo, nullptr, &m_Layout), "create pipeline layout"))
		{
			vkDestroyShaderModule(device, vertex, nullptr);
			vkDestroyShaderModule(device, fragment, nullptr);
			return false;
		}

		VkGraphicsPipelineCreateInfo pipelineInfo {};
		pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
		pipelineInfo.stageCount = 2;
		pipelineInfo.pStages = stages;
		pipelineInfo.pVertexInputState = &vertexInput;
		pipelineInfo.pInputAssemblyState = &inputAssembly;
		pipelineInfo.pViewportState = &viewport;
		pipelineInfo.pRasterizationState = &raster;
		pipelineInfo.pMultisampleState = &multisample;
		pipelineInfo.pColorBlendState = &blend;
		pipelineInfo.pDynamicState = &dynamic;
		pipelineInfo.layout = m_Layout;
		pipelineInfo.renderPass = renderPass;
		pipelineInfo.subpass = 0;

		VkResult result = vkCreateGraphicsPipelines(device, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &m_Pipeline);
		vkDestroyShaderModule(device, vertex, nullptr);
		vkDestroyShaderModule(device, fragment, nullptr);

		if (!CheckVk(result, "create graphics pipeline"))
			return false;

		LITE_INFO("Vulkan graphics pipeline ready");
		return true;
	}

	void VulkanPipeline::Destroy()
	{
		if (m_Pipeline)
		{
			vkDestroyPipeline(m_Device, m_Pipeline, nullptr);
			m_Pipeline = VK_NULL_HANDLE;
		}

		if (m_Layout)
		{
			vkDestroyPipelineLayout(m_Device, m_Layout, nullptr);
			m_Layout = VK_NULL_HANDLE;
		}
	}

	void VulkanPipeline::Bind(VkCommandBuffer commandBuffer, VkExtent2D extent) const
	{
		vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_Pipeline);

		float aspect = extent.height > 0
			? static_cast<float>(extent.width) / static_cast<float>(extent.height)
			: 1.0f;
		vkCmdPushConstants(commandBuffer, m_Layout, VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(float), &aspect);

		VkViewport viewport {};
		viewport.width = static_cast<float>(extent.width);
		viewport.height = static_cast<float>(extent.height);
		viewport.maxDepth = 1.0f;

		VkRect2D scissor {};
		scissor.extent = extent;

		vkCmdSetViewport(commandBuffer, 0, 1, &viewport);
		vkCmdSetScissor(commandBuffer, 0, 1, &scissor);
	}

}
