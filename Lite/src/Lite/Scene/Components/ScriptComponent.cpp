#include <Lite/Scene/Components/ComponentInstall.h>
#include <Lite/Scene/Components/ScriptComponent.h>

namespace Lite {

	namespace {

		const char* Label(Entity entity)
		{
			return entity.Has<ScriptComponent>() ? "Script" : nullptr;
		}

		void Read(Scene& scene, uint32_t entity, const YAML::Node& node)
		{
			auto* script = static_cast<ScriptComponent*>(scene.AddComponent(ComponentId::Script, entity));
			if (script == nullptr || !node["class"])
				return;

			script->Class = node["class"].as<std::string>();
		}

		void Write(YAML::Node& node, const void* component)
		{
			const auto& script = *static_cast<const ScriptComponent*>(component);
			node["class"] = script.Class;
		}

		struct Registration
		{
			Registration()
			{
				InstallComponent<ScriptComponent>(ComponentId::Script, "script", "Script", Label, Read, Write);
			}
		};

		Registration g_Registration;

	}

}
