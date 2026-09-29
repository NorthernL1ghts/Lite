#include <Lite/Scene/Components/ComponentStorage.h>

namespace Lite {

	namespace {

		ComponentPoolCreate* PoolFactories()
		{
			static ComponentPoolCreate factories[static_cast<size_t>(ComponentId::Count)] {};
			return factories;
		}

	}

	void RegisterComponentPool(ComponentId id, ComponentPoolCreate create)
	{
		PoolFactories()[static_cast<size_t>(id)] = create;
	}

	ComponentPool* ComponentStorage::PoolFor(ComponentId id)
	{
		size_t index = static_cast<size_t>(id);
		if (m_Pools[index] == nullptr)
		{
			ComponentPoolCreate create = PoolFactories()[index];
			if (create != nullptr)
				m_Pools[index].reset(create());
		}

		return m_Pools[index].get();
	}

	ComponentPool* ComponentStorage::TryPool(ComponentId id)
	{
		return m_Pools[static_cast<size_t>(id)].get();
	}

	const ComponentPool* ComponentStorage::TryPool(ComponentId id) const
	{
		return m_Pools[static_cast<size_t>(id)].get();
	}

	void* ComponentStorage::Emplace(ComponentId id, uint32_t entity)
	{
		ComponentPool* pool = PoolFor(id);
		return pool != nullptr ? pool->Emplace(entity) : nullptr;
	}

	void* ComponentStorage::Find(ComponentId id, uint32_t entity)
	{
		ComponentPool* pool = TryPool(id);
		return pool != nullptr ? pool->Find(entity) : nullptr;
	}

	const void* ComponentStorage::Find(ComponentId id, uint32_t entity) const
	{
		const ComponentPool* pool = TryPool(id);
		return pool != nullptr ? pool->Find(entity) : nullptr;
	}

	void ComponentStorage::Erase(ComponentId id, uint32_t entity)
	{
		ComponentPool* pool = TryPool(id);
		if (pool != nullptr)
			pool->Erase(entity);
	}

}
