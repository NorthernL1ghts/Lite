#pragma once

#include <cstdint>

namespace Lite {

	enum class VertexFormat
	{
		Float2 = 0,
		Float3,
		Float4
	};

	struct VertexAttribute
	{
		uint32_t Location = 0;
		VertexFormat Format = VertexFormat::Float2;
		uint32_t Offset = 0;
	};

	struct VertexLayout
	{
		uint32_t Stride = 0;
		VertexAttribute Attributes[8] {};
		uint32_t Count = 0;
	};

}
