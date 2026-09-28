#pragma once

#include <cmath>

namespace Lite {

	struct Vec2
	{
		float x = 0.0f;
		float y = 0.0f;

		Vec2() = default;
		constexpr Vec2(float x, float y) : x(x), y(y) {}
		explicit constexpr Vec2(float scalar) : x(scalar), y(scalar) {}

		float& operator[](int index) { return (&x)[index]; }
		const float& operator[](int index) const { return (&x)[index]; }

		constexpr Vec2 operator+(const Vec2& other) const { return { x + other.x, y + other.y }; }
		constexpr Vec2 operator-(const Vec2& other) const { return { x - other.x, y - other.y }; }
		constexpr Vec2 operator*(const Vec2& other) const { return { x * other.x, y * other.y }; }
		constexpr Vec2 operator/(const Vec2& other) const { return { x / other.x, y / other.y }; }
		constexpr Vec2 operator*(float scalar) const { return { x * scalar, y * scalar }; }
		constexpr Vec2 operator/(float scalar) const { return { x / scalar, y / scalar }; }
		constexpr Vec2 operator-() const { return { -x, -y }; }

		Vec2& operator+=(const Vec2& other) { x += other.x; y += other.y; return *this; }
		Vec2& operator-=(const Vec2& other) { x -= other.x; y -= other.y; return *this; }
		Vec2& operator*=(float scalar) { x *= scalar; y *= scalar; return *this; }
		Vec2& operator/=(float scalar) { x /= scalar; y /= scalar; return *this; }

		constexpr bool operator==(const Vec2& other) const { return x == other.x && y == other.y; }
		constexpr bool operator!=(const Vec2& other) const { return !(*this == other); }

		constexpr float LengthSquared() const { return x * x + y * y; }
		float Length() const { return std::sqrt(LengthSquared()); }

		Vec2 Normalized() const
		{
			float length = Length();
			return length > 0.0f ? *this / length : Vec2{};
		}

		constexpr float Dot(const Vec2& other) const { return x * other.x + y * other.y; }
		constexpr float Cross(const Vec2& other) const { return x * other.y - y * other.x; }
	};

	constexpr Vec2 operator*(float scalar, const Vec2& value) { return value * scalar; }

	struct Vec3
	{
		float x = 0.0f;
		float y = 0.0f;
		float z = 0.0f;

		Vec3() = default;
		constexpr Vec3(float x, float y, float z) : x(x), y(y), z(z) {}
		constexpr Vec3(const Vec2& xy, float z) : x(xy.x), y(xy.y), z(z) {}
		explicit constexpr Vec3(float scalar) : x(scalar), y(scalar), z(scalar) {}

		float& operator[](int index) { return (&x)[index]; }
		const float& operator[](int index) const { return (&x)[index]; }

		constexpr Vec2 xy() const { return { x, y }; }

		constexpr Vec3 operator+(const Vec3& other) const { return { x + other.x, y + other.y, z + other.z }; }
		constexpr Vec3 operator-(const Vec3& other) const { return { x - other.x, y - other.y, z - other.z }; }
		constexpr Vec3 operator*(const Vec3& other) const { return { x * other.x, y * other.y, z * other.z }; }
		constexpr Vec3 operator/(const Vec3& other) const { return { x / other.x, y / other.y, z / other.z }; }
		constexpr Vec3 operator*(float scalar) const { return { x * scalar, y * scalar, z * scalar }; }
		constexpr Vec3 operator/(float scalar) const { return { x / scalar, y / scalar, z / scalar }; }
		constexpr Vec3 operator-() const { return { -x, -y, -z }; }

		Vec3& operator+=(const Vec3& other) { x += other.x; y += other.y; z += other.z; return *this; }
		Vec3& operator-=(const Vec3& other) { x -= other.x; y -= other.y; z -= other.z; return *this; }
		Vec3& operator*=(float scalar) { x *= scalar; y *= scalar; z *= scalar; return *this; }
		Vec3& operator/=(float scalar) { x /= scalar; y /= scalar; z /= scalar; return *this; }

		constexpr bool operator==(const Vec3& other) const { return x == other.x && y == other.y && z == other.z; }
		constexpr bool operator!=(const Vec3& other) const { return !(*this == other); }

		constexpr float LengthSquared() const { return x * x + y * y + z * z; }
		float Length() const { return std::sqrt(LengthSquared()); }

		Vec3 Normalized() const
		{
			float length = Length();
			return length > 0.0f ? *this / length : Vec3{};
		}

		constexpr float Dot(const Vec3& other) const { return x * other.x + y * other.y + z * other.z; }

		constexpr Vec3 Cross(const Vec3& other) const
		{
			return {
				y * other.z - z * other.y,
				z * other.x - x * other.z,
				x * other.y - y * other.x
			};
		}
	};

	constexpr Vec3 operator*(float scalar, const Vec3& value) { return value * scalar; }

	struct Vec4
	{
		float x = 0.0f;
		float y = 0.0f;
		float z = 0.0f;
		float w = 0.0f;

		Vec4() = default;
		constexpr Vec4(float x, float y, float z, float w) : x(x), y(y), z(z), w(w) {}
		constexpr Vec4(const Vec3& xyz, float w) : x(xyz.x), y(xyz.y), z(xyz.z), w(w) {}
		constexpr Vec4(const Vec2& xy, float z, float w) : x(xy.x), y(xy.y), z(z), w(w) {}
		explicit constexpr Vec4(float scalar) : x(scalar), y(scalar), z(scalar), w(scalar) {}

		float& operator[](int index) { return (&x)[index]; }
		const float& operator[](int index) const { return (&x)[index]; }

		constexpr Vec2 xy() const { return { x, y }; }
		constexpr Vec3 xyz() const { return { x, y, z }; }

		constexpr Vec4 operator+(const Vec4& other) const { return { x + other.x, y + other.y, z + other.z, w + other.w }; }
		constexpr Vec4 operator-(const Vec4& other) const { return { x - other.x, y - other.y, z - other.z, w - other.w }; }
		constexpr Vec4 operator*(const Vec4& other) const { return { x * other.x, y * other.y, z * other.z, w * other.w }; }
		constexpr Vec4 operator/(const Vec4& other) const { return { x / other.x, y / other.y, z / other.z, w / other.w }; }
		constexpr Vec4 operator*(float scalar) const { return { x * scalar, y * scalar, z * scalar, w * scalar }; }
		constexpr Vec4 operator/(float scalar) const { return { x / scalar, y / scalar, z / scalar, w / scalar }; }
		constexpr Vec4 operator-() const { return { -x, -y, -z, -w }; }

		Vec4& operator+=(const Vec4& other) { x += other.x; y += other.y; z += other.z; w += other.w; return *this; }
		Vec4& operator-=(const Vec4& other) { x -= other.x; y -= other.y; z -= other.z; w -= other.w; return *this; }
		Vec4& operator*=(float scalar) { x *= scalar; y *= scalar; z *= scalar; w *= scalar; return *this; }
		Vec4& operator/=(float scalar) { x /= scalar; y /= scalar; z /= scalar; w /= scalar; return *this; }

		constexpr bool operator==(const Vec4& other) const { return x == other.x && y == other.y && z == other.z && w == other.w; }
		constexpr bool operator!=(const Vec4& other) const { return !(*this == other); }

		constexpr float LengthSquared() const { return x * x + y * y + z * z + w * w; }
		float Length() const { return std::sqrt(LengthSquared()); }

		Vec4 Normalized() const
		{
			float length = Length();
			return length > 0.0f ? *this / length : Vec4{};
		}

		constexpr float Dot(const Vec4& other) const { return x * other.x + y * other.y + z * other.z + w * other.w; }
	};

	constexpr Vec4 operator*(float scalar, const Vec4& value) { return value * scalar; }

	inline Vec2 Lerp(const Vec2& a, const Vec2& b, float t) { return a + (b - a) * t; }
	inline Vec3 Lerp(const Vec3& a, const Vec3& b, float t) { return a + (b - a) * t; }
	inline Vec4 Lerp(const Vec4& a, const Vec4& b, float t) { return a + (b - a) * t; }

}
