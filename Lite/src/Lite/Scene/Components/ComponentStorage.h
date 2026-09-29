#pragma once

#include <Lite/Core/Base.h>
#include <Lite/Scene/Components/Component.h>

#include <cstdint>
#include <memory>
#include <unordered_map>

namespace Lite {

	class ComponentPool
	{
	public:
		virtual ~ComponentPool() = default;
		virtual void* Emplace(uint32_t id) = 0;
		virtual void* Find(uint32_t id) = 0;
		virtual const void* Find(uint32_t id) const = 0;
		virtual void Erase(uint32_t id) = 0;
	};

	template<typename T>
	class TypedPool final : public ComponentPool
	{
	public:
		void* Emplace(uint32_t id) override
		{
			return &m_Items.try_emplace(id).first->second;
		}

		void* Find(uint32_t id) override
		{
			auto found = m_Items.find(id);
			return found == m_Items.end() ? nullptr : &found->second;
		}

		const void* Find(uint32_t id) const override
		{
			auto found = m_Items.find(id);
			return found == m_Items.end() ? nullptr : &found->second;
		}

		void Erase(uint32_t id) override
		{
			m_Items.erase(id);
		}

	private:
		std::unordered_map<uint32_t, T> m_Items;
	};

	using ComponentPoolCreate = ComponentPool* (*)();

	LITE_API void RegisterComponentPool(ComponentId id, ComponentPoolCreate create);

	class ComponentStorage
	{
	public:
		void* Emplace(ComponentId id, uint32_t entity);
		void* Find(ComponentId id, uint32_t entity);
		const void* Find(ComponentId id, uint32_t entity) const;
		void Erase(ComponentId id, uint32_t entity);

	private:
		ComponentPool* PoolFor(ComponentId id);
		ComponentPool* TryPool(ComponentId id);
		const ComponentPool* TryPool(ComponentId id) const;

		std::unique_ptr<ComponentPool> m_Pools[static_cast<size_t>(ComponentId::Count)];
	};

}
