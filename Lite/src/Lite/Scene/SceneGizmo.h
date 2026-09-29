#pragma once

#include <Lite/Renderer/MeshShape.h>
#include <Lite/Scene/Components/MeshComponent.h>
#include <Lite/Scene/Scene.h>
#include <Lite/Scene/SceneCamera.h>

#include <algorithm>
#include <cmath>

namespace Lite {

	enum class GizmoAction
	{
		None,
		Move,
		Rotate,
		Scale
	};

	struct GizmoLayout
	{
		bool Ok = false;
		Vec2 CenterWorld {};
		Vec2 Center {};
		Vec2 World[4] {};
		Vec2 Screen[4] {};
		int Count = 0;
		float Ring = 36.0f;
	};

	struct GizmoDrag
	{
		GizmoAction Action = GizmoAction::None;
		int Corner = -1;
		float Angle = 0.0f;
		float Spin = 0.0f;
		Vec2 Grab {};
		Vec3 Position {};
		float Rotation = 0.0f;
		Vec3 Scale { 1.0f, 1.0f, 1.0f };
	};

	struct GizmoHot
	{
		GizmoAction Action = GizmoAction::None;
		int Corner = -1;
	};

	inline constexpr float kGizmoHandle = 7.0f;
	inline constexpr float kGizmoRingGap = 16.0f;
	inline constexpr float kGizmoRingHit = 7.0f;
	inline constexpr float kGizmoMinRing = 36.0f;

	inline const Vec2* MeshCorners(const MeshComponent* mesh, int& count)
	{
		if (mesh != nullptr && mesh->Type == MeshType::Triangle)
		{
			count = 3;
			return kTriangleCorners;
		}

		count = 4;
		return kQuadCorners;
	}

	inline bool BuildGizmo(const Mat4& viewProjection, float windowW, float windowH, Entity entity, GizmoLayout& layout)
	{
		TransformComponent* transform = entity.Get<TransformComponent>();
		if (transform == nullptr)
			return false;

		int count = 0;
		const Vec2* local = MeshCorners(entity.Get<MeshComponent>(), count);
		layout = {};
		layout.CenterWorld = { transform->Local.Position.x, transform->Local.Position.y };
		if (!WorldToScreen(viewProjection, windowW, windowH, layout.CenterWorld, layout.Center))
			return false;

		layout.Count = count;
		float reach = 0.0f;
		for (int index = 0; index < count; ++index)
		{
			Vec3 transformed = transform->Local.TransformPoint({ local[index].x, local[index].y, 0.0f });
			layout.World[index] = { transformed.x, transformed.y };
			if (!WorldToScreen(viewProjection, windowW, windowH, layout.World[index], layout.Screen[index]))
				return false;
			reach = std::max(reach, (layout.Center - layout.Screen[index]).Length());
		}

		layout.Ring = std::max(reach + kGizmoRingGap, kGizmoMinRing);
		layout.Ok = true;
		return true;
	}

	inline int HitGizmoScale(const GizmoLayout& layout, Vec2 mouse)
	{
		int hit = -1;
		float best = kGizmoHandle + 3.0f;
		for (int index = 0; index < layout.Count; ++index)
		{
			float distance = (mouse - layout.Screen[index]).Length();
			if (distance > best)
				continue;
			best = distance;
			hit = index;
		}

		return hit;
	}

	inline bool HitGizmoRing(const GizmoLayout& layout, Vec2 mouse)
	{
		float distance = (mouse - layout.Center).Length();
		return std::abs(distance - layout.Ring) <= kGizmoRingHit;
	}

	inline GizmoHot HitGizmo(const GizmoLayout& layout, Vec2 mouse, const Vec2* world)
	{
		GizmoHot hot;
		int corner = HitGizmoScale(layout, mouse);
		if (corner >= 0)
		{
			hot.Action = GizmoAction::Scale;
			hot.Corner = corner;
			return hot;
		}

		if (HitGizmoRing(layout, mouse))
		{
			hot.Action = GizmoAction::Rotate;
			return hot;
		}

		if (world != nullptr && PointInPolygon(*world, layout.World, layout.Count))
			hot.Action = GizmoAction::Move;
		return hot;
	}

	inline bool BeginGizmo(Scene& scene, const Mat4& viewProjection, float windowW, float windowH, uint32_t entityId, float mouseX, float mouseY, GizmoDrag& drag)
	{
		drag = {};
		Entity entity = scene.GetEntity(entityId);
		TransformComponent* transform = entity ? entity.Get<TransformComponent>() : nullptr;
		GizmoLayout layout;
		if (transform == nullptr || !BuildGizmo(viewProjection, windowW, windowH, entity, layout))
			return false;

		Vec2 world;
		if (!ScreenToWorld(viewProjection, windowW, windowH, mouseX, mouseY, world))
			return false;

		GizmoHot hot = HitGizmo(layout, { mouseX, mouseY }, &world);
		if (hot.Action == GizmoAction::None)
			return false;

		drag.Action = hot.Action;
		drag.Corner = hot.Corner;
		drag.Grab = world;
		drag.Position = transform->Local.Position;
		drag.Rotation = transform->Local.GetRotationZ();
		drag.Scale = transform->Local.Scale;
		drag.Angle = std::atan2(world.y - drag.Position.y, world.x - drag.Position.x);
		return true;
	}

	inline bool ApplyGizmo(Scene& scene, const Mat4& viewProjection, float windowW, float windowH, uint32_t entityId, float mouseX, float mouseY, GizmoDrag& drag)
	{
		Entity entity = scene.GetEntity(entityId);
		TransformComponent* transform = entity ? entity.Get<TransformComponent>() : nullptr;
		Vec2 world;
		if (transform == nullptr || drag.Action == GizmoAction::None || !ScreenToWorld(viewProjection, windowW, windowH, mouseX, mouseY, world))
		{
			drag.Action = GizmoAction::None;
			return false;
		}

		switch (drag.Action)
		{
			case GizmoAction::Move:
				transform->Local.Position.x = drag.Position.x + (world.x - drag.Grab.x);
				transform->Local.Position.y = drag.Position.y + (world.y - drag.Grab.y);
				break;
			case GizmoAction::Rotate:
			{
				constexpr float pi = 3.14159265f;
				float angle = std::atan2(world.y - drag.Position.y, world.x - drag.Position.x);
				float delta = angle - drag.Angle;
				while (delta > pi)
					delta -= pi * 2.0f;
				while (delta < -pi)
					delta += pi * 2.0f;
				drag.Spin += delta;
				drag.Angle = angle;
				transform->Local.SetRotationZ(drag.Rotation + drag.Spin);
				break;
			}
			case GizmoAction::Scale:
			{
				int count = 0;
				const Vec2* local = MeshCorners(entity.Get<MeshComponent>(), count);
				if (drag.Corner < 0 || drag.Corner >= count)
					break;

				Transform basis;
				basis.Position = drag.Position;
				basis.SetRotationZ(drag.Rotation);
				basis.Scale = { 1.0f, 1.0f, 1.0f };
				Vec3 offset { world.x - basis.Position.x, world.y - basis.Position.y, 0.0f };
				Vec3 localPoint = basis.Rotation.Normalized().Conjugate().Rotate(offset);
				Vec2 corner = local[drag.Corner];
				Vec3 scale = drag.Scale;
				if (std::abs(corner.x) > 0.001f)
					scale.x = localPoint.x / corner.x;
				if (std::abs(corner.y) > 0.001f)
					scale.y = localPoint.y / corner.y;
				if (std::abs(scale.x) < 0.02f)
					scale.x = scale.x < 0.0f ? -0.02f : 0.02f;
				if (std::abs(scale.y) < 0.02f)
					scale.y = scale.y < 0.0f ? -0.02f : 0.02f;
				transform->Local.Scale.x = scale.x;
				transform->Local.Scale.y = scale.y;
				break;
			}
			default:
				break;
		}

		scene.RefreshPhysics(entityId);
		return true;
	}

}
