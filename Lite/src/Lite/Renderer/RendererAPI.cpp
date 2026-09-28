#include "RendererAPI.h"

#include "Renderer.h"
#include "ShaderProgram.h"
#include "VertexArray.h"

namespace Lite {

	void RendererAPI::DrawIndexed(const VertexArray& vertexArray)
	{
		Renderer::GetShader().Bind();
		vertexArray.Bind();
		vkCmdDrawIndexed(Renderer::GetCommandBuffer(), vertexArray.GetIndexCount(), 1, 0, 0, 0);
	}

}
