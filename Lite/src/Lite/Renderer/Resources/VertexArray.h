#pragma once

#include <Lite/Core/Base.h>
#include <Lite/Renderer/Resources/Buffer.h>

#include <cstdint>

namespace Lite {

	class LITE_API VertexArray
	{
	public:
		bool Create(const void* vertices, uint32_t vertexSize, const uint16_t* indices, uint32_t indexCount);
		bool Upload(const void* vertices, uint32_t vertexSize, const uint16_t* indices, uint32_t indexCount);
		void Destroy();
		void Bind() const;

		uint32_t GetIndexCount() const { return m_IndexBuffer.GetCount(); }

	private:
		VertexBuffer m_VertexBuffer;
		IndexBuffer m_IndexBuffer;
	};

}
