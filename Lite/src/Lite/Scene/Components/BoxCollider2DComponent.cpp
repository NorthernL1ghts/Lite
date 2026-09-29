#include <Lite/Scene/Components/BoxCollider2DComponent.h>
#include <Lite/Scene/Components/ComponentInstall.h>
#include <Lite/Scene/Components/SceneYaml.h>

namespace Lite {

	namespace {

		const char* Label(Entity entity)
		{
			return entity.Has<BoxCollider2DComponent>() ? "Box Collider 2D" : nullptr;
		}

		void Read(Scene& scene, uint32_t entity, const YAML::Node& node)
		{
			auto* box = static_cast<BoxCollider2DComponent*>(scene.AddComponent(ComponentId::BoxCollider2D, entity));
			if (box == nullptr)
				return;

			if (node["size"])
				box->Size = ReadVec2(node["size"], box->Size);
			if (node["offset"])
				box->Offset = ReadVec2(node["offset"], box->Offset);
			if (node["trigger"])
				box->IsTrigger = node["trigger"].as<bool>();
		}

		void Write(YAML::Node& node, const void* component)
		{
			const auto& box = *static_cast<const BoxCollider2DComponent*>(component);
			node["size"] = WriteVec2(box.Size);
			node["offset"] = WriteVec2(box.Offset);
			node["trigger"] = box.IsTrigger;
		}

		struct Registration
		{
			Registration()
			{
				InstallComponent<BoxCollider2DComponent>(ComponentId::BoxCollider2D, "box-collider2d", "Box Collider 2D", Label, Read, Write);
			}
		};

		Registration g_Registration;

	}

}
