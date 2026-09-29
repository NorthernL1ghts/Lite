#include <Lite/Scene/Components/ComponentOps.h>
#include <Lite/Scene/Components/ComponentStorage.h>
#include <Lite/Scene/Components/SpinComponent.h>

namespace Lite {

	namespace {

		const char* Label(Entity entity)
		{
			return entity.Has<SpinComponent>() ? "Spin" : nullptr;
		}

		void Add(Entity entity)
		{
			entity.Add<SpinComponent>();
		}

		void Read(Scene& scene, uint32_t entity, const YAML::Node& node)
		{
			auto* spin = static_cast<SpinComponent*>(scene.AddComponent(ComponentId::Spin, entity));
			if (spin == nullptr || !node["rate"])
				return;

			spin->Rate = node["rate"].as<float>();
		}

		void Write(YAML::Node& node, const void* component)
		{
			const auto& spin = *static_cast<const SpinComponent*>(component);
			node["rate"] = spin.Rate;
		}

		struct Registration
		{
			Registration()
			{
				ComponentOps ops {};
				ops.Id = ComponentId::Spin;
				ops.Section = "spin";
				ops.CatalogName = "Spin";
				ops.Label = Label;
				ops.Add = Add;
				ops.Read = Read;
				ops.Write = Write;
				RegisterComponent(ops);
				RegisterComponentPool(ComponentId::Spin, []() -> ComponentPool* { return new TypedPool<SpinComponent>(); });
			}
		};

		Registration g_Registration;

	}

}
