#pragma once

#include <functional>
#include <string>

namespace Lite {
	class Scene;
}

class SceneBrowser
{
public:
	void ShowOpen();
	void ShowSave();
	void ShowOpenProject();
	void ShowSaveProject();
	void Draw(
		Lite::Scene* scene,
		const std::function<void(const std::string&)>& openScene,
		const std::function<void()>& onSaved,
		const std::function<void(const std::string&)>& openProject,
		const std::function<bool(const std::string&)>& saveProject);

private:
	void Prepare(Lite::Scene* scene, bool project);
	void ApplyDirectoryText();
	std::string Selection(const char* extension) const;
	void DrawFiles(
		bool save,
		bool project,
		Lite::Scene* scene,
		const std::function<void(const std::string&)>& openScene,
		const std::function<void()>& onSaved,
		const std::function<void(const std::string&)>& openProject,
		const std::function<bool(const std::string&)>& saveProject);

	std::string m_Directory;
	char m_DirectoryText[512] {};
	char m_FileName[256] {};
	bool m_ShowOpen = false;
	bool m_ShowSave = false;
	bool m_ShowOpenProject = false;
	bool m_ShowSaveProject = false;
	bool m_FocusFile = false;
};
