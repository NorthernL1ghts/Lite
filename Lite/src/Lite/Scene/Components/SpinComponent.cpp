#include <Lite/Scene/Components/SpinComponent.h>

#include <Lite/Scene/Components/ComponentStorage.h>
#include <Lite/Scene/Scene.h>

#include <format>

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

		bool Read(Scene& scene, uint32_t entity, std::string_view key, std::istream& stream, ComponentField field)
		{
			if (key != "rate" && key != "spin")
				return false;

			float rate = 0.0f;
			stream >> rate;
			if (!field.InSection && rate == 0.0f)
				return true;

			auto* spin = static_cast<SpinComponent*>(scene.AddComponent(ComponentId::Spin, entity));
			if (spin == nullptr)
				return false;

			spin->Rate = rate;
			return true;
		}

		void Write(std::ostream& output, const void* component)
		{
			const auto& spin = *static_cast<const SpinComponent*>(component);
			output << "component spin\n";
			output << std::format("rate {:.4f}\n", spin.Rate);
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
				ops.ReadLegacy = true;
				RegisterComponent(ops);
				RegisterComponentPool(ComponentId::Spin, []() -> ComponentPool* { return new TypedPool<SpinComponent>(); });
			}
		};

		Registration g_Registration;

	}

}
