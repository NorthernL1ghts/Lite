#include "VulkanPipeline.h"

namespace {

	const uint32_t VertexShader[] = {
		0x07230203, 0x00010000, 0x000D000B, 0x00000021, 0x00000000, 0x00020011, 0x00000001, 0x0006000B,
		0x00000001, 0x4C534C47, 0x6474732E, 0x3035342E, 0x00000000, 0x0003000E, 0x00000000, 0x00000001,
		0x0009000F, 0x00000000, 0x00000004, 0x6E69616D, 0x00000000, 0x0000000D, 0x00000012, 0x0000001D,
		0x0000001F, 0x00030003, 0x00000002, 0x000001C2, 0x000A0004, 0x475F4C47, 0x4C474F4F, 0x70635F45,
		0x74735F70, 0x5F656C79, 0x656E696C, 0x7269645F, 0x69746365, 0x00006576, 0x00080004, 0x475F4C47,
		0x4C474F4F, 0x6E695F45, 0x64756C63, 0x69645F65, 0x74636572, 0x00657669, 0x00040005, 0x00000004,
		0x6E69616D, 0x00000000, 0x00060005, 0x0000000B, 0x505F6C67, 0x65567265, 0x78657472, 0x00000000,
		0x00060006, 0x0000000B, 0x00000000, 0x505F6C67, 0x7469736F, 0x006E6F69, 0x00070006, 0x0000000B,
		0x00000001, 0x505F6C67, 0x746E696F, 0x657A6953, 0x00000000, 0x00070006, 0x0000000B, 0x00000002,
		0x435F6C67, 0x4470696C, 0x61747369, 0x0065636E, 0x00070006, 0x0000000B, 0x00000003, 0x435F6C67,
		0x446C6C75, 0x61747369, 0x0065636E, 0x00030005, 0x0000000D, 0x00000000, 0x00050005, 0x00000012,
		0x6F506E69, 0x69746973, 0x00006E6F, 0x00050005, 0x0000001D, 0x67617266, 0x6F6C6F43, 0x00000072,
		0x00040005, 0x0000001F, 0x6F436E69, 0x00726F6C, 0x00030047, 0x0000000B, 0x00000002, 0x00050048,
		0x0000000B, 0x00000000, 0x0000000B, 0x00000000, 0x00050048, 0x0000000B, 0x00000001, 0x0000000B,
		0x00000001, 0x00050048, 0x0000000B, 0x00000002, 0x0000000B, 0x00000003, 0x00050048, 0x0000000B,
		0x00000003, 0x0000000B, 0x00000004, 0x00040047, 0x00000012, 0x0000001E, 0x00000000, 0x00040047,
		0x0000001D, 0x0000001E, 0x00000000, 0x00040047, 0x0000001F, 0x0000001E, 0x00000001, 0x00020013,
		0x00000002, 0x00030021, 0x00000003, 0x00000002, 0x00030016, 0x00000006, 0x00000020, 0x00040017,
		0x00000007, 0x00000006, 0x00000004, 0x00040015, 0x00000008, 0x00000020, 0x00000000, 0x0004002B,
		0x00000008, 0x00000009, 0x00000001, 0x0004001C, 0x0000000A, 0x00000006, 0x00000009, 0x0006001E,
		0x0000000B, 0x00000007, 0x00000006, 0x0000000A, 0x0000000A, 0x00040020, 0x0000000C, 0x00000003,
		0x0000000B, 0x0004003B, 0x0000000C, 0x0000000D, 0x00000003, 0x00040015, 0x0000000E, 0x00000020,
		0x00000001, 0x0004002B, 0x0000000E, 0x0000000F, 0x00000000, 0x00040017, 0x00000010, 0x00000006,
		0x00000002, 0x00040020, 0x00000011, 0x00000001, 0x00000010, 0x0004003B, 0x00000011, 0x00000012,
		0x00000001, 0x0004002B, 0x00000006, 0x00000014, 0x00000000, 0x0004002B, 0x00000006, 0x00000015,
		0x3F800000, 0x00040020, 0x00000019, 0x00000003, 0x00000007, 0x00040017, 0x0000001B, 0x00000006,
		0x00000003, 0x00040020, 0x0000001C, 0x00000003, 0x0000001B, 0x0004003B, 0x0000001C, 0x0000001D,
		0x00000003, 0x00040020, 0x0000001E, 0x00000001, 0x0000001B, 0x0004003B, 0x0000001E, 0x0000001F,
		0x00000001, 0x00050036, 0x00000002, 0x00000004, 0x00000000, 0x00000003, 0x000200F8, 0x00000005,
		0x0004003D, 0x00000010, 0x00000013, 0x00000012, 0x00050051, 0x00000006, 0x00000016, 0x00000013,
		0x00000000, 0x00050051, 0x00000006, 0x00000017, 0x00000013, 0x00000001, 0x00070050, 0x00000007,
		0x00000018, 0x00000016, 0x00000017, 0x00000014, 0x00000015, 0x00050041, 0x00000019, 0x0000001A,
		0x0000000D, 0x0000000F, 0x0003003E, 0x0000001A, 0x00000018, 0x0004003D, 0x0000001B, 0x00000020,
		0x0000001F, 0x0003003E, 0x0000001D, 0x00000020, 0x000100FD, 0x00010038
	};

	const uint32_t FragmentShader[] = {
		0x07230203, 0x00010000, 0x000D000B, 0x00000013, 0x00000000, 0x00020011, 0x00000001, 0x0006000B,
		0x00000001, 0x4C534C47, 0x6474732E, 0x3035342E, 0x00000000, 0x0003000E, 0x00000000, 0x00000001,
		0x0007000F, 0x00000004, 0x00000004, 0x6E69616D, 0x00000000, 0x00000009, 0x0000000C, 0x00030010,
		0x00000004, 0x00000007, 0x00030003, 0x00000002, 0x000001C2, 0x000A0004, 0x475F4C47, 0x4C474F4F,
		0x70635F45, 0x74735F70, 0x5F656C79, 0x656E696C, 0x7269645F, 0x69746365, 0x00006576, 0x00080004,
		0x475F4C47, 0x4C474F4F, 0x6E695F45, 0x64756C63, 0x69645F65, 0x74636572, 0x00657669, 0x00040005,
		0x00000004, 0x6E69616D, 0x00000000, 0x00050005, 0x00000009, 0x4374756F, 0x726F6C6F, 0x00000000,
		0x00050005, 0x0000000C, 0x67617266, 0x6F6C6F43, 0x00000072, 0x00040047, 0x00000009, 0x0000001E,
		0x00000000, 0x00040047, 0x0000000C, 0x0000001E, 0x00000000, 0x00020013, 0x00000002, 0x00030021,
		0x00000003, 0x00000002, 0x00030016, 0x00000006, 0x00000020, 0x00040017, 0x00000007, 0x00000006,
		0x00000004, 0x00040020, 0x00000008, 0x00000003, 0x00000007, 0x0004003B, 0x00000008, 0x00000009,
		0x00000003, 0x00040017, 0x0000000A, 0x00000006, 0x00000003, 0x00040020, 0x0000000B, 0x00000001,
		0x0000000A, 0x0004003B, 0x0000000B, 0x0000000C, 0x00000001, 0x0004002B, 0x00000006, 0x0000000E,
		0x3F800000, 0x00050036, 0x00000002, 0x00000004, 0x00000000, 0x00000003, 0x000200F8, 0x00000005,
		0x0004003D, 0x0000000A, 0x0000000D, 0x0000000C, 0x00050051, 0x00000006, 0x0000000F, 0x0000000D,
		0x00000000, 0x00050051, 0x00000006, 0x00000010, 0x0000000D, 0x00000001, 0x00050051, 0x00000006,
		0x00000011, 0x0000000D, 0x00000002, 0x00070050, 0x00000007, 0x00000012, 0x0000000F, 0x00000010,
		0x00000011, 0x0000000E, 0x0003003E, 0x00000009, 0x00000012, 0x000100FD, 0x00010038
	};

	VkShaderModule CreateShader(VkDevice device, const uint32_t* code, size_t wordCount)
	{
		VkShaderModuleCreateInfo info {};
		info.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
		info.codeSize = wordCount * sizeof(uint32_t);
		info.pCode = code;

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

		VkShaderModule vertex = CreateShader(device, VertexShader, sizeof(VertexShader) / sizeof(uint32_t));
		VkShaderModule fragment = CreateShader(device, FragmentShader, sizeof(FragmentShader) / sizeof(uint32_t));
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

		VkPipelineLayoutCreateInfo layoutInfo {};
		layoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
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
