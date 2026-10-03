#pragma once

#include <Lite/Math/Quaternion.h>

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
			return Position + TransformDirection(point);
		}

		Vec3 TransformDirection(const Vec3& direction) const
		{
			return Rotation.Rotate({ direction.x * Scale.x, direction.y * Scale.y, direction.z * Scale.z });
		}

		Vec3 InverseTransformPoint(const Vec3& point) const
		{
			const Vec3 unrotated = Rotation.Normalized().Conjugate().Rotate(point - Position);
			auto axis = [](float value, float scale)
			{
				return std::fabs(scale) > 0.0001f ? value / scale : value;
			};
			return { axis(unrotated.x, Scale.x), axis(unrotated.y, Scale.y), axis(unrotated.z, Scale.z) };
		}
	};

	inline Transform CombineTransforms(const Transform& parent, const Transform& local)
	{
		Transform world;
		world.Position = parent.TransformPoint(local.Position);
		world.Rotation = (parent.Rotation * local.Rotation).Normalized();
		world.Scale = { parent.Scale.x * local.Scale.x, parent.Scale.y * local.Scale.y, parent.Scale.z * local.Scale.z };
		return world;
	}

}
