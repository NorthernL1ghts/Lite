#include <Lite/Scene/Components/TransformComponent.h>

#include <Lite/Scene/Components/ComponentStorage.h>
#include <Lite/Scene/Components/SceneText.h>
#include <Lite/Scene/Scene.h>

#include <format>

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

		bool Read(Scene& scene, uint32_t entity, std::string_view key, std::istream& stream, ComponentField)
		{
			if (key != "position" && key != "rotation" && key != "scale")
				return false;

			auto* transform = static_cast<TransformComponent*>(scene.GetComponent(ComponentId::Transform, entity));
			if (transform == nullptr)
				return false;

			if (key == "position")
				ReadVec3(stream, transform->Local.Position);
			else if (key == "rotation")
			{
				float radians = 0.0f;
				stream >> radians;
				transform->Local.SetRotationZ(radians);
			}
			else
				ReadVec3(stream, transform->Local.Scale);

			return true;
		}

		void Write(std::ostream& output, const void* component)
		{
			const auto& transform = *static_cast<const TransformComponent*>(component);
			const Vec3& position = transform.Local.Position;
			const Vec3& scale = transform.Local.Scale;
			output << "component transform\n";
			output << std::format("position {:.4f} {:.4f} {:.4f}\n", position.x, position.y, position.z);
			output << std::format("rotation {:.4f}\n", transform.Local.GetRotationZ());
			output << std::format("scale {:.4f} {:.4f} {:.4f}\n", scale.x, scale.y, scale.z);
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
				ops.ReadLegacy = true;
				ops.ReadLoose = true;
				RegisterComponent(ops);
				RegisterComponentPool(ComponentId::Transform, []() -> ComponentPool* { return new TypedPool<TransformComponent>(); });
			}
		};

		Registration g_Registration;

	}

}
