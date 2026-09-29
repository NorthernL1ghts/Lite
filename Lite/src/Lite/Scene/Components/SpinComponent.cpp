#include <Lite/Scene/Components/ComponentInstall.h>
#include <Lite/Scene/Components/SpinComponent.h>

namespace Lite {

	namespace {

		const char* Label(Entity entity)
		{
			return entity.Has<SpinComponent>() ? "Spin" : nullptr;
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
				InstallComponent<SpinComponent>(ComponentId::Spin, "spin", "Spin", Label, Read, Write);
			}
		};

		Registration g_Registration;

	}

}
