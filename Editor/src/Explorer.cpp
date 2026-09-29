#include <Explorer.h>

#include <Lite/Project/Project.h>
#include <Lite/Scene/Console.h>

#include <imgui.h>

#ifndef WIN32_LEAN_AND_MEAN
	#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
	#define NOMINMAX
#endif
#include <Windows.h>
#include <commdlg.h>

#include <algorithm>
#include <cfloat>
#include <cwchar>
#include <format>
#include <string>
#include <vector>

namespace {

	bool IsHidden(const std::filesystem::path& path)
	{
		const std::string name = path.filename().string();
		return !name.empty() && name[0] == '.';
	}

	std::string LowerName(const std::filesystem::path& path)
	{
		std::string name = path.filename().string();
		for (char& character : name)
		{
			if (character >= 'A' && character <= 'Z')
				character = static_cast<char>(character - 'A' + 'a');
		}
		return name;
	}

	std::string ExtensionOf(const std::filesystem::path& path)
	{
		std::string extension = path.extension().string();
		for (char& character : extension)
		{
			if (character >= 'A' && character <= 'Z')
				character = static_cast<char>(character - 'A' + 'a');
		}
		return extension;
	}

	enum class IconKind
	{
		Folder,
		Scene,
		Image,
		Script,
		Prefab,
		File
	};

	IconKind KindFor(const std::filesystem::path& path, bool directory)
	{
		if (directory)
			return IconKind::Folder;

		const std::string extension = ExtensionOf(path);
		if (extension == ".scene")
			return IconKind::Scene;
		if (extension == ".png" || extension == ".jpg" || extension == ".jpeg")
			return IconKind::Image;
		if (extension == ".cs" || extension == ".lua" || extension == ".js")
			return IconKind::Script;
		if (extension == ".prefab")
			return IconKind::Prefab;
		return IconKind::File;
	}

	void DrawFolderIcon(ImDrawList* draw, ImVec2 min, ImVec2 max, ImU32 color)
	{
		float width = max.x - min.x;
		float height = max.y - min.y;
		ImVec2 tabMin { min.x, min.y + height * 0.16f };
		ImVec2 tabMax { min.x + width * 0.46f, min.y + height * 0.40f };
		draw->AddRectFilled(tabMin, tabMax, color, 3.0f);
		ImVec2 bodyMin { min.x, min.y + height * 0.32f };
		draw->AddRectFilled(bodyMin, max, color, 4.0f);
		ImU32 lip = IM_COL32(255, 255, 255, 48);
		draw->AddRectFilled(
			ImVec2(bodyMin.x + 2.0f, bodyMin.y + 3.0f),
			ImVec2(max.x - 2.0f, bodyMin.y + height * 0.22f),
			lip,
			2.0f);
	}

	void DrawFileIcon(ImDrawList* draw, ImVec2 min, ImVec2 max, ImU32 color)
	{
		float width = max.x - min.x;
		float fold = width * 0.32f;
		draw->AddRectFilled(min, max, color, 3.0f);
		draw->AddTriangleFilled(
			ImVec2(max.x - fold, min.y),
			ImVec2(max.x, min.y + fold),
			ImVec2(max.x - fold, min.y + fold),
			IM_COL32(255, 255, 255, 70));
		ImU32 line = IM_COL32(255, 255, 255, 150);
		float left = min.x + width * 0.18f;
		float right = max.x - width * 0.18f;
		float top = min.y + (max.y - min.y) * 0.42f;
		for (int row = 0; row < 3; ++row)
		{
			float y = top + row * 5.0f;
			draw->AddLine(ImVec2(left, y), ImVec2(right, y), line, 1.4f);
		}
	}

	void DrawIcon(ImDrawList* draw, ImVec2 min, ImVec2 max, IconKind kind)
	{
		switch (kind)
		{
			case IconKind::Folder:
				DrawFolderIcon(draw, min, max, IM_COL32(214, 168, 72, 255));
				break;
			case IconKind::Scene:
				DrawFileIcon(draw, min, max, IM_COL32(86, 156, 214, 255));
				break;
			case IconKind::Image:
				DrawFileIcon(draw, min, max, IM_COL32(92, 176, 138, 255));
				break;
			case IconKind::Script:
				DrawFileIcon(draw, min, max, IM_COL32(126, 146, 214, 255));
				break;
			case IconKind::Prefab:
				DrawFileIcon(draw, min, max, IM_COL32(176, 124, 214, 255));
				break;
			default:
				DrawFileIcon(draw, min, max, IM_COL32(150, 156, 166, 255));
				break;
		}
	}

	std::string DragPath(const std::filesystem::path& path)
	{
		std::error_code error;
		std::filesystem::path canonical = std::filesystem::weakly_canonical(path, error);
		if (error)
			canonical = path;
		return canonical.generic_string();
	}

	void BeginFileDrag(const std::filesystem::path& path, IconKind kind, const std::string& label)
	{
		const char* type = nullptr;
		if (kind == IconKind::Image)
			type = "LITE_TEXTURE";
		else if (kind == IconKind::Scene)
			type = "LITE_SCENE";
		if (type == nullptr || !ImGui::BeginDragDropSource())
			return;

		const std::string payload = DragPath(path);
		ImGui::SetDragDropPayload(type, payload.c_str(), payload.size() + 1);
		ImGui::TextUnformatted(label.c_str());
		ImGui::EndDragDropSource();
	}

	std::string FitLabel(const std::string& text, float width)
	{
		ImFont* font = ImGui::GetFont();
		float size = ImGui::GetFontSize();
		if (font->CalcTextSizeA(size, FLT_MAX, 0.0f, text.c_str()).x <= width)
			return text;

		std::string fitted = text;
		while (fitted.size() > 1 && font->CalcTextSizeA(size, FLT_MAX, 0.0f, (fitted + "...").c_str()).x > width)
			fitted.pop_back();
		return fitted + "...";
	}

	bool IsCode(const std::filesystem::path& path)
	{
		const std::string name = LowerName(path);
		if (name == "cmakelists.txt" || name == "shaders" || name == "src")
			return true;

		const std::string extension = ExtensionOf(path);
		return extension == ".cpp"
			|| extension == ".h"
			|| extension == ".hpp"
			|| extension == ".c"
			|| extension == ".cxx"
			|| extension == ".vert"
			|| extension == ".frag"
			|| extension == ".comp"
			|| extension == ".glsl"
			|| extension == ".hlsl"
			|| extension == ".cmake"
			|| extension == ".inl";
	}

}

std::filesystem::path Explorer::ProjectRoot() const
{
	Lite::Ref<Lite::Project> project = Lite::Project::GetActive();
	if (!project || project->GetProjectDirectory().empty())
		return {};

	return project->GetProjectDirectory();
}

bool Explorer::InsideProject(const std::filesystem::path& path) const
{
	if (m_Project.empty() || path.empty())
		return false;

	std::error_code error;
	std::filesystem::path relative = std::filesystem::relative(path, m_Project, error);
	return !error && (relative.empty() || relative.begin()->string() != "..");
}

std::filesystem::path Explorer::Destination() const
{
	std::filesystem::path fallback = m_Project;
	if (Lite::Ref<Lite::Project> project = Lite::Project::GetActive())
	{
		std::filesystem::path assetName = project->GetConfig().AssetDirectory.empty() ? std::filesystem::path("assets") : project->GetConfig().AssetDirectory;
		fallback = assetName.is_absolute() ? assetName : m_Project / assetName;
	}

	if (!m_View.empty() && InsideProject(m_View))
		return m_View;

	if (m_Selected.empty() || !InsideProject(m_Selected))
		return fallback;

	std::error_code error;
	if (std::filesystem::is_directory(m_Selected, error))
		return m_Selected;

	std::filesystem::path parent = m_Selected.parent_path();
	return parent.empty() ? m_Project : parent;
}

void Explorer::DrawGrid(
	const std::filesystem::path& directory,
	const std::function<void(const std::string&)>& openScene,
	const std::function<void(const std::string&)>& openProject)
{
	std::error_code error;
	std::vector<std::filesystem::directory_entry> entries;
	for (const std::filesystem::directory_entry& entry : std::filesystem::directory_iterator(directory, error))
	{
		if (error || IsHidden(entry.path()) || IsCode(entry.path()))
			continue;
		entries.push_back(entry);
	}

	std::sort(entries.begin(), entries.end(), [](const std::filesystem::directory_entry& left, const std::filesystem::directory_entry& right)
	{
		std::error_code leftError;
		std::error_code rightError;
		bool leftDirectory = left.is_directory(leftError);
		bool rightDirectory = right.is_directory(rightError);
		if (leftDirectory != rightDirectory)
			return leftDirectory;
		return left.path().filename() < right.path().filename();
	});

	const float tileWidth = 84.0f;
	const float tileHeight = 92.0f;
	const float spacing = 8.0f;
	const float panelWidth = ImGui::GetContentRegionAvail().x;
	const int columns = std::max(1, static_cast<int>((panelWidth + spacing) / (tileWidth + spacing)));
	int column = 0;

	if (entries.empty())
	{
		ImGui::TextDisabled("Empty folder");
		return;
	}

	for (const std::filesystem::directory_entry& entry : entries)
	{
		const std::filesystem::path& child = entry.path();
		std::error_code fileError;
		const bool directoryEntry = entry.is_directory(fileError);
		const IconKind kind = KindFor(child, directoryEntry);
		const std::string name = child.filename().string();
		if (column > 0)
			ImGui::SameLine(0.0f, spacing);

		ImGui::PushID(child.string().c_str());
		ImVec2 origin = ImGui::GetCursorScreenPos();
		ImGui::InvisibleButton("##tile", ImVec2(tileWidth, tileHeight));
		const bool hovered = ImGui::IsItemHovered();
		const bool selected = m_Selected == child;
		if (ImGui::IsItemClicked())
			m_Selected = child;
		if (!directoryEntry)
			BeginFileDrag(child, kind, name);

		ImDrawList* draw = ImGui::GetWindowDrawList();
		ImU32 background = 0;
		if (selected)
			background = IM_COL32(70, 110, 170, 90);
		else if (hovered)
			background = IM_COL32(255, 255, 255, 18);
		if (background != 0)
			draw->AddRectFilled(origin, ImVec2(origin.x + tileWidth, origin.y + tileHeight), background, 6.0f);

		ImVec2 iconMin { origin.x + 22.0f, origin.y + 8.0f };
		ImVec2 iconMax { origin.x + 62.0f, origin.y + 46.0f };
		DrawIcon(draw, iconMin, iconMax, kind);

		const std::string label = FitLabel(name, tileWidth - 8.0f);
		ImVec2 textSize = ImGui::CalcTextSize(label.c_str());
		float textX = origin.x + (tileWidth - textSize.x) * 0.5f;
		draw->AddText(ImVec2(textX, origin.y + 54.0f), IM_COL32(220, 224, 230, 255), label.c_str());
		ImGui::PopID();

		if (hovered && ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left))
		{
			if (directoryEntry)
			{
				m_View = child;
				m_Selected = child;
			}
			else if (ExtensionOf(child) == ".scene")
				openScene(child.string());
			else if (ExtensionOf(child) == ".lite")
				openProject(child.string());
		}

		column += 1;
		if (column >= columns)
			column = 0;
	}
}

void Explorer::CreateFolder()
{
	std::string name = m_FolderName;
	if (name.empty() || name.find_first_of("\\/:*?\"<>|") != std::string::npos)
	{
		Lite::Console::Log("Failed to create folder: invalid name");
		return;
	}

	std::filesystem::path folder = Destination() / name;
	std::error_code error;
	if (std::filesystem::exists(folder, error))
	{
		Lite::Console::Log(std::format("Failed to create folder: {} already exists", folder.string()));
		return;
	}

	std::filesystem::create_directory(folder, error);
	if (error)
	{
		Lite::Console::Log(std::format("Failed to create folder: {}", folder.string()));
		return;
	}

	m_Selected = folder;
	Lite::Console::Log(std::format("Folder created: {}", folder.string()));
}

void Explorer::ImportFiles()
{
	wchar_t buffer[8192] {};
	OPENFILENAMEW dialog {};
	dialog.lStructSize = sizeof(dialog);
	dialog.lpstrFile = buffer;
	dialog.nMaxFile = static_cast<DWORD>(sizeof(buffer) / sizeof(buffer[0]));
	dialog.lpstrFilter = L"All files\0*.*\0Scenes\0*.scene\0Images\0*.png;*.jpg;*.jpeg\0";
	dialog.nFilterIndex = 1;
	dialog.Flags = OFN_EXPLORER | OFN_FILEMUSTEXIST | OFN_ALLOWMULTISELECT | OFN_PATHMUSTEXIST | OFN_NOCHANGEDIR;
	if (GetOpenFileNameW(&dialog) == FALSE)
		return;

	std::vector<std::filesystem::path> chosen;
	const wchar_t* cursor = buffer;
	std::filesystem::path first(cursor);
	cursor += std::wcslen(cursor) + 1;
	if (*cursor == L'\0')
	{
		chosen.push_back(first);
	}
	else
	{
		while (*cursor != L'\0')
		{
			chosen.push_back(first / cursor);
			cursor += std::wcslen(cursor) + 1;
		}
	}

	std::filesystem::path destination = Destination();
	for (const std::filesystem::path& source : chosen)
	{
		if (IsCode(source))
		{
			Lite::Console::Log(std::format("Failed to import {}: the explorer holds project content", source.filename().string()));
			continue;
		}

		std::filesystem::path target = destination / source.filename();
		std::error_code error;
		if (std::filesystem::exists(target, error))
		{
			Lite::Console::Log(std::format("Failed to import {}: already in the project", source.filename().string()));
			continue;
		}

		std::filesystem::copy_file(source, target, error);
		if (error)
		{
			Lite::Console::Log(std::format("Failed to import {}", source.filename().string()));
			continue;
		}

		m_Selected = target;
		Lite::Console::Log(std::format("Imported {} into {}", source.filename().string(), destination.string()));
	}
}

void Explorer::Draw(
	const std::function<void(const std::string&)>& openScene,
	const std::function<void(const std::string&)>& openProject)
{
	ImGui::Begin("Explorer");

	std::filesystem::path root = ProjectRoot();
	if (root.empty())
	{
		ImGui::TextDisabled("Open a project to browse its files");
		ImGui::End();
		return;
	}

	Lite::Ref<Lite::Project> project = Lite::Project::GetActive();
	std::filesystem::path assetName = project->GetConfig().AssetDirectory.empty() ? std::filesystem::path("assets") : project->GetConfig().AssetDirectory;
	std::filesystem::path assets = assetName.is_absolute() ? assetName : root / assetName;
	std::filesystem::path scripts = root / "scripts";
	std::filesystem::path prefabs = root / "prefabs";
	std::error_code error;
	std::filesystem::create_directories(assets, error);
	std::filesystem::create_directories(scripts, error);
	std::filesystem::create_directories(prefabs, error);

	const std::filesystem::path libraries[] = { assets, scripts, prefabs };
	const char* labels[] = { "Assets", "Scripts", "Prefabs" };
	if (root != m_Project)
	{
		m_Project = root;
		m_View.clear();
		m_Selected = assets;
	}

	std::error_code viewError;
	if (!m_View.empty() && !std::filesystem::is_directory(m_View, viewError))
		m_View.clear();

	if (ImGui::SmallButton("Up"))
	{
		if (!m_View.empty())
		{
			bool atLibrary = false;
			for (const std::filesystem::path& library : libraries)
				atLibrary = atLibrary || m_View == library;

			if (atLibrary)
				m_View.clear();
			else
				m_View = m_View.parent_path();
			m_Selected = m_View.empty() ? assets : m_View;
		}
	}
	ImGui::SameLine();
	if (ImGui::SmallButton("New Folder"))
	{
		m_FolderName[0] = '\0';
		m_ShowNewFolder = true;
	}
	ImGui::SameLine();
	if (ImGui::SmallButton("Import"))
		ImportFiles();

	ImGui::SameLine();
	if (m_View.empty())
	{
		ImGui::TextDisabled("Content");
	}
	else
	{
		std::filesystem::path libraryRoot = assets;
		const char* libraryLabel = "Assets";
		for (int index = 0; index < 3; ++index)
		{
			std::error_code relativeError;
			std::filesystem::path relative = std::filesystem::relative(m_View, libraries[index], relativeError);
			bool inside = !relativeError && (relative.empty() || relative.begin()->string() != "..");
			if (!inside)
				continue;
			libraryRoot = libraries[index];
			libraryLabel = labels[index];
			break;
		}

		if (ImGui::SmallButton(libraryLabel))
		{
			m_View = libraryRoot;
			m_Selected = libraryRoot;
		}

		std::filesystem::path walked = libraryRoot;
		std::error_code relativeError;
		std::filesystem::path relative = std::filesystem::relative(m_View, libraryRoot, relativeError);
		if (!relativeError)
		{
			for (const std::filesystem::path& part : relative)
			{
				if (part.empty() || part == ".")
					continue;
				walked /= part;
				ImGui::SameLine(0.0f, 4.0f);
				ImGui::TextDisabled("/");
				ImGui::SameLine(0.0f, 4.0f);
				std::string crumb = part.string() + "##crumb" + walked.string();
				if (ImGui::SmallButton(crumb.c_str()))
				{
					m_View = walked;
					m_Selected = walked;
				}
			}
		}
	}

	ImGui::BeginChild("##ExplorerGrid", ImVec2(0.0f, 0.0f), ImGuiChildFlags_Borders);
	if (m_View.empty())
	{
		const float tileWidth = 84.0f;
		const float tileHeight = 92.0f;
		const float spacing = 8.0f;
		const float panelWidth = ImGui::GetContentRegionAvail().x;
		const int columns = std::max(1, static_cast<int>((panelWidth + spacing) / (tileWidth + spacing)));
		int column = 0;
		for (int index = 0; index < 3; ++index)
		{
			if (column > 0)
				ImGui::SameLine(0.0f, spacing);

			ImGui::PushID(labels[index]);
			ImVec2 origin = ImGui::GetCursorScreenPos();
			ImGui::InvisibleButton("##library", ImVec2(tileWidth, tileHeight));
			const bool hovered = ImGui::IsItemHovered();
			const bool selected = m_Selected == libraries[index];
			if (ImGui::IsItemClicked())
				m_Selected = libraries[index];

			ImDrawList* draw = ImGui::GetWindowDrawList();
			ImU32 background = 0;
			if (selected)
				background = IM_COL32(70, 110, 170, 90);
			else if (hovered)
				background = IM_COL32(255, 255, 255, 18);
			if (background != 0)
				draw->AddRectFilled(origin, ImVec2(origin.x + tileWidth, origin.y + tileHeight), background, 6.0f);

			ImU32 folderColor = IM_COL32(214, 168, 72, 255);
			if (index == 1)
				folderColor = IM_COL32(126, 146, 214, 255);
			else if (index == 2)
				folderColor = IM_COL32(176, 124, 214, 255);
			DrawFolderIcon(draw, ImVec2(origin.x + 22.0f, origin.y + 8.0f), ImVec2(origin.x + 62.0f, origin.y + 46.0f), folderColor);

			ImVec2 textSize = ImGui::CalcTextSize(labels[index]);
			draw->AddText(ImVec2(origin.x + (tileWidth - textSize.x) * 0.5f, origin.y + 54.0f), IM_COL32(220, 224, 230, 255), labels[index]);
			ImGui::PopID();

			if (hovered && ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left))
			{
				m_View = libraries[index];
				m_Selected = libraries[index];
			}

			column += 1;
			if (column >= columns)
				column = 0;
		}
	}
	else
	{
		DrawGrid(m_View, openScene, openProject);
	}
	ImGui::EndChild();

	if (m_ShowNewFolder)
	{
		ImGui::OpenPopup("New Folder");
		m_ShowNewFolder = false;
	}

	ImGui::SetNextWindowSize(ImVec2(320.0f, 0.0f), ImGuiCond_Appearing);
	if (ImGui::BeginPopupModal("New Folder", nullptr, ImGuiWindowFlags_AlwaysAutoResize))
	{
		ImGui::TextUnformatted("Folder name");
		ImGui::SetNextItemWidth(-1.0f);
		bool confirm = ImGui::InputText("##FolderName", m_FolderName, sizeof(m_FolderName), ImGuiInputTextFlags_EnterReturnsTrue);
		if (ImGui::Button("Create", ImVec2(96.0f, 0.0f)) || confirm)
		{
			CreateFolder();
			ImGui::CloseCurrentPopup();
		}
		ImGui::SameLine();
		if (ImGui::Button("Cancel", ImVec2(96.0f, 0.0f)))
			ImGui::CloseCurrentPopup();
		ImGui::EndPopup();
	}

	ImGui::End();
}
