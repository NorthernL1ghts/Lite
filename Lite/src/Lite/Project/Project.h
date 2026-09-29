#pragma once

#include <Lite/Core/Base.h>
#include <Lite/Core/Memory.h>

#include <filesystem>
#include <string>

namespace Lite {

	struct ProjectConfig
	{
		std::string Name = "Untitled";

		std::filesystem::path StartScene;

		std::filesystem::path AssetDirectory;
		std::filesystem::path ScriptModulePath;
	};

	class LITE_API Project
	{
	public:
		static const std::filesystem::path& GetProjectDirectory();
		static std::filesystem::path GetAssetDirectory();
		static std::filesystem::path GetAssetFileSystemPath(const std::filesystem::path& path);

		ProjectConfig& GetConfig() { return m_Config; }

		static Ref<Project> GetActive();
		static Ref<Project> New();
		static Ref<Project> Load(const std::filesystem::path& path);
		static bool SaveActive(const std::filesystem::path& path);
		static std::filesystem::path Locate(const std::filesystem::path& relative);

	private:
		ProjectConfig m_Config;
		std::filesystem::path m_ProjectDirectory;
	};

}
