#pragma once

#include <filesystem>
#include <functional>
#include <string>

class Explorer
{
public:
	void Draw(
		const std::function<void(const std::string&)>& openScene,
		const std::function<void(const std::string&)>& openProject,
		const std::function<void(const std::string&)>& openPrefab);

private:
	std::filesystem::path ProjectRoot() const;
	std::filesystem::path Destination() const;
	bool InsideProject(const std::filesystem::path& path) const;
	void DrawGrid(
		const std::filesystem::path& directory,
		const std::function<void(const std::string&)>& openScene,
		const std::function<void(const std::string&)>& openProject,
		const std::function<void(const std::string&)>& openPrefab);
	void CreateFolder();
	void ImportFiles();
	void DrawDetails();
	void ApplyRename();
	bool CanRename(const std::filesystem::path& path) const;

	std::filesystem::path m_Selected;
	std::filesystem::path m_View;
	std::filesystem::path m_Project;
	std::filesystem::path m_RenamePath;
	char m_FolderName[128] {};
	char m_Rename[260] {};
	bool m_ShowNewFolder = false;
	bool m_RenameEditing = false;
	bool m_FocusRename = false;
};
