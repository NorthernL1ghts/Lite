#include <Lite/Scene/Components/CircleCollider2DComponent.h>
#include <Lite/Scene/Components/ComponentOps.h>
#include <Lite/Scene/Components/ComponentStorage.h>
#include <Lite/Scene/Components/SceneYaml.h>

namespace Lite {

	namespace {

		const char* Label(Entity entity)
		{
			return entity.Has<CircleCollider2DComponent>() ? "Circle Collider 2D" : nullptr;
		}

		void Add(Entity entity)
		{
			entity.Add<CircleCollider2DComponent>();
		}

		void Read(Scene& scene, uint32_t entity, const YAML::Node& node)
		{
			auto* circle = static_cast<CircleCollider2DComponent*>(scene.AddComponent(ComponentId::CircleCollider2D, entity));
			if (circle == nullptr)
				return;

			if (node["radius"])
				circle->Radius = node["radius"].as<float>();
			if (node["offset"])
				circle->Offset = ReadVec2(node["offset"], circle->Offset);
			if (node["trigger"])
				circle->IsTrigger = node["trigger"].as<bool>();
		}

		void Write(YAML::Node& node, const void* component)
		{
			const auto& circle = *static_cast<const CircleCollider2DComponent*>(component);
			node["radius"] = circle.Radius;
			node["offset"] = WriteVec2(circle.Offset);
			node["trigger"] = circle.IsTrigger;
		}

		struct Registration
		{
			Registration()
			{
				ComponentOps ops {};
				ops.Id = ComponentId::CircleCollider2D;
				ops.Section = "circle-collider2d";
				ops.CatalogName = "Circle Collider 2D";
				ops.Label = Label;
				ops.Add = Add;
				ops.Read = Read;
				ops.Write = Write;
				RegisterComponent(ops);
				RegisterComponentPool(ComponentId::CircleCollider2D, []() -> ComponentPool* { return new TypedPool<CircleCollider2DComponent>(); });
			}
		};

		Registration g_Registration;

	}

}
