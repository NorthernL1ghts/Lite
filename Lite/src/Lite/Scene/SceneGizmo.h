#pragma once

#include <Lite/Renderer/MeshShape.h>
#include <Lite/Scene/Components/MeshComponent.h>
#include <Lite/Scene/Scene.h>
#include <Lite/Scene/SceneCamera.h>

#include <algorithm>
#include <array>
#include <cmath>
#include <optional>
#include <span>

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
		std::array<Vec2, 4> World {};
		std::array<Vec2, 4> Screen {};
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
		Vec2 Center {};
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

	inline std::span<const Vec2> MeshCorners(const MeshComponent* mesh)
	{
		if (mesh != nullptr && mesh->Type == MeshType::Triangle)
			return kTriangleCorners;
		return kQuadCorners;
	}

	inline bool BuildGizmo(const Mat4& viewProjection, float windowW, float windowH, const Entity& entity, GizmoLayout& layout)
	{
		const TransformComponent* transform = entity.Get<TransformComponent>();
		if (transform == nullptr)
			return false;

		const Transform world = entity.WorldTransform();
		const std::span<const Vec2> local = MeshCorners(entity.Get<MeshComponent>());
		layout = {};
		layout.CenterWorld = { world.Position.x, world.Position.y };
		const std::optional<Vec2> center = WorldToScreen(viewProjection, windowW, windowH, layout.CenterWorld);
		if (!center)
			return false;
		layout.Center = *center;

		layout.Count = static_cast<int>(local.size());
		float reach = 0.0f;
		for (int index = 0; index < layout.Count; ++index)
		{
			const Vec3 transformed = world.TransformPoint({ local[static_cast<size_t>(index)].x, local[static_cast<size_t>(index)].y, 0.0f });
			layout.World[static_cast<size_t>(index)] = { transformed.x, transformed.y };
			const std::optional<Vec2> screen = WorldToScreen(viewProjection, windowW, windowH, layout.World[static_cast<size_t>(index)]);
			if (!screen)
				return false;
			layout.Screen[static_cast<size_t>(index)] = *screen;
			reach = std::max(reach, (layout.Center - layout.Screen[static_cast<size_t>(index)]).Length());
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
			const float distance = (mouse - layout.Screen[static_cast<size_t>(index)]).Length();
			if (distance > best)
				continue;
			best = distance;
			hit = index;
		}

		return hit;
	}

	inline bool HitGizmoRing(const GizmoLayout& layout, Vec2 mouse)
	{
		const float distance = (mouse - layout.Center).Length();
		return std::abs(distance - layout.Ring) <= kGizmoRingHit;
	}

	inline GizmoHot HitGizmo(const GizmoLayout& layout, Vec2 mouse, std::optional<Vec2> world)
	{
		GizmoHot hot;
		const int corner = HitGizmoScale(layout, mouse);
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

		const std::span<const Vec2> outline { layout.World.data(), static_cast<size_t>(layout.Count) };
		if (world && PointInPolygon(*world, outline))
			hot.Action = GizmoAction::Move;
		return hot;
	}

	inline bool BeginGizmo(Scene& scene, const Mat4& viewProjection, float windowW, float windowH, uint32_t entityId, float mouseX, float mouseY, GizmoDrag& drag)
	{
		drag = {};
		const Entity entity = scene.GetEntity(entityId);
		const TransformComponent* transform = entity ? entity.Get<TransformComponent>() : nullptr;
		GizmoLayout layout;
		if (transform == nullptr || !BuildGizmo(viewProjection, windowW, windowH, entity, layout))
			return false;

		const std::optional<Vec2> world = ScreenToWorld(viewProjection, windowW, windowH, mouseX, mouseY);
		if (!world)
			return false;

		const GizmoHot hot = HitGizmo(layout, { mouseX, mouseY }, world);
		if (hot.Action == GizmoAction::None)
			return false;

		const Transform worldTransform = entity.WorldTransform();
		drag.Action = hot.Action;
		drag.Corner = hot.Corner;
		drag.Grab = *world;
		drag.Center = { worldTransform.Position.x, worldTransform.Position.y };
		drag.Position = transform->Local.Position;
		drag.Rotation = transform->Local.GetRotationZ();
		drag.Scale = transform->Local.Scale;
		drag.Angle = std::atan2(world->y - drag.Center.y, world->x - drag.Center.x);
		return true;
	}

	inline bool ApplyGizmo(Scene& scene, const Mat4& viewProjection, float windowW, float windowH, uint32_t entityId, float mouseX, float mouseY, GizmoDrag& drag)
	{
		Entity entity = scene.GetEntity(entityId);
		TransformComponent* transform = entity ? entity.Get<TransformComponent>() : nullptr;
		const std::optional<Vec2> world = ScreenToWorld(viewProjection, windowW, windowH, mouseX, mouseY);
		if (transform == nullptr || drag.Action == GizmoAction::None || !world)
		{
			drag.Action = GizmoAction::None;
			return false;
		}

		const uint32_t parent = scene.GetParent(entityId);
		const Transform parentWorld = parent != 0 ? scene.WorldTransform(parent) : Transform{};

		switch (drag.Action)
		{
			case GizmoAction::Move:
			{
				const Vec3 grab = parentWorld.InverseTransformPoint({ drag.Grab.x, drag.Grab.y, 0.0f });
				const Vec3 now = parentWorld.InverseTransformPoint({ world->x, world->y, 0.0f });
				transform->Local.Position.x = drag.Position.x + (now.x - grab.x);
				transform->Local.Position.y = drag.Position.y + (now.y - grab.y);
				break;
			}
			case GizmoAction::Rotate:
			{
				const float angle = std::atan2(world->y - drag.Center.y, world->x - drag.Center.x);
				float delta = angle - drag.Angle;
				while (delta > kPi)
					delta -= kPi * 2.0f;
				while (delta < -kPi)
					delta += kPi * 2.0f;
				drag.Spin += delta;
				drag.Angle = angle;
				transform->Local.SetRotationZ(drag.Rotation + drag.Spin);
				break;
			}
			case GizmoAction::Scale:
			{
				const std::span<const Vec2> local = MeshCorners(entity.Get<MeshComponent>());
				if (drag.Corner < 0 || drag.Corner >= static_cast<int>(local.size()))
					return false;

				const Vec3 parentPoint = parentWorld.InverseTransformPoint({ world->x, world->y, 0.0f });
				const Vec3 offset = parentPoint - drag.Position;
				const Vec3 localPoint = Quat::FromAxisAngle({ 0.0f, 0.0f, 1.0f }, drag.Rotation).Conjugate().Rotate(offset);
				const Vec2 corner = local[static_cast<size_t>(drag.Corner)];
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
				return false;
		}

		scene.RefreshPhysics(entityId);
		return true;
	}

}
