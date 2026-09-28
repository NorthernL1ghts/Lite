#pragma once

#include <Lite/Input/Input.h>
#include <Lite/Scene/Scene.h>

#include <algorithm>
#include <cmath>

namespace Lite {

	inline constexpr float kCameraTurnSpeed = 1.6f;
	inline constexpr float kCameraZoomScale = 0.85f;
	inline constexpr float kCameraSizeMin = 0.25f;
	inline constexpr float kCameraSizeMax = 12.0f;
	inline constexpr float kDefaultAspect = 16.0f / 9.0f;

	inline float AspectRatio(float width, float height)
	{
		return height > 0.0f ? width / height : kDefaultAspect;
	}

	inline void TurnCamera(float& radians, float seconds)
	{
		float step = kCameraTurnSpeed * seconds;
		if (Input::IsKeyPressed(Key::Q))
			radians += step;
		if (Input::IsKeyPressed(Key::E))
			radians -= step;
	}

	inline void ZoomCamera(float& size, float steps)
	{
		if (steps == 0.0f)
			return;

		size *= std::pow(kCameraZoomScale, steps);
		size = std::clamp(size, kCameraSizeMin, kCameraSizeMax);
	}

}
