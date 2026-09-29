#include <Lite/Scene/Scene.h>

namespace Lite {

	namespace {

		ComponentOps* Table()
		{
			static ComponentOps ops[static_cast<size_t>(ComponentId::Count)] {};
			return ops;
		}

	}

	void RegisterComponent(const ComponentOps& ops)
	{
		Table()[static_cast<size_t>(ops.Id)] = ops;
	}

	const ComponentOps* FindComponent(ComponentId id)
	{
		if (id >= ComponentId::Count)
			return nullptr;

		const ComponentOps& ops = Table()[static_cast<size_t>(id)];
		return ops.CatalogName != nullptr ? &ops : nullptr;
	}

	const ComponentOps* FindComponentSection(std::string_view section)
	{
		for (size_t index = 0; index < static_cast<size_t>(ComponentId::Count); ++index)
		{
			const ComponentOps& ops = Table()[index];
			if (ops.Section != nullptr && ops.Section == section)
				return &ops;
		}

		return nullptr;
	}

	const ComponentEntry* ComponentCatalog(size_t& count)
	{
		static ComponentEntry entries[static_cast<size_t>(ComponentId::Count)] {};
		static size_t filled = 0;
		if (filled == 0)
		{
			for (size_t index = 0; index < static_cast<size_t>(ComponentId::Count); ++index)
			{
				const ComponentOps& ops = Table()[index];
				if (ops.CatalogName == nullptr)
					continue;

				entries[filled++] = { ops.CatalogName, ops.Label, ops.Add };
			}
		}

		count = filled;
		return entries;
	}

}
