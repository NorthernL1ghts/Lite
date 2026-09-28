#pragma once

#include "Lite/Core/Base.h"
#include "Vulkan/VulkanBuffer.h"

#include <cstdint>

namespace Lite {

	class LITE_API VertexBuffer
	{
	public:
		bool Create(const void* data, uint32_t size);
		bool Upload(const void* data, uint32_t size);
		void Destroy();
		void Bind() const;

	private:
		VulkanBuffer m_Buffer;
	};

	class LITE_API IndexBuffer
	{
	public:
		bool Create(const uint16_t* indices, uint32_t count);
		bool Upload(const uint16_t* indices, uint32_t count);
		void Destroy();
		void Bind() const;

		uint32_t GetCount() const { return m_Count; }

	private:
		VulkanBuffer m_Buffer;
		uint32_t m_Count = 0;
	};

}
