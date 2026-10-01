#include <Lite/Scene/Components/ComponentInstall.h>
#include <Lite/Scene/Components/MaterialComponent.h>
#include <Lite/Scene/Components/SceneYaml.h>

#include <Lite/Assets/AssetRegistry.h>
#include <Lite/Scene/Console.h>

#include <format>
#include <string>

namespace Lite {

	namespace {

		const char* Label(Entity entity)
		{
			return entity.Has<MaterialComponent>() ? "Material" : nullptr;
		}

		void Read(Scene& scene, uint32_t entity, const YAML::Node& node)
		{
			auto* material = static_cast<MaterialComponent*>(scene.AddComponent(ComponentId::Material, entity));
			if (material == nullptr)
				return;

			if (node["shader"])
				material->Shader = node["shader"].as<std::string>();
			if (node["color"])
				material->Color = ReadVec4(node["color"], material->Color);
			if (node["tiling"])
				material->Tiling = ReadVec2(node["tiling"], material->Tiling);
			if (node["texture"])
				material->TexturePath = node["texture"].as<std::string>();
			if (node["vertex-colors"])
				material->UseVertexColors = node["vertex-colors"].as<bool>();
			if (node["colors"] && node["colors"].IsSequence())
			{
				material->UseVertexColors = true;
				int index = 0;
				for (const YAML::Node& color : node["colors"])
				{
					if (index >= 4)
						break;
					material->Colors[index++] = ReadVec4(color, {});
				}
			}
		}

		void Write(YAML::Node& node, const void* component)
		{
			const auto& material = *static_cast<const MaterialComponent*>(component);
			node["shader"] = material.Shader.empty() ? kDefaultShader : material.Shader;
			node["color"] = WriteVec4(material.Color);
			node["tiling"] = WriteVec2(material.Tiling);
			if (!material.TexturePath.empty())
				node["texture"] = material.TexturePath;
			node["vertex-colors"] = material.UseVertexColors;
			if (material.UseVertexColors)
			{
				YAML::Node colors(YAML::NodeType::Sequence);
				for (const Vec4& color : material.Colors)
					colors.push_back(WriteVec4(color));
				node["colors"] = colors;
			}
		}

		void Finish(void* component)
		{
			auto& material = *static_cast<MaterialComponent*>(component);
			if (material.Shader.empty())
				material.Shader = kDefaultShader;
			if (material.TexturePath.empty())
				return;

			material.TexturePath = AssetRegistry::Get().Store(material.TexturePath);
			material.Texture = AssetRegistry::Get().Load<Texture>(material.TexturePath);
			if (!material.Texture)
				Console::Log(std::format("Failed to load texture {}", material.TexturePath));
		}

		struct Registration
		{
			Registration()
			{
				InstallComponent<MaterialComponent>(ComponentId::Material, "material", "Material", Label, Read, Write, Finish);
			}
		};

		Registration g_Registration;

	}

}
