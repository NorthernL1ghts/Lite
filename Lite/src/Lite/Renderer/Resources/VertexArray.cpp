#include <Lite/Renderer/Resources/VertexArray.h>

namespace Lite {

	bool VertexArray::Create(const void* vertices, uint32_t vertexSize, const uint16_t* indices, uint32_t indexCount)
	{
		Destroy();

		if (!m_VertexBuffer.Create(vertices, vertexSize))
			return false;

		if (!m_IndexBuffer.Create(indices, indexCount))
		{
			m_VertexBuffer.Destroy();
			return false;
		}

		return true;
	}

	bool VertexArray::Upload(const void* vertices, uint32_t vertexSize, const uint16_t* indices, uint32_t indexCount)
	{
		return m_VertexBuffer.Upload(vertices, vertexSize) && m_IndexBuffer.Upload(indices, indexCount);
	}

	void VertexArray::Destroy()
	{
		m_IndexBuffer.Destroy();
		m_VertexBuffer.Destroy();
	}

	void VertexArray::Bind() const
	{
		m_VertexBuffer.Bind();
		m_IndexBuffer.Bind();
	}

}
