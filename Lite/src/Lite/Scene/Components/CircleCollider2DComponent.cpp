#include <Lite/Scene/Components/CircleCollider2DComponent.h>
#include <Lite/Scene/Components/ComponentInstall.h>
#include <Lite/Scene/Components/SceneYaml.h>

namespace Lite {

	namespace {

		const char* Label(Entity entity)
		{
			return entity.Has<CircleCollider2DComponent>() ? "Circle Collider 2D" : nullptr;
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
			if (node["layer"])
				circle->Layer = node["layer"].as<uint64_t>();
			if (node["mask"])
				circle->Mask = node["mask"].as<uint64_t>();
		}

		void Write(YAML::Node& node, const void* component)
		{
			const auto& circle = *static_cast<const CircleCollider2DComponent*>(component);
			node["radius"] = circle.Radius;
			node["offset"] = WriteVec2(circle.Offset);
			node["trigger"] = circle.IsTrigger;
			if (circle.Layer != kDefaultCollisionLayer)
				node["layer"] = circle.Layer;
			if (circle.Mask != kDefaultCollisionMask)
				node["mask"] = circle.Mask;
		}

		struct Registration
		{
			Registration()
			{
				InstallComponent<CircleCollider2DComponent>(ComponentId::CircleCollider2D, "circle-collider2d", "Circle Collider 2D", Label, Read, Write);
			}
		};

		Registration g_Registration;

	}

}
