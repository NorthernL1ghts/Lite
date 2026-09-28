#include <Lite/Scene/Scene.h>

namespace Lite {

	namespace {

		const char* LabelTransform(Entity entity)
		{
			return entity.Has<TransformComponent>() ? "Transform" : nullptr;
		}

		const char* LabelCamera(Entity entity)
		{
			CameraComponent* camera = entity.Get<CameraComponent>();
			if (camera == nullptr)
				return nullptr;
			return camera->Projection == CameraProjection::Perspective ? "Perspective Camera" : "Orthographic Camera";
		}

		const char* LabelMesh(Entity entity)
		{
			MeshComponent* mesh = entity.Get<MeshComponent>();
			if (mesh == nullptr)
				return nullptr;
			switch (mesh->Type)
			{
				case MeshType::Triangle: return "Triangle";
				case MeshType::Sprite: return "Sprite";
				default: return "Quad";
			}
		}

		const char* LabelMaterial(Entity entity)
		{
			return entity.Has<MaterialComponent>() ? "Material" : nullptr;
		}

		const char* LabelSpin(Entity entity)
		{
			return entity.Has<SpinComponent>() ? "Spin" : nullptr;
		}

		const char* LabelBody(Entity entity)
		{
			Rigidbody2DComponent* body = entity.Get<Rigidbody2DComponent>();
			if (body == nullptr)
				return nullptr;
			switch (body->Type)
			{
				case BodyType::Static: return "Static Rigidbody";
				case BodyType::Kinematic: return "Kinematic Rigidbody";
				default: return "Dynamic Rigidbody";
			}
		}

		const char* LabelBox(Entity entity)
		{
			return entity.Has<BoxCollider2DComponent>() ? "Box Collider 2D" : nullptr;
		}

		const char* LabelCircle(Entity entity)
		{
			return entity.Has<CircleCollider2DComponent>() ? "Circle Collider 2D" : nullptr;
		}

		const char* LabelSorting(Entity entity)
		{
			return entity.Has<SortingComponent>() ? "Sorting" : nullptr;
		}

		void AddTransform(Entity entity) { entity.Add<TransformComponent>(); }
		void AddCamera(Entity entity) { entity.Add<CameraComponent>(); }
		void AddMesh(Entity entity) { entity.Add<MeshComponent>(); }
		void AddMaterial(Entity entity) { entity.Add<MaterialComponent>(); }
		void AddSpin(Entity entity) { entity.Add<SpinComponent>(); }
		void AddBody(Entity entity) { entity.Add<Rigidbody2DComponent>(); }
		void AddBox(Entity entity) { entity.Add<BoxCollider2DComponent>(); }
		void AddCircle(Entity entity) { entity.Add<CircleCollider2DComponent>(); }
		void AddSorting(Entity entity) { entity.Add<SortingComponent>(); }

	}

	const ComponentEntry* ComponentCatalog(size_t& count)
	{
		static const ComponentEntry entries[] = {
			{ "Transform", LabelTransform, AddTransform },
			{ "Orthographic Camera", LabelCamera, AddCamera },
			{ "Quad", LabelMesh, AddMesh },
			{ "Material", LabelMaterial, AddMaterial },
			{ "Spin", LabelSpin, AddSpin },
			{ "Rigidbody 2D", LabelBody, AddBody },
			{ "Box Collider 2D", LabelBox, AddBox },
			{ "Circle Collider 2D", LabelCircle, AddCircle },
			{ "Sorting", LabelSorting, AddSorting }
		};

		count = sizeof(entries) / sizeof(entries[0]);
		return entries;
	}

}
