#include <Lite/Scene/Components/ComponentOps.h>
#include <Lite/Scene/Components/ComponentStorage.h>
#include <Lite/Scene/Components/SortingComponent.h>

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

		void Read(Scene& scene, uint32_t entity, const YAML::Node& node)
		{
			auto* sorting = static_cast<SortingComponent*>(scene.AddComponent(ComponentId::Sorting, entity));
			if (sorting == nullptr || !node["order"])
				return;

			sorting->Order = node["order"].as<int>();
		}

		void Write(YAML::Node& node, const void* component)
		{
			const auto& sorting = *static_cast<const SortingComponent*>(component);
			node["order"] = sorting.Order;
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
