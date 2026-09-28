#pragma once

#include "Quaternion.h"

#include <cmath>

namespace Lite {

	struct Transform
	{
		Vec3 Position {};
		Quat Rotation = Quat::Identity();
		Vec3 Scale { 1.0f, 1.0f, 1.0f };

		void SetRotation(const Vec3& eulerRadians)
		{
			Quat x = Quat::FromAxisAngle({ 1.0f, 0.0f, 0.0f }, eulerRadians.x);
			Quat y = Quat::FromAxisAngle({ 0.0f, 1.0f, 0.0f }, eulerRadians.y);
			Quat z = Quat::FromAxisAngle({ 0.0f, 0.0f, 1.0f }, eulerRadians.z);
			Rotation = z * y * x;
		}

		void SetRotationZ(float radians)
		{
			Rotation = Quat::FromAxisAngle({ 0.0f, 0.0f, 1.0f }, radians);
		}

		float GetRotationZ() const
		{
			Quat rotation = Rotation.Normalized();
			return 2.0f * std::atan2(rotation.z, rotation.w);
		}

		void Translate(const Vec3& offset)
		{
			Position += offset;
		}

		void Rotate(const Vec3& axis, float radians)
		{
			Rotation = Quat::FromAxisAngle(axis, radians) * Rotation;
		}

		Mat4 GetTranslation() const
		{
			return Mat4::Translate(Position);
		}

		Mat4 GetRotation() const
		{
			return Rotation.ToMat4();
		}

		Mat4 GetScale() const
		{
			return Mat4::Scale(Scale);
		}

		Mat4 GetMatrix() const
		{
			return GetTranslation() * GetRotation() * GetScale();
		}

		Mat4 GetViewMatrix() const
		{
			Quat inverseRotation = Rotation.Normalized().Conjugate();
			return inverseRotation.ToMat4() * Mat4::Translate(-Position);
		}

		Vec3 TransformPoint(const Vec3& point) const
		{
			return (GetMatrix() * Vec4(point, 1.0f)).xyz();
		}

		Vec3 TransformDirection(const Vec3& direction) const
		{
			return Rotation.Rotate({ direction.x * Scale.x, direction.y * Scale.y, direction.z * Scale.z });
		}
	};

}
