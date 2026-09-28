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
	void Draw(Lite::Scene* scene, const std::function<void(const std::string&)>& openScene, const std::function<void()>& onSaved);

private:
	void Prepare(Lite::Scene* scene);
	void ApplyDirectoryText();
	std::string Selection() const;
	void DrawFiles(bool save, Lite::Scene* scene, const std::function<void(const std::string&)>& openScene, const std::function<void()>& onSaved);

	std::string m_Directory;
	char m_DirectoryText[512] {};
	char m_FileName[256] {};
	bool m_ShowOpen = false;
	bool m_ShowSave = false;
	bool m_FocusFile = false;
};
