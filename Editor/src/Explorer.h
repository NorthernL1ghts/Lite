#pragma once

#include <filesystem>
#include <functional>
#include <string>

class Explorer
{
public:
	void Draw(
		const std::function<void(const std::string&)>& openScene,
		const std::function<void(const std::string&)>& openProject);

private:
	std::filesystem::path ProjectRoot() const;
	std::filesystem::path Destination() const;
	bool InsideProject(const std::filesystem::path& path) const;
	void DrawGrid(
		const std::filesystem::path& directory,
		const std::function<void(const std::string&)>& openScene,
		const std::function<void(const std::string&)>& openProject);
	void CreateFolder();
	void ImportFiles();

	std::filesystem::path m_Selected;
	std::filesystem::path m_View;
	std::filesystem::path m_Project;
	char m_FolderName[128] {};
	bool m_ShowNewFolder = false;
};
