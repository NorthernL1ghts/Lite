#include <Lite/Scene/Components/SortingComponent.h>

#include <Lite/Scene/Components/ComponentStorage.h>
#include <Lite/Scene/Scene.h>

#include <format>

namespace Lite {

	namespace {

		const char* Label(Entity entity)
		{
			return entity.Has<SortingComponent>() ? "Sorting" : nullptr;
		}

		void Add(Entity entity)
		{
			entity.Add<SortingComponent>();
		}

		bool Read(Scene& scene, uint32_t entity, std::string_view key, std::istream& stream, ComponentField)
		{
			if (key != "order")
				return false;

			auto* sorting = static_cast<SortingComponent*>(scene.GetComponent(ComponentId::Sorting, entity));
			if (sorting == nullptr)
				return false;

			stream >> sorting->Order;
			return true;
		}

		void Write(std::ostream& output, const void* component)
		{
			const auto& sorting = *static_cast<const SortingComponent*>(component);
			output << "component sorting\n";
			output << std::format("order {}\n", sorting.Order);
		}

		struct Registration
		{
			Registration()
			{
				ComponentOps ops {};
				ops.Id = ComponentId::Sorting;
				ops.Section = "sorting";
				ops.CatalogName = "Sorting";
				ops.Label = Label;
				ops.Add = Add;
				ops.Read = Read;
				ops.Write = Write;
				RegisterComponent(ops);
				RegisterComponentPool(ComponentId::Sorting, []() -> ComponentPool* { return new TypedPool<SortingComponent>(); });
			}
		};

		Registration g_Registration;

	}

}
