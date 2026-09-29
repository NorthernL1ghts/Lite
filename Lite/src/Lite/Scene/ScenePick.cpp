#include <Lite/Scene/Scene.h>

#include <Lite/Renderer/MeshShape.h>
#include <Lite/Scene/SceneCamera.h>

#include <algorithm>
#include <vector>

namespace Lite {

	namespace {

		bool Hits(MeshType type, const Transform& transform, Vec2 point)
		{
			const Vec2* local = type == MeshType::Triangle ? kTriangleCorners : kQuadCorners;
			int count = type == MeshType::Triangle ? 3 : 4;
			Vec2 world[4];
			for (int index = 0; index < count; ++index)
			{
				Vec3 transformed = transform.TransformPoint({ local[index].x, local[index].y, 0.0f });
				world[index] = { transformed.x, transformed.y };
			}

			return PointInPolygon(point, world, count);
		}

	}

	uint32_t Scene::Pick(const Mat4& viewProjection, float mouseX, float mouseY, float windowW, float windowH)
	{
		Vec2 world;
		if (!ScreenToWorld(viewProjection, windowW, windowH, mouseX, mouseY, world))
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
			Lite::Entity entity = entities[index];
			if (entity.Get<MeshComponent>() == nullptr || entity.Get<TransformComponent>() == nullptr)
				continue;

			int order = 0;
			if (SortingComponent* sorting = entity.Get<SortingComponent>())
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

		for (Candidate& candidate : candidates)
		{
			MeshComponent* mesh = candidate.Entity.Get<MeshComponent>();
			TransformComponent* transform = candidate.Entity.Get<TransformComponent>();
			if (mesh == nullptr || transform == nullptr)
				continue;
			if (Hits(mesh->Type, transform->Local, { world.x, world.y }))
				return candidate.Entity.GetId();
		}

		return 0;
	}

}
