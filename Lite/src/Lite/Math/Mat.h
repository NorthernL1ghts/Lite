#pragma once

#include <Lite/Math/Vector.h>

#include <algorithm>
#include <cmath>

namespace Lite {

	struct Mat2
	{
		Vec2 columns[2] { { 1.0f, 0.0f }, { 0.0f, 1.0f } };

		Mat2() = default;
		constexpr Mat2(const Vec2& c0, const Vec2& c1) : columns{ c0, c1 } {}

		Vec2& operator[](int column) { return columns[column]; }
		const Vec2& operator[](int column) const { return columns[column]; }

		const float* Data() const { return &columns[0].x; }

		static constexpr Mat2 Identity() { return { { 1.0f, 0.0f }, { 0.0f, 1.0f } }; }

		Mat2 operator*(const Mat2& other) const
		{
			Mat2 result;
			for (int column = 0; column < 2; ++column)
			{
				for (int row = 0; row < 2; ++row)
				{
					result[column][row] =
						columns[0][row] * other[column][0] +
						columns[1][row] * other[column][1];
				}
			}
			return result;
		}

		Vec2 operator*(const Vec2& vector) const
		{
			return columns[0] * vector.x + columns[1] * vector.y;
		}

		Mat2 Transposed() const
		{
			return { { columns[0].x, columns[1].x }, { columns[0].y, columns[1].y } };
		}
	};

	struct Mat3
	{
		Vec3 columns[3] { { 1.0f, 0.0f, 0.0f }, { 0.0f, 1.0f, 0.0f }, { 0.0f, 0.0f, 1.0f } };

		Mat3() = default;
		constexpr Mat3(const Vec3& c0, const Vec3& c1, const Vec3& c2) : columns{ c0, c1, c2 } {}

		Vec3& operator[](int column) { return columns[column]; }
		const Vec3& operator[](int column) const { return columns[column]; }

		const float* Data() const { return &columns[0].x; }

		static constexpr Mat3 Identity()
		{
			return { { 1.0f, 0.0f, 0.0f }, { 0.0f, 1.0f, 0.0f }, { 0.0f, 0.0f, 1.0f } };
		}

		Mat3 operator*(const Mat3& other) const
		{
			Mat3 result;
			for (int column = 0; column < 3; ++column)
			{
				for (int row = 0; row < 3; ++row)
				{
					result[column][row] =
						columns[0][row] * other[column][0] +
						columns[1][row] * other[column][1] +
						columns[2][row] * other[column][2];
				}
			}
			return result;
		}

		Vec3 operator*(const Vec3& vector) const
		{
			return columns[0] * vector.x + columns[1] * vector.y + columns[2] * vector.z;
		}

		Mat3 Transposed() const
		{
			return {
				{ columns[0].x, columns[1].x, columns[2].x },
				{ columns[0].y, columns[1].y, columns[2].y },
				{ columns[0].z, columns[1].z, columns[2].z }
			};
		}
	};

	struct Mat4
	{
		Vec4 columns[4] {
			{ 1.0f, 0.0f, 0.0f, 0.0f },
			{ 0.0f, 1.0f, 0.0f, 0.0f },
			{ 0.0f, 0.0f, 1.0f, 0.0f },
			{ 0.0f, 0.0f, 0.0f, 1.0f }
		};

		Mat4() = default;
		constexpr Mat4(const Vec4& c0, const Vec4& c1, const Vec4& c2, const Vec4& c3)
			: columns{ c0, c1, c2, c3 }
		{
		}

		Vec4& operator[](int column) { return columns[column]; }
		const Vec4& operator[](int column) const { return columns[column]; }

		const float* Data() const { return &columns[0].x; }

		static constexpr Mat4 Identity()
		{
			return {
				{ 1.0f, 0.0f, 0.0f, 0.0f },
				{ 0.0f, 1.0f, 0.0f, 0.0f },
				{ 0.0f, 0.0f, 1.0f, 0.0f },
				{ 0.0f, 0.0f, 0.0f, 1.0f }
			};
		}

		Mat4 operator*(const Mat4& other) const
		{
			Mat4 result;
			for (int column = 0; column < 4; ++column)
			{
				for (int row = 0; row < 4; ++row)
				{
					result[column][row] =
						columns[0][row] * other[column][0] +
						columns[1][row] * other[column][1] +
						columns[2][row] * other[column][2] +
						columns[3][row] * other[column][3];
				}
			}
			return result;
		}

		Vec4 operator*(const Vec4& vector) const
		{
			return columns[0] * vector.x + columns[1] * vector.y + columns[2] * vector.z + columns[3] * vector.w;
		}

		Mat4 Transposed() const
		{
			return {
				{ columns[0].x, columns[1].x, columns[2].x, columns[3].x },
				{ columns[0].y, columns[1].y, columns[2].y, columns[3].y },
				{ columns[0].z, columns[1].z, columns[2].z, columns[3].z },
				{ columns[0].w, columns[1].w, columns[2].w, columns[3].w }
			};
		}

		Mat4 Inverse() const
		{
			Mat4 copy = *this;
			Mat4 result = Identity();

			for (int column = 0; column < 4; ++column)
			{
				int pivot = column;
				float maxAbs = std::fabs(copy[column][column]);
				for (int row = column + 1; row < 4; ++row)
				{
					float value = std::fabs(copy[column][row]);
					if (value > maxAbs)
					{
						maxAbs = value;
						pivot = row;
					}
				}

				if (maxAbs < 1.0e-8f)
					return Identity();

				if (pivot != column)
				{
					for (int c = 0; c < 4; ++c)
					{
						std::swap(copy[c][column], copy[c][pivot]);
						std::swap(result[c][column], result[c][pivot]);
					}
				}

				float invPivot = 1.0f / copy[column][column];
				for (int c = 0; c < 4; ++c)
				{
					copy[c][column] *= invPivot;
					result[c][column] *= invPivot;
				}

				for (int row = 0; row < 4; ++row)
				{
					if (row == column)
						continue;

					float factor = copy[column][row];
					for (int c = 0; c < 4; ++c)
					{
						copy[c][row] -= copy[c][column] * factor;
						result[c][row] -= result[c][column] * factor;
					}
				}
			}

			return result;
		}

		static Mat4 Translate(const Vec3& offset)
		{
			Mat4 result = Identity();
			result[3] = { offset.x, offset.y, offset.z, 1.0f };
			return result;
		}

		static Mat4 Scale(const Vec3& scale)
		{
			Mat4 result;
			result[0][0] = scale.x;
			result[1][1] = scale.y;
			result[2][2] = scale.z;
			result[3][3] = 1.0f;
			return result;
		}

		static Mat4 Rotate(float radians, const Vec3& axis)
		{
			Vec3 n = axis.Normalized();
			float c = std::cos(radians);
			float s = std::sin(radians);
			float oneMinusC = 1.0f - c;

			Mat4 result = Identity();
			result[0] = {
				c + n.x * n.x * oneMinusC,
				n.y * n.x * oneMinusC + n.z * s,
				n.z * n.x * oneMinusC - n.y * s,
				0.0f
			};
			result[1] = {
				n.x * n.y * oneMinusC - n.z * s,
				c + n.y * n.y * oneMinusC,
				n.z * n.y * oneMinusC + n.x * s,
				0.0f
			};
			result[2] = {
				n.x * n.z * oneMinusC + n.y * s,
				n.y * n.z * oneMinusC - n.x * s,
				c + n.z * n.z * oneMinusC,
				0.0f
			};
			return result;
		}

		static Mat4 Orthographic(float left, float right, float bottom, float top, float zNear, float zFar)
		{
			Mat4 result;
			result[0] = {};
			result[1] = {};
			result[2] = {};
			result[3] = {};
			result[0][0] = 2.0f / (right - left);
			result[1][1] = 2.0f / (top - bottom);
			result[2][2] = -1.0f / (zFar - zNear);
			result[3][0] = -(right + left) / (right - left);
			result[3][1] = -(top + bottom) / (top - bottom);
			result[3][2] = -zNear / (zFar - zNear);
			result[3][3] = 1.0f;
			return result;
		}

		static Mat4 Perspective(float fovYRadians, float aspect, float zNear, float zFar)
		{
			float tanHalf = std::tan(fovYRadians * 0.5f);
			Mat4 result;
			result[0] = {};
			result[1] = {};
			result[2] = {};
			result[3] = {};
			result[0][0] = 1.0f / (aspect * tanHalf);
			result[1][1] = 1.0f / tanHalf;
			result[2][2] = zFar / (zNear - zFar);
			result[2][3] = -1.0f;
			result[3][2] = -(zFar * zNear) / (zFar - zNear);
			return result;
		}

		static Mat4 LookAt(const Vec3& eye, const Vec3& center, const Vec3& up)
		{
			Vec3 forward = (center - eye).Normalized();
			Vec3 side = forward.Cross(up).Normalized();
			Vec3 newUp = side.Cross(forward);

			Mat4 result = Identity();
			result[0][0] = side.x;
			result[1][0] = side.y;
			result[2][0] = side.z;
			result[0][1] = newUp.x;
			result[1][1] = newUp.y;
			result[2][1] = newUp.z;
			result[0][2] = -forward.x;
			result[1][2] = -forward.y;
			result[2][2] = -forward.z;
			result[3][0] = -side.Dot(eye);
			result[3][1] = -newUp.Dot(eye);
			result[3][2] = forward.Dot(eye);
			return result;
		}
	};

}
