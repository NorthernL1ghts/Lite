#include <Lite/Project/ProjectSerializer.h>

#include <Lite/Core/Assert.h>
#include <Lite/Core/Log/Logger.h>

#include <yaml-cpp/yaml.h>

#include <fstream>

namespace Lite {

	ProjectSerializer::ProjectSerializer(Ref<Project> project)
		: m_Project(std::move(project))
	{
	}

	bool ProjectSerializer::Serialize(const std::filesystem::path& filepath)
	{
		LITE_CORE_ASSERT(m_Project, "Project serializer has no project");
		const ProjectConfig& config = m_Project->GetConfig();

		YAML::Emitter out;
		out << YAML::BeginMap;
		out << YAML::Key << "Project" << YAML::Value;
		out << YAML::BeginMap;
		out << YAML::Key << "Name" << YAML::Value << config.Name;
		out << YAML::Key << "StartScene" << YAML::Value << config.StartScene.generic_string();
		out << YAML::Key << "AssetDirectory" << YAML::Value << config.AssetDirectory.generic_string();
		out << YAML::Key << "ScriptModulePath" << YAML::Value << config.ScriptModulePath.generic_string();
		out << YAML::EndMap;
		out << YAML::EndMap;

		if (!out.good())
		{
			LITE_ERROR("Failed to save project file '{}'", filepath.string());
			return false;
		}

		std::ofstream fout(filepath);
		if (!fout)
		{
			LITE_ERROR("Failed to save project file '{}'", filepath.string());
			return false;
		}

		fout << out.c_str();
		return static_cast<bool>(fout);
	}

	bool ProjectSerializer::Deserialize(const std::filesystem::path& filepath)
	{
		LITE_CORE_ASSERT(m_Project, "Project serializer has no project");
		ProjectConfig& config = m_Project->GetConfig();

		YAML::Node data;
		try
		{
			data = YAML::LoadFile(filepath.string());

			YAML::Node projectNode = data["Project"];
			if (!projectNode)
				return false;

			config.Name = projectNode["Name"].as<std::string>();
			config.StartScene = projectNode["StartScene"].as<std::string>();
			config.AssetDirectory = projectNode["AssetDirectory"].as<std::string>();
			config.ScriptModulePath = projectNode["ScriptModulePath"].as<std::string>();
		}
		catch (const YAML::Exception& exception)
		{
			LITE_ERROR("Failed to load project file '{}'\n     {}", filepath.string(), exception.what());
			return false;
		}

		return true;
	}

}
