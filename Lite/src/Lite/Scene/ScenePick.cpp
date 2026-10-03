#include <Lite/Scene/Scene.h>

#include <Lite/Renderer/MeshShape.h>
#include <Lite/Scene/SceneCamera.h>

#include <algorithm>
#include <array>
#include <span>
#include <vector>

namespace Lite {

	namespace {

		bool Hits(MeshType type, const Transform& transform, Vec2 point)
		{
			const std::span<const Vec2> local = type == MeshType::Triangle ? std::span<const Vec2> { kTriangleCorners } : std::span<const Vec2> { kQuadCorners };
			std::array<Vec2, 4> world {};
			for (size_t index = 0; index < local.size(); ++index)
			{
				const Vec3 transformed = transform.TransformPoint({ local[index].x, local[index].y, 0.0f });
				world[index] = { transformed.x, transformed.y };
			}

			return PointInPolygon(point, std::span<const Vec2> { world.data(), local.size() });
		}

	}

	uint32_t Scene::Pick(const Mat4& viewProjection, float mouseX, float mouseY, float windowW, float windowH)
	{
		const std::optional<Vec2> world = ScreenToWorld(viewProjection, windowW, windowH, mouseX, mouseY);
		if (!world)
			return 0;

		struct Candidate
		{
			Entity Entity;
			int Plane = 0;
			int Order = 0;
			size_t Index = 0;
		};

		std::vector<Lite::Entity> entities = GetEntities();
		std::vector<Candidate> candidates;
		candidates.reserve(entities.size());
		for (size_t index = 0; index < entities.size(); ++index)
		{
			const Entity entity = entities[index];
			if (entity.Get<MeshComponent>() == nullptr || entity.Get<TransformComponent>() == nullptr)
				continue;

			int order = 0;
			if (const SortingComponent* sorting = entity.Get<SortingComponent>())
				order = sorting->Order;
			const Record* record = FindRecord(entity.GetId());
			int plane = record != nullptr ? PlaneOrder(record->Plane) : 0;
			candidates.push_back({ entity, plane, order, index });
		}

		std::stable_sort(candidates.begin(), candidates.end(), [](const Candidate& left, const Candidate& right)
		{
			if (left.Plane != right.Plane)
				return left.Plane > right.Plane;
			if (left.Order != right.Order)
				return left.Order > right.Order;
			return left.Index > right.Index;
		});

		for (const Candidate& candidate : candidates)
		{
			const Entity entity = candidate.Entity;
			const MeshComponent* mesh = entity.Get<MeshComponent>();
			const TransformComponent* transform = entity.Get<TransformComponent>();
			if (mesh == nullptr || transform == nullptr)
				continue;
			if (Hits(mesh->Type, entity.WorldTransform(), *world))
				return entity.GetId();
		}

		return 0;
	}

}
