#include <Lite/Scene/Scene.h>

#include <Lite/Renderer/MeshShape.h>

#include <algorithm>
#include <vector>

namespace Lite {

	namespace {

		bool Contains(Vec2 point, const Vec2* vertices, int count)
		{
			bool positive = false;
			bool negative = false;
			for (int index = 0; index < count; ++index)
			{
				const Vec2& current = vertices[index];
				const Vec2& next = vertices[(index + 1) % count];
				float cross = (next.x - current.x) * (point.y - current.y) - (next.y - current.y) * (point.x - current.x);
				if (cross > 0.0f)
					positive = true;
				if (cross < 0.0f)
					negative = true;
				if (positive && negative)
					return false;
			}

			return true;
		}

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

			return Contains(point, world, count);
		}

	}

	uint32_t Scene::Pick(const Mat4& viewProjection, float mouseX, float mouseY, float windowW, float windowH)
	{
		if (windowW <= 1.0f || windowH <= 1.0f)
			return 0;

		float ndcX = (mouseX / windowW) * 2.0f - 1.0f;
		float ndcY = 1.0f - (mouseY / windowH) * 2.0f;
		Vec4 world = viewProjection.Inverse() * Vec4(ndcX, ndcY, 0.0f, 1.0f);
		if (world.w != 0.0f)
			world /= world.w;

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
