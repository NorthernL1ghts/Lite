#include <Lite/Scene/Components/ComponentInstall.h>
#include <Lite/Scene/Components/SortingComponent.h>

namespace Lite {

	namespace {

		const char* Label(Entity entity)
		{
			return entity.Has<SortingComponent>() ? "Sorting" : nullptr;
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
				InstallComponent<SortingComponent>(ComponentId::Sorting, "sorting", "Sorting", Label, Read, Write);
			}
		};

		Registration g_Registration;

	}

}
