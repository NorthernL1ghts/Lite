#include <Lite/Renderer/Resources/Buffer.h>

#include <Lite/Renderer/Renderer.h>

namespace Lite {

	bool VertexBuffer::Create(const void* data, uint32_t size)
	{
		if (!m_Buffer.Create(Renderer::GetDevice(), Renderer::GetPhysicalDevice(), size, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT))
			return false;

		if (!m_Buffer.Upload(data, size))
		{
			m_Buffer.Destroy();
			return false;
		}

		return true;
	}

	bool VertexBuffer::Upload(const void* data, uint32_t size)
	{
		return m_Buffer.Upload(data, size);
	}

	void VertexBuffer::Destroy()
	{
		m_Buffer.Destroy();
	}

	void VertexBuffer::Bind() const
	{
		m_Buffer.BindVertex(Renderer::GetCommandBuffer());
	}

	bool IndexBuffer::Create(const uint16_t* indices, uint32_t count)
	{
		VkDeviceSize size = static_cast<VkDeviceSize>(count) * sizeof(uint16_t);
		if (!m_Buffer.Create(Renderer::GetDevice(), Renderer::GetPhysicalDevice(), size, VK_BUFFER_USAGE_INDEX_BUFFER_BIT))
			return false;

		if (!m_Buffer.Upload(indices, size))
		{
			m_Buffer.Destroy();
			return false;
		}

		m_Count = count;
		return true;
	}

	bool IndexBuffer::Upload(const uint16_t* indices, uint32_t count)
	{
		VkDeviceSize size = static_cast<VkDeviceSize>(count) * sizeof(uint16_t);
		if (!m_Buffer.Upload(indices, size))
			return false;

		m_Count = count;
		return true;
	}

	void IndexBuffer::Destroy()
	{
		m_Buffer.Destroy();
		m_Count = 0;
	}

	void IndexBuffer::Bind() const
	{
		m_Buffer.BindIndex(Renderer::GetCommandBuffer());
	}

}
