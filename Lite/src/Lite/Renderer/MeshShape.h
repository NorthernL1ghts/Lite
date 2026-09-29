#pragma once

#include <Lite/Math/Vector.h>

#include <span>

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

	inline bool PointInPolygon(Vec2 point, std::span<const Vec2> vertices)
	{
		if (vertices.size() < 3)
			return false;

		bool positive = false;
		bool negative = false;
		for (size_t index = 0; index < vertices.size(); ++index)
		{
			const Vec2& current = vertices[index];
			const Vec2& next = vertices[(index + 1) % vertices.size()];
			float cross = (next.x - current.x) * (point.y - current.y) - (next.y - current.y) * (point.x - current.x);
			if (cross > 0.0f)
				positive = true;
			if (cross < 0.0f)
				negative = true;
			if (positive && negative)
				return false;
		}

		return true;
	}

}
