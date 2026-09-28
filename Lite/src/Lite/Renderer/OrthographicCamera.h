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
			m_Transform.Position = position;
			Recalculate();
		}

		void SetRotation(float radians)
		{
			m_Transform.SetRotationZ(radians);
			Recalculate();
		}

		void SetTransform(const Transform& transform)
		{
			m_Transform = transform;
			Recalculate();
		}

		const Vec3& GetPosition() const { return m_Transform.Position; }
		float GetRotation() const { return m_Transform.GetRotationZ(); }
		const Transform& GetTransform() const { return m_Transform; }
		const Mat4& GetProjection() const { return m_Projection; }
		const Mat4& GetView() const { return m_View; }
		const Mat4& GetViewProjection() const { return m_ViewProjection; }

	private:
		void Recalculate()
		{
			m_View = m_Transform.GetViewMatrix();
			m_ViewProjection = m_Projection * m_View;
		}

		Transform m_Transform {};
		Mat4 m_Projection = Mat4::Identity();
		Mat4 m_View = Mat4::Identity();
		Mat4 m_ViewProjection = Mat4::Identity();
	};

}
