#include <Lite/Scene/Components/MaterialComponent.h>

#include <Lite/Assets/AssetRegistry.h>
#include <Lite/Scene/Components/ComponentStorage.h>
#include <Lite/Scene/Components/SceneText.h>
#include <Lite/Scene/Console.h>
#include <Lite/Scene/Scene.h>

#include <format>
#include <string>

namespace Lite {

	namespace {

		const char* Label(Entity entity)
		{
			return entity.Has<MaterialComponent>() ? "Material" : nullptr;
		}

		void Add(Entity entity)
		{
			entity.Add<MaterialComponent>();
		}

		bool Read(Scene& scene, uint32_t entity, std::string_view key, std::istream& stream, ComponentField)
		{
			bool known = key == "shader" || key == "color" || key == "tiling" || key == "texture" || key == "vertex-colors" || key == "colors" || key == "corners";
			if (!known)
				return false;

			auto* material = static_cast<MaterialComponent*>(scene.AddComponent(ComponentId::Material, entity));
			if (material == nullptr)
				return false;

			if (key == "shader")
			{
				std::string shader;
				std::getline(stream >> std::ws, shader);
				material->Shader = Trim(shader);
			}
			else if (key == "color")
				ReadVec4(stream, material->Color);
			else if (key == "tiling")
				stream >> material->Tiling.x >> material->Tiling.y;
			else if (key == "texture")
			{
				std::string texture;
				std::getline(stream >> std::ws, texture);
				material->TexturePath = Trim(texture);
			}
			else if (key == "vertex-colors")
			{
				int enabled = 0;
				stream >> enabled;
				material->UseVertexColors = enabled != 0;
			}
			else
			{
				material->UseVertexColors = true;
				for (Vec4& color : material->Colors)
					ReadVec4(stream, color);
			}

			return true;
		}

		void Write(std::ostream& output, const void* component)
		{
			const auto& material = *static_cast<const MaterialComponent*>(component);
			output << "component material\n";
			output << std::format("shader {}\n", material.Shader.empty() ? kDefaultShader : material.Shader);
			output << std::format("color {:.4f} {:.4f} {:.4f} {:.4f}\n", material.Color.x, material.Color.y, material.Color.z, material.Color.w);
			output << std::format("tiling {:.4f} {:.4f}\n", material.Tiling.x, material.Tiling.y);
			if (!material.TexturePath.empty())
				output << std::format("texture {}\n", material.TexturePath);
			output << std::format("vertex-colors {}\n", material.UseVertexColors ? 1 : 0);
			if (material.UseVertexColors)
			{
				output << "colors";
				for (const Vec4& color : material.Colors)
					output << std::format(" {:.4f} {:.4f} {:.4f} {:.4f}", color.x, color.y, color.z, color.w);
				output << '\n';
			}
		}

		void Finish(void* component)
		{
			auto& material = *static_cast<MaterialComponent*>(component);
			if (material.Shader.empty())
				material.Shader = kDefaultShader;
			if (material.TexturePath.empty())
				return;

			material.Texture = AssetRegistry::Get().Load<Texture>(material.TexturePath);
			if (!material.Texture)
				Console::Log(std::format("Failed to load texture {}", material.TexturePath));
		}

		struct Registration
		{
			Registration()
			{
				ComponentOps ops {};
				ops.Id = ComponentId::Material;
				ops.Section = "material";
				ops.CatalogName = "Material";
				ops.Label = Label;
				ops.Add = Add;
				ops.Read = Read;
				ops.Write = Write;
				ops.Finish = Finish;
				ops.ReadLegacy = true;
				ops.ReadBeside = ComponentId::Spin;
				RegisterComponent(ops);
				RegisterComponentPool(ComponentId::Material, []() -> ComponentPool* { return new TypedPool<MaterialComponent>(); });
			}
		};

		Registration g_Registration;

	}

}
