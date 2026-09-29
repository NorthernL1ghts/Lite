#pragma once

#include <Lite/Input/Input.h>
#include <Lite/Scene/Scene.h>

#include <algorithm>
#include <cmath>
#include <optional>

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

	inline Mat4 FitViewport(const Mat4& viewProjection, float viewportX, float viewportY, float viewportW, float viewportH, float windowW, float windowH)
	{
		if (windowW <= 1.0f || windowH <= 1.0f || viewportW <= 1.0f || viewportH <= 1.0f)
			return viewProjection;

		float left = (viewportX / windowW) * 2.0f - 1.0f;
		float right = ((viewportX + viewportW) / windowW) * 2.0f - 1.0f;
		float top = 1.0f - (viewportY / windowH) * 2.0f;
		float bottom = 1.0f - ((viewportY + viewportH) / windowH) * 2.0f;

		Mat4 fit = Mat4::Identity();
		fit[0][0] = (right - left) * 0.5f;
		fit[1][1] = (top - bottom) * 0.5f;
		fit[3][0] = (right + left) * 0.5f;
		fit[3][1] = (top + bottom) * 0.5f;
		return fit * viewProjection;
	}

	inline std::optional<Vec2> ScreenToWorld(const Mat4& viewProjection, float windowW, float windowH, float x, float y)
	{
		if (windowW <= 1.0f || windowH <= 1.0f)
			return std::nullopt;

		float ndcX = (x / windowW) * 2.0f - 1.0f;
		float ndcY = 1.0f - (y / windowH) * 2.0f;
		Vec4 point = viewProjection.Inverse() * Vec4(ndcX, ndcY, 0.0f, 1.0f);
		if (point.w != 0.0f)
			point /= point.w;
		return Vec2 { point.x, point.y };
	}

	inline std::optional<Vec2> WorldToScreen(const Mat4& viewProjection, float windowW, float windowH, Vec2 world)
	{
		if (windowW <= 1.0f || windowH <= 1.0f)
			return std::nullopt;

		Vec4 clip = viewProjection * Vec4(world.x, world.y, 0.0f, 1.0f);
		if (clip.w == 0.0f)
			return std::nullopt;

		clip /= clip.w;
		return Vec2 { (clip.x * 0.5f + 0.5f) * windowW, (1.0f - (clip.y * 0.5f + 0.5f)) * windowH };
	}

}
