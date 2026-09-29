#include <Lite/Scene/Components/CircleCollider2DComponent.h>

#include <Lite/Scene/Components/ComponentStorage.h>
#include <Lite/Scene/Scene.h>

#include <format>

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

		bool Read(Scene& scene, uint32_t entity, std::string_view key, std::istream& stream, ComponentField)
		{
			auto* circle = static_cast<CircleCollider2DComponent*>(scene.GetComponent(ComponentId::CircleCollider2D, entity));
			if (circle == nullptr)
				return false;

			if (key == "radius")
				stream >> circle->Radius;
			else if (key == "offset")
				stream >> circle->Offset.x >> circle->Offset.y;
			else if (key == "trigger")
			{
				int trigger = 0;
				stream >> trigger;
				circle->IsTrigger = trigger != 0;
			}
			else
				return false;

			return true;
		}

		void Write(std::ostream& output, const void* component)
		{
			const auto& circle = *static_cast<const CircleCollider2DComponent*>(component);
			output << "component circle-collider2d\n";
			output << std::format("radius {:.4f}\n", circle.Radius);
			output << std::format("offset {:.4f} {:.4f}\n", circle.Offset.x, circle.Offset.y);
			output << std::format("trigger {}\n", circle.IsTrigger ? 1 : 0);
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
