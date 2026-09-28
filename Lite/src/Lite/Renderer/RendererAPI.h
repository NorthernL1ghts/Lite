#pragma once

#include <Lite/Core/Base.h>

namespace Lite {

	class VertexArray;

	class LITE_API RendererAPI
	{
	public:
		static void DrawIndexed(const VertexArray& vertexArray);
	};

}
