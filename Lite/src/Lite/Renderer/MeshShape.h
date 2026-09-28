#pragma once

#include <Lite/Math/Vector.h>

namespace Lite {

	inline constexpr Vec2 kQuadCorners[] = {
		{ -0.5f, -0.5f },
		{  0.5f, -0.5f },
		{  0.5f,  0.5f },
		{ -0.5f,  0.5f }
	};

	inline constexpr Vec2 kTriangleCorners[] = {
		{  0.00f, -0.72f },
		{ -0.78f,  0.58f },
		{  0.78f,  0.58f }
	};

}
