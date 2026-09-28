#pragma once

#include <Lite/Math/Mat.h>

#include <cmath>

namespace Lite {

	struct Quat
	{
		float x = 0.0f;
		float y = 0.0f;
		float z = 0.0f;
		float w = 1.0f;

		Quat() = default;
		constexpr Quat(float x, float y, float z, float w) : x(x), y(y), z(z), w(w) {}

		static constexpr Quat Identity() { return { 0.0f, 0.0f, 0.0f, 1.0f }; }

		constexpr Quat operator+(const Quat& other) const { return { x + other.x, y + other.y, z + other.z, w + other.w }; }
		constexpr Quat operator-(const Quat& other) const { return { x - other.x, y - other.y, z - other.z, w - other.w }; }
		constexpr Quat operator*(float scalar) const { return { x * scalar, y * scalar, z * scalar, w * scalar }; }
		constexpr Quat operator-() const { return { -x, -y, -z, -w }; }

		constexpr Quat operator*(const Quat& other) const
		{
			return {
				w * other.x + x * other.w + y * other.z - z * other.y,
				w * other.y - x * other.z + y * other.w + z * other.x,
				w * other.z + x * other.y - y * other.x + z * other.w,
				w * other.w - x * other.x - y * other.y - z * other.z
			};
		}

		constexpr float Dot(const Quat& other) const { return x * other.x + y * other.y + z * other.z + w * other.w; }
		constexpr float LengthSquared() const { return Dot(*this); }
		float Length() const { return std::sqrt(LengthSquared()); }

		Quat Normalized() const
		{
			float length = Length();
			return length > 0.0f ? *this * (1.0f / length) : Identity();
		}

		constexpr Quat Conjugate() const { return { -x, -y, -z, w }; }

		Vec3 Rotate(const Vec3& vector) const
		{
			Quat q = Normalized();
			Quat point { vector.x, vector.y, vector.z, 0.0f };
			Quat rotated = q * point * q.Conjugate();
			return { rotated.x, rotated.y, rotated.z };
		}

		static Quat FromAxisAngle(const Vec3& axis, float radians)
		{
			Vec3 n = axis.Normalized();
			float half = radians * 0.5f;
			float s = std::sin(half);
			return { n.x * s, n.y * s, n.z * s, std::cos(half) };
		}

		Mat4 ToMat4() const
		{
			Quat q = Normalized();
			float xx = q.x * q.x;
			float yy = q.y * q.y;
			float zz = q.z * q.z;
			float xy = q.x * q.y;
			float xz = q.x * q.z;
			float yz = q.y * q.z;
			float wx = q.w * q.x;
			float wy = q.w * q.y;
			float wz = q.w * q.z;

			Mat4 result = Mat4::Identity();
			result[0] = { 1.0f - 2.0f * (yy + zz), 2.0f * (xy + wz), 2.0f * (xz - wy), 0.0f };
			result[1] = { 2.0f * (xy - wz), 1.0f - 2.0f * (xx + zz), 2.0f * (yz + wx), 0.0f };
			result[2] = { 2.0f * (xz + wy), 2.0f * (yz - wx), 1.0f - 2.0f * (xx + yy), 0.0f };
			return result;
		}

		static Quat Slerp(const Quat& a, const Quat& b, float t)
		{
			float cosTheta = a.Dot(b);
			Quat end = b;
			if (cosTheta < 0.0f)
			{
				end = -b;
				cosTheta = -cosTheta;
			}

			if (cosTheta > 0.9995f)
				return (a + (end - a) * t).Normalized();

			float theta = std::acos(cosTheta);
			float sinTheta = std::sin(theta);
			float weightA = std::sin((1.0f - t) * theta) / sinTheta;
			float weightB = std::sin(t * theta) / sinTheta;
			return a * weightA + end * weightB;
		}
	};

	struct DualQuat
	{
		Quat real = Quat::Identity();
		Quat dual { 0.0f, 0.0f, 0.0f, 0.0f };

		static constexpr DualQuat Identity() { return { Quat::Identity(), { 0.0f, 0.0f, 0.0f, 0.0f } }; }

		DualQuat operator*(const DualQuat& other) const
		{
			return {
				real * other.real,
				real * other.dual + dual * other.real
			};
		}

		DualQuat Normalized() const
		{
			float length = real.Length();
			if (length <= 0.0f)
				return Identity();

			float inv = 1.0f / length;
			return { real * inv, dual * inv };
		}
	};

}
