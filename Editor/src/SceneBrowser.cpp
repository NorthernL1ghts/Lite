#include <SceneBrowser.h>

#include <Lite/Core/IO/FileSystem.h>
#include <Lite/Scene/Scene.h>

#include <imgui.h>

#include <algorithm>
#include <cstdio>
#include <filesystem>
#include <string>
#include <vector>

void SceneBrowser::ShowOpen()
{
	m_ShowOpen = true;
}

void SceneBrowser::ShowSave()
{
	m_ShowSave = true;
}

void SceneBrowser::Prepare(Lite::Scene* scene)
{
	std::filesystem::path directory;
	std::string fileName = scene != nullptr ? scene->GetName() : "Untitled";
	if (scene != nullptr && !scene->GetPath().empty())
	{
		std::filesystem::path current(scene->GetPath());
		if (current.has_parent_path())
			directory = current.parent_path();
		if (!current.stem().empty())
			fileName = current.stem().string();
	}

	if (directory.empty())
	{
		std::string scenes = Lite::Scene::Locate("assets/scenes");
		if (!scenes.empty())
			directory = scenes;
	}

	if (directory.empty())
		directory = Lite::FileSystem::ExecutableDirectory() / "assets" / "scenes";

	std::error_code error;
	if (!std::filesystem::is_directory(directory, error))
		directory = Lite::FileSystem::ExecutableDirectory();

	m_Directory = directory.string();
	std::snprintf(m_DirectoryText, sizeof(m_DirectoryText), "%s", m_Directory.c_str());
	std::snprintf(m_FileName, sizeof(m_FileName), "%s", fileName.c_str());
	m_FocusFile = true;
}

void SceneBrowser::ApplyDirectoryText()
{
	std::filesystem::path typed(m_DirectoryText);
	if (typed.empty())
	{
		m_Directory.clear();
		return;
	}

	std::error_code error;
	if (std::filesystem::is_directory(typed, error))
	{
		m_Directory = std::filesystem::absolute(typed, error).string();
		std::snprintf(m_DirectoryText, sizeof(m_DirectoryText), "%s", m_Directory.c_str());
		return;
	}

	if (std::filesystem::is_regular_file(typed, error))
	{
		m_Directory = std::filesystem::absolute(typed.parent_path(), error).string();
		std::snprintf(m_DirectoryText, sizeof(m_DirectoryText), "%s", m_Directory.c_str());
		std::snprintf(m_FileName, sizeof(m_FileName), "%s", typed.stem().string().c_str());
	}
}

std::string SceneBrowser::Selection() const
{
	std::filesystem::path entered(m_FileName);
	if (entered.empty())
		return {};

	if (!entered.is_absolute())
	{
		if (m_Directory.empty())
			return {};
		entered = std::filesystem::path(m_Directory) / entered;
	}

	if (entered.extension().empty())
		entered.replace_extension(".scene");

	return entered.lexically_normal().string();
}

void SceneBrowser::DrawFiles(bool save, Lite::Scene* scene, const std::function<void(const std::string&)>& openScene, const std::function<void()>& onSaved)
{
	ImGuiInputTextFlags directoryFlags = ImGuiInputTextFlags_EnterReturnsTrue;
	ImGui::SetNextItemWidth(-1.0f);
	if (ImGui::InputText("##Directory", m_DirectoryText, sizeof(m_DirectoryText), directoryFlags) || ImGui::IsItemDeactivatedAfterEdit())
		ApplyDirectoryText();

	ImGui::BeginChild(save ? "##SaveFiles" : "##OpenFiles", ImVec2(0.0f, -ImGui::GetFrameHeightWithSpacing() * 2.0f), ImGuiChildFlags_Borders);

	if (m_Directory.empty())
	{
		for (char letter = 'A'; letter <= 'Z'; ++letter)
		{
			std::string root = std::string(1, letter) + ":\\";
			std::error_code error;
			if (!std::filesystem::exists(root, error))
				continue;
			if (ImGui::Selectable(root.c_str()))
			{
				m_Directory = root;
				std::snprintf(m_DirectoryText, sizeof(m_DirectoryText), "%s", m_Directory.c_str());
			}
		}
	}
	else
	{
		std::filesystem::path current(m_Directory);
		if (ImGui::Selectable(".."))
		{
			std::filesystem::path parent = current.parent_path();
			if (parent.empty() || parent == current)
				m_Directory.clear();
			else
				m_Directory = parent.string();
			std::snprintf(m_DirectoryText, sizeof(m_DirectoryText), "%s", m_Directory.c_str());
		}

		struct BrowserEntry
		{
			std::string Label;
			std::filesystem::path Path;
			bool Directory = false;
		};

		std::vector<BrowserEntry> entries;
		std::error_code error;
		for (const std::filesystem::directory_entry& entry : std::filesystem::directory_iterator(current, error))
		{
			std::error_code fileError;
			bool directory = entry.is_directory(fileError);
			if (fileError)
				continue;
			if (!directory && entry.path().extension() != ".scene")
				continue;

			BrowserEntry item;
			item.Directory = directory;
			item.Path = entry.path();
			item.Label = directory ? entry.path().filename().string() + "/" : entry.path().filename().string();
			entries.push_back(std::move(item));
		}

		std::sort(entries.begin(), entries.end(), [](const BrowserEntry& left, const BrowserEntry& right)
		{
			if (left.Directory != right.Directory)
				return left.Directory;
			return left.Label < right.Label;
		});

		for (const BrowserEntry& entry : entries)
		{
			bool selected = !entry.Directory && entry.Path.stem().string() == m_FileName;
			if (!ImGui::Selectable(entry.Label.c_str(), selected, ImGuiSelectableFlags_AllowDoubleClick))
				continue;

			if (entry.Directory)
			{
				m_Directory = entry.Path.string();
				std::snprintf(m_DirectoryText, sizeof(m_DirectoryText), "%s", m_Directory.c_str());
				continue;
			}

			std::snprintf(m_FileName, sizeof(m_FileName), "%s", entry.Path.stem().string().c_str());
			if (!save && ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left))
			{
				openScene(entry.Path.string());
				ImGui::CloseCurrentPopup();
			}
		}
	}

	ImGui::EndChild();

	ImGui::AlignTextToFramePadding();
	ImGui::TextUnformatted("Name");
	ImGui::SameLine();
	if (m_FocusFile)
		ImGui::SetKeyboardFocusHere();
	ImGui::SetNextItemWidth(-1.0f);
	bool confirm = ImGui::InputText("##FileName", m_FileName, sizeof(m_FileName), ImGuiInputTextFlags_EnterReturnsTrue);
	m_FocusFile = false;

	const char* action = save ? "Save" : "Open";
	if (ImGui::Button(action, ImVec2(96.0f, 0.0f)) || confirm)
	{
		std::string path = Selection();
		if (!path.empty())
		{
			if (save && scene != nullptr && scene->SaveAs(path))
			{
				onSaved();
				ImGui::CloseCurrentPopup();
			}
			else if (!save)
			{
				openScene(path);
				ImGui::CloseCurrentPopup();
			}
		}
	}
	ImGui::SameLine();
	if (ImGui::Button("Cancel", ImVec2(96.0f, 0.0f)))
		ImGui::CloseCurrentPopup();
}

void SceneBrowser::Draw(Lite::Scene* scene, const std::function<void(const std::string&)>& openScene, const std::function<void()>& onSaved)
{
	if (m_ShowOpen)
	{
		Prepare(scene);
		ImGui::OpenPopup("Open Scene");
		m_ShowOpen = false;
	}

	ImGui::SetNextWindowSize(ImVec2(560.0f, 460.0f), ImGuiCond_Appearing);
	if (ImGui::BeginPopupModal("Open Scene", nullptr, ImGuiWindowFlags_NoResize))
	{
		DrawFiles(false, scene, openScene, onSaved);
		ImGui::EndPopup();
	}

	if (m_ShowSave)
	{
		Prepare(scene);
		ImGui::OpenPopup("Save Scene");
		m_ShowSave = false;
	}

	ImGui::SetNextWindowSize(ImVec2(560.0f, 460.0f), ImGuiCond_Appearing);
	if (ImGui::BeginPopupModal("Save Scene", nullptr, ImGuiWindowFlags_NoResize))
	{
		DrawFiles(true, scene, openScene, onSaved);
		ImGui::EndPopup();
	}
}
