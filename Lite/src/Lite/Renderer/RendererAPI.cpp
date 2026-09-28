#include <Lite/Renderer/RendererAPI.h>

#include <Lite/Renderer/Renderer.h>
#include <Lite/Renderer/Resources/VertexArray.h>

namespace Lite {

	void RendererAPI::DrawIndexed(const VertexArray& vertexArray)
	{
		vertexArray.Bind();
		Renderer::RecordDraw(vertexArray.GetIndexCount());
		vkCmdDrawIndexed(Renderer::GetCommandBuffer(), vertexArray.GetIndexCount(), 1, 0, 0, 0);
	}

}
