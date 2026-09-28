#include "RendererAPI.h"

#include "Renderer.h"
#include "VertexArray.h"

namespace Lite {

	void RendererAPI::DrawIndexed(const VertexArray& vertexArray)
	{
		VkCommandBuffer commandBuffer = Renderer::GetCommandBuffer();
		Renderer::GetPipeline().Bind(commandBuffer, Renderer::GetExtent());
		vertexArray.Bind();
		vkCmdDrawIndexed(commandBuffer, vertexArray.GetIndexCount(), 1, 0, 0, 0);
	}

}
