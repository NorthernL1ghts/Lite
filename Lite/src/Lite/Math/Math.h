#pragma once

#include <Lite/Math/Vector.h>
#include <Lite/Math/Mat.h>
#include <Lite/Math/Quaternion.h>
#include <Lite/Math/Transform.h>

#include <numbers>

namespace Lite {

	inline constexpr float kPi = std::numbers::pi_v<float>;

	constexpr float Radians(float degrees)
	{
		return degrees * (kPi / 180.0f);
	}

	constexpr float Degrees(float radians)
	{
		return radians * (180.0f / kPi);
	}

}
