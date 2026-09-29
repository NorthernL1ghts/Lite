#pragma once

#include <Lite/Math/Math.h>
#include <Lite/Scene/Components/Component.h>

namespace Lite {

	inline constexpr float kDefaultFieldOfView = 60.0f * (3.14159265f / 180.0f);

	enum class CameraProjection
	{
		Orthographic,
		Perspective
	};

	struct CameraComponent
	{
		CameraProjection Projection = CameraProjection::Orthographic;
		float Size = 2.0f;
		float FieldOfView = kDefaultFieldOfView;
		float Near = -1.0f;
		float Far = 1.0f;
		bool Primary = false;
	};

	template<>
	struct ComponentTraits<CameraComponent>
	{
		static constexpr ComponentId Id = ComponentId::Camera;
	};

	inline Mat4 CameraProjectionMatrix(const CameraComponent& camera, float aspect)
	{
		if (camera.Projection == CameraProjection::Perspective)
		{
			float zNear = camera.Near > 0.0f ? camera.Near : 0.1f;
			float zFar = camera.Far > zNear ? camera.Far : zNear + 100.0f;
			float fov = camera.FieldOfView > 0.0f ? camera.FieldOfView : kDefaultFieldOfView;
			return Mat4::Perspective(fov, aspect, zNear, zFar);
		}

		float halfHeight = camera.Size * 0.5f;
		float halfWidth = halfHeight * aspect;
		return Mat4::Orthographic(-halfWidth, halfWidth, -halfHeight, halfHeight, camera.Near, camera.Far);
	}

}
