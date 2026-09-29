#include <Lite/Scene/Components/MeshComponent.h>

#include <Lite/Scene/Components/ComponentStorage.h>
#include <Lite/Scene/Scene.h>

#include <format>
#include <string>

namespace Lite {

	namespace {

		const char* MeshName(MeshType type)
		{
			switch (type)
			{
				case MeshType::Sprite: return "sprite";
				case MeshType::Triangle: return "triangle";
				default: return "quad";
			}
		}

		MeshType ParseMesh(std::string_view type)
		{
			if (type == "sprite")
				return MeshType::Sprite;
			if (type == "triangle")
				return MeshType::Triangle;
			return MeshType::Quad;
		}

		const char* Label(Entity entity)
		{
			MeshComponent* mesh = entity.Get<MeshComponent>();
			if (mesh == nullptr)
				return nullptr;
			switch (mesh->Type)
			{
				case MeshType::Triangle: return "Triangle";
				case MeshType::Sprite: return "Sprite";
				default: return "Quad";
			}
		}

		void Add(Entity entity)
		{
			entity.Add<MeshComponent>();
		}

		bool Read(Scene& scene, uint32_t entity, std::string_view key, std::istream& stream, ComponentField)
		{
			if (key != "type" && key != "kind")
				return false;

			auto* mesh = static_cast<MeshComponent*>(scene.AddComponent(ComponentId::Mesh, entity));
			if (mesh == nullptr)
				return false;

			std::string type;
			stream >> type;
			mesh->Type = ParseMesh(type);
			return true;
		}

		void Write(std::ostream& output, const void* component)
		{
			const auto& mesh = *static_cast<const MeshComponent*>(component);
			output << "component mesh\n";
			output << std::format("type {}\n", MeshName(mesh.Type));
		}

		struct Registration
		{
			Registration()
			{
				ComponentOps ops {};
				ops.Id = ComponentId::Mesh;
				ops.Section = "mesh";
				ops.CatalogName = "Quad";
				ops.Label = Label;
				ops.Add = Add;
				ops.Read = Read;
				ops.Write = Write;
				ops.ReadLegacy = true;
				RegisterComponent(ops);
				RegisterComponentPool(ComponentId::Mesh, []() -> ComponentPool* { return new TypedPool<MeshComponent>(); });
			}
		};

		Registration g_Registration;

	}

}
