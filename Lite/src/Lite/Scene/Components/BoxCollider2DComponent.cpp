#include <Lite/Scene/Components/BoxCollider2DComponent.h>

#include <Lite/Scene/Components/ComponentStorage.h>
#include <Lite/Scene/Scene.h>

#include <format>

namespace Lite {

	namespace {

		const char* Label(Entity entity)
		{
			return entity.Has<BoxCollider2DComponent>() ? "Box Collider 2D" : nullptr;
		}

		void Add(Entity entity)
		{
			entity.Add<BoxCollider2DComponent>();
		}

		bool Read(Scene& scene, uint32_t entity, std::string_view key, std::istream& stream, ComponentField)
		{
			auto* box = static_cast<BoxCollider2DComponent*>(scene.GetComponent(ComponentId::BoxCollider2D, entity));
			if (box == nullptr)
				return false;

			if (key == "size")
				stream >> box->Size.x >> box->Size.y;
			else if (key == "offset")
				stream >> box->Offset.x >> box->Offset.y;
			else if (key == "trigger")
			{
				int trigger = 0;
				stream >> trigger;
				box->IsTrigger = trigger != 0;
			}
			else
				return false;

			return true;
		}

		void Write(std::ostream& output, const void* component)
		{
			const auto& box = *static_cast<const BoxCollider2DComponent*>(component);
			output << "component box-collider2d\n";
			output << std::format("size {:.4f} {:.4f}\n", box.Size.x, box.Size.y);
			output << std::format("offset {:.4f} {:.4f}\n", box.Offset.x, box.Offset.y);
			output << std::format("trigger {}\n", box.IsTrigger ? 1 : 0);
		}

		struct Registration
		{
			Registration()
			{
				ComponentOps ops {};
				ops.Id = ComponentId::BoxCollider2D;
				ops.Section = "box-collider2d";
				ops.CatalogName = "Box Collider 2D";
				ops.Label = Label;
				ops.Add = Add;
				ops.Read = Read;
				ops.Write = Write;
				RegisterComponent(ops);
				RegisterComponentPool(ComponentId::BoxCollider2D, []() -> ComponentPool* { return new TypedPool<BoxCollider2DComponent>(); });
			}
		};

		Registration g_Registration;

	}

}
