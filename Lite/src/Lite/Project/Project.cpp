#include <Lite/Project/Project.h>
#include <Lite/Project/ProjectSerializer.h>

#include <Lite/Core/Assert.h>
#include <Lite/Core/IO/FileSystem.h>
#include <Lite/Core/Log/Logger.h>

namespace Lite {

	namespace {

		Ref<Project> s_ActiveProject;

	}

	const std::filesystem::path& Project::GetProjectDirectory()
	{
		LITE_CORE_ASSERT(s_ActiveProject, "No active project");
		return s_ActiveProject->m_ProjectDirectory;
	}

	std::filesystem::path Project::GetAssetDirectory()
	{
		LITE_CORE_ASSERT(s_ActiveProject, "No active project");
		return GetProjectDirectory() / s_ActiveProject->m_Config.AssetDirectory;
	}

	std::filesystem::path Project::GetAssetFileSystemPath(const std::filesystem::path& path)
	{
		LITE_CORE_ASSERT(s_ActiveProject, "No active project");
		return GetAssetDirectory() / path;
	}

	Ref<Project> Project::GetActive()
	{
		return s_ActiveProject;
	}

	Ref<Project> Project::New()
	{
		s_ActiveProject = CreateRef<Project>();
		return s_ActiveProject;
	}

	Ref<Project> Project::Load(const std::filesystem::path& path)
	{
		Ref<Project> project = CreateRef<Project>();
		ProjectSerializer serializer(project);
		if (!serializer.Deserialize(path))
			return nullptr;

		project->m_ProjectDirectory = path.parent_path();
		s_ActiveProject = project;
		EnsureContentFolders();
		LITE_INFO("Project loaded: {} ({})", project->m_Config.Name, path.string());
		return s_ActiveProject;
	}

	bool Project::SaveActive(const std::filesystem::path& path)
	{
		if (!s_ActiveProject)
		{
			LITE_ERROR("Failed to save project: no active project");
			return false;
		}

		ProjectSerializer serializer(s_ActiveProject);
		if (!serializer.Serialize(path))
			return false;

		s_ActiveProject->m_ProjectDirectory = path.parent_path();
		EnsureContentFolders();
		LITE_INFO("Project saved: {}", path.string());
		return true;
	}

	void Project::EnsureContentFolders()
	{
		if (!s_ActiveProject || s_ActiveProject->m_ProjectDirectory.empty())
			return;

		if (s_ActiveProject->m_Config.AssetDirectory.empty())
			s_ActiveProject->m_Config.AssetDirectory = "assets";

		std::filesystem::path assets = s_ActiveProject->m_ProjectDirectory / s_ActiveProject->m_Config.AssetDirectory;
		std::error_code error;
		std::filesystem::create_directories(assets, error);
		for (const char* folder : ContentFolders)
			std::filesystem::create_directories(assets / folder, error);
	}

	std::filesystem::path Project::Locate(const std::filesystem::path& relative)
	{
		return FileSystem::Locate(relative);
	}

}
