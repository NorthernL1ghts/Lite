#pragma once

#include <Lite/Scene/Components/Component.h>
#include <Lite/Scene/Scene.h>

#include <yaml-cpp/yaml.h>

#include <string_view>

namespace Lite {

	struct ComponentOps
	{
		ComponentId Id = ComponentId::Count;
		const char* Section = nullptr;
		const char* CatalogName = nullptr;
		const char* (*Label)(Entity entity) = nullptr;
		void (*Add)(Entity entity) = nullptr;
		void (*Read)(Scene& scene, uint32_t entity, const YAML::Node& node) = nullptr;
		void (*Write)(YAML::Node& node, const void* component) = nullptr;
		void (*Finish)(void* component) = nullptr;
	};

	LITE_API void RegisterComponent(const ComponentOps& ops);
	LITE_API const ComponentOps* FindComponent(ComponentId id);
	LITE_API const ComponentOps* FindComponentSection(std::string_view section);

}
