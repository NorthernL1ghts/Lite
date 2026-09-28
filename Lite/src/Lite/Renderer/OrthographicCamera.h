#pragma once

#include "Lite/Math/Math.h"

namespace Lite {

	class OrthographicCamera
	{
	public:
		void SetProjection(float size, float aspect, float zNear = -1.0f, float zFar = 1.0f)
		{
			float halfHeight = size * 0.5f;
			float halfWidth = halfHeight * aspect;
			SetProjection(-halfWidth, halfWidth, -halfHeight, halfHeight, zNear, zFar);
		}

		void SetProjection(float left, float right, float bottom, float top, float zNear = -1.0f, float zFar = 1.0f)
		{
			m_Projection = Mat4::Orthographic(left, right, bottom, top, zNear, zFar);
			Recalculate();
		}

		void SetPosition(const Vec3& position)
		{
			m_Position = position;
			Recalculate();
		}

		void SetRotation(float radians)
		{
			m_Rotation = radians;
			Recalculate();
		}

		const Vec3& GetPosition() const { return m_Position; }
		float GetRotation() const { return m_Rotation; }
		const Mat4& GetProjection() const { return m_Projection; }
		const Mat4& GetView() const { return m_View; }
		const Mat4& GetViewProjection() const { return m_ViewProjection; }

	private:
		void Recalculate()
		{
			m_View = Mat4::Rotate(-m_Rotation, { 0.0f, 0.0f, 1.0f }) * Mat4::Translate(-m_Position);
			m_ViewProjection = m_Projection * m_View;
		}

		Vec3 m_Position {};
		float m_Rotation = 0.0f;
		Mat4 m_Projection = Mat4::Identity();
		Mat4 m_View = Mat4::Identity();
		Mat4 m_ViewProjection = Mat4::Identity();
	};

}
