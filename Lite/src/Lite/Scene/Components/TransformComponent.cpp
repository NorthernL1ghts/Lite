#include <Lite/Scene/Components/ComponentOps.h>
#include <Lite/Scene/Components/ComponentStorage.h>
#include <Lite/Scene/Components/SceneYaml.h>
#include <Lite/Scene/Components/TransformComponent.h>

namespace Lite {

	namespace {

		const char* Label(Entity entity)
		{
			return entity.Has<TransformComponent>() ? "Transform" : nullptr;
		}

		void Add(Entity entity)
		{
			entity.Add<TransformComponent>();
		}

		void Read(Scene& scene, uint32_t entity, const YAML::Node& node)
		{
			auto* transform = static_cast<TransformComponent*>(scene.GetComponent(ComponentId::Transform, entity));
			if (transform == nullptr)
				transform = static_cast<TransformComponent*>(scene.AddComponent(ComponentId::Transform, entity));
			if (transform == nullptr)
				return;

			if (node["position"])
				transform->Local.Position = ReadVec3(node["position"], transform->Local.Position);
			if (node["rotation"])
				transform->Local.SetRotationZ(node["rotation"].as<float>());
			if (node["scale"])
				transform->Local.Scale = ReadVec3(node["scale"], transform->Local.Scale);
		}

		void Write(YAML::Node& node, const void* component)
		{
			const auto& transform = *static_cast<const TransformComponent*>(component);
			node["position"] = WriteVec3(transform.Local.Position);
			node["rotation"] = transform.Local.GetRotationZ();
			node["scale"] = WriteVec3(transform.Local.Scale);
		}

		struct Registration
		{
			Registration()
			{
				ComponentOps ops {};
				ops.Id = ComponentId::Transform;
				ops.Section = "transform";
				ops.CatalogName = "Transform";
				ops.Label = Label;
				ops.Add = Add;
				ops.Read = Read;
				ops.Write = Write;
				RegisterComponent(ops);
				RegisterComponentPool(ComponentId::Transform, []() -> ComponentPool* { return new TypedPool<TransformComponent>(); });
			}
		};

		Registration g_Registration;

	}

}
