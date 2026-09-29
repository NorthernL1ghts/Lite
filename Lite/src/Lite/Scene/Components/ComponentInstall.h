#pragma once

#include <Lite/Scene/Components/ComponentOps.h>
#include <Lite/Scene/Components/ComponentStorage.h>

namespace Lite {

	template<typename T>
	void InstallComponent(
		ComponentId id,
		const char* section,
		const char* catalog,
		const char* (*label)(Entity),
		void (*read)(Scene&, uint32_t, const YAML::Node&),
		void (*write)(YAML::Node&, const void*),
		void (*finish)(void*) = nullptr)
	{
		ComponentOps ops {};
		ops.Id = id;
		ops.Section = section;
		ops.CatalogName = catalog;
		ops.Label = label;
		ops.Add = [](Entity entity) { entity.Add<T>(); };
		ops.Read = read;
		ops.Write = write;
		ops.Finish = finish;
		RegisterComponent(ops);
		RegisterComponentPool(id, []() -> ComponentPool* { return new TypedPool<T>(); });
	}

}
