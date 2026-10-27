#include <Explorer.h>

#include <Lite/Assets/AssetRegistry.h>
#include <Lite/Assets/Texture.h>
#include <Lite/Core/IO/FileSystem.h>
#include <Lite/Project/Project.h>
#include <Lite/Scene/Console.h>

#include <imgui.h>
#include <imgui_impl_vulkan.h>

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
#include <cstdio>
#include <cstring>
#include <cwchar>
#include <fstream>
#include <format>
#include <string>
#include <unordered_map>
#include <vector>
#include <vulkan/vulkan.h>

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
		Material,
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
		if (extension == ".material")
			return IconKind::Material;
		return IconKind::File;
	}

	float ButtonWidth(const char* label)
	{
		return ImGui::CalcTextSize(label).x + ImGui::GetStyle().FramePadding.x * 2.0f;
	}

	void PlaceInline(float width)
	{
		const float spacing = ImGui::GetStyle().ItemSpacing.x;
		const float regionEnd = ImGui::GetWindowPos().x + ImGui::GetWindowContentRegionMax().x;
		if (ImGui::GetItemRectMax().x + spacing + width <= regionEnd)
			ImGui::SameLine(0.0f, spacing);
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
			case IconKind::Material:
				DrawFileIcon(draw, min, max, IM_COL32(214, 132, 92, 255));
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
		else if (kind == IconKind::Prefab)
			type = "LITE_PREFAB";
		else if (kind == IconKind::Material)
			type = "LITE_MATERIAL";
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

	float EaseHover(const std::string& key, bool hovered)
	{
		static std::unordered_map<std::string, float> amounts;
		float& amount = amounts[key];
		const float target = hovered ? 1.0f : 0.0f;
		const float step = std::min(1.0f, ImGui::GetIO().DeltaTime * 12.0f);
		amount += (target - amount) * step;
		return amount;
	}

	struct MaterialLook
	{
		std::filesystem::file_time_type Stamp {};
		ImVec4 Color { 1.0f, 1.0f, 1.0f, 1.0f };
		float Metallic = 0.0f;
		bool Ready = false;
		bool Found = false;
	};

	bool MaterialLookFor(const std::filesystem::path& path, ImVec4& color, float& metallic)
	{
		static std::unordered_map<std::string, MaterialLook> looks;
		const std::string key = path.generic_string();
		std::error_code error;
		const auto stamp = std::filesystem::last_write_time(path, error);
		MaterialLook& look = looks[key];
		if (!error && look.Ready && look.Stamp == stamp)
		{
			color = look.Color;
			metallic = look.Metallic;
			return look.Found;
		}

		look.Ready = true;
		look.Stamp = stamp;
		look.Found = false;
		look.Metallic = 0.0f;
		std::ifstream file(path);
		std::string line;
		while (std::getline(file, line))
		{
			const auto colorAt = line.find("color:");
			if (colorAt != std::string::npos)
			{
				const char* bracket = std::strchr(line.c_str() + colorAt, '[');
				float red = 1.0f;
				float green = 1.0f;
				float blue = 1.0f;
				float alpha = 1.0f;
				if (bracket != nullptr && std::sscanf(bracket + 1, " %f , %f , %f , %f", &red, &green, &blue, &alpha) >= 3)
				{
					look.Color = ImVec4(red, green, blue, alpha);
					look.Found = true;
				}
			}

			const auto metalAt = line.find("metallic:");
			if (metalAt != std::string::npos)
				std::sscanf(line.c_str() + metalAt + 9, " %f", &look.Metallic);
		}

		color = look.Color;
		metallic = look.Metallic;
		return look.Found;
	}

	struct Thumb
	{
		VkImageView View = VK_NULL_HANDLE;
		VkDescriptorSet Set = VK_NULL_HANDLE;
		std::filesystem::file_time_type Stamp {};
		int Frame = 0;
		bool Failed = false;
		bool Ready = false;
	};

	std::unordered_map<std::string, Thumb> g_Thumbs;
	int g_ThumbFrame = -1;

	void TrimThumbs()
	{
		const int frame = ImGui::GetFrameCount();
		if (g_ThumbFrame == frame)
			return;

		g_ThumbFrame = frame;
		for (auto thumb = g_Thumbs.begin(); thumb != g_Thumbs.end();)
		{
			if (thumb->second.Frame > 0 && thumb->second.Frame + 1 < frame)
			{
				if (thumb->second.Set != VK_NULL_HANDLE)
					ImGui_ImplVulkan_RemoveTexture(thumb->second.Set);
				thumb = g_Thumbs.erase(thumb);
			}
			else
			{
				++thumb;
			}
		}
	}

	VkDescriptorSet Thumbnail(const std::filesystem::path& path)
	{
		const std::string key = path.generic_string();
		const int frame = ImGui::GetFrameCount();
		std::error_code error;
		const auto stamp = std::filesystem::last_write_time(path, error);
		auto found = g_Thumbs.find(key);
		if (found == g_Thumbs.end() && g_Thumbs.size() >= 64)
			return VK_NULL_HANDLE;
		if (found == g_Thumbs.end())
			found = g_Thumbs.emplace(key, Thumb {}).first;

		Thumb& cached = found->second;
		cached.Frame = frame;
		if (cached.Failed && cached.Ready && cached.Stamp == stamp)
			return VK_NULL_HANDLE;

		Lite::Ref<Lite::Texture> texture = Lite::AssetRegistry::Get().Load<Lite::Texture>(path.string());
		if (!texture || texture->GetView() == VK_NULL_HANDLE)
		{
			cached.Failed = true;
			cached.Ready = true;
			cached.Stamp = stamp;
			return VK_NULL_HANDLE;
		}

		if (cached.View != texture->GetView() || cached.Set == VK_NULL_HANDLE)
		{
			if (cached.Set != VK_NULL_HANDLE)
				ImGui_ImplVulkan_RemoveTexture(cached.Set);
			cached.Set = ImGui_ImplVulkan_AddTexture(texture->GetView(), VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
			cached.View = texture->GetView();
		}

		cached.Failed = cached.Set == VK_NULL_HANDLE;
		cached.Ready = true;
		cached.Stamp = stamp;
		return cached.Set;
	}

	void DrawImage(ImDrawList* draw, ImVec2 min, ImVec2 max, VkDescriptorSet set)
	{
		const ImTextureID id = static_cast<ImTextureID>(reinterpret_cast<std::uintptr_t>(set));
		draw->AddImageRounded(id, min, max, ImVec2(0.0f, 0.0f), ImVec2(1.0f, 1.0f), IM_COL32_WHITE, 6.0f);
		draw->AddRect(min, max, IM_COL32(255, 255, 255, 40), 6.0f);
	}

	void DrawMaterialSwatch(ImDrawList* draw, ImVec2 min, ImVec2 max, const ImVec4& color, float metallic)
	{
		draw->AddRectFilled(min, max, ImGui::ColorConvertFloat4ToU32(color), 6.0f);
		draw->AddRect(min, max, IM_COL32(255, 255, 255, 48), 6.0f);
		if (metallic > 0.15f)
		{
			const float shine = std::min(1.0f, metallic);
			const ImVec2 shineMax { max.x - 4.0f, min.y + (max.y - min.y) * 0.42f };
			draw->AddRectFilled(
				ImVec2(min.x + 4.0f, min.y + 4.0f),
				shineMax,
				IM_COL32(255, 255, 255, static_cast<int>(36.0f + shine * 90.0f)),
				4.0f);
		}
	}

	void DrawFolderPeek(ImDrawList* draw, ImVec2 min, ImVec2 max, const std::filesystem::path& folder)
	{
		ImU32 chips[3] {};
		VkDescriptorSet images[3] {};
		int count = 0;
		int seen = 0;
		std::error_code error;
		for (const std::filesystem::directory_entry& entry : std::filesystem::directory_iterator(folder, error))
		{
			if (error || count >= 3 || seen >= 12)
				break;
			++seen;
			if (IsHidden(entry.path()) || IsCode(entry.path()))
				continue;

			std::error_code fileError;
			if (entry.is_directory(fileError))
				continue;

			const IconKind kind = KindFor(entry.path(), false);
			if (kind == IconKind::Material)
			{
				ImVec4 color { 0.84f, 0.55f, 0.36f, 1.0f };
				float metallic = 0.0f;
				MaterialLookFor(entry.path(), color, metallic);
				chips[count] = ImGui::ColorConvertFloat4ToU32(color);
				images[count] = VK_NULL_HANDLE;
				++count;
			}
			else if (kind == IconKind::Image)
			{
				chips[count] = IM_COL32(92, 176, 138, 255);
				images[count] = Thumbnail(entry.path());
				++count;
			}
		}

		if (count == 0)
			return;

		const float chip = 11.0f;
		const float gap = 3.0f;
		const float row = chip * static_cast<float>(count) + gap * static_cast<float>(count - 1);
		float x = min.x + ((max.x - min.x) - row) * 0.5f;
		const float y = max.y - chip - 5.0f;
		for (int index = 0; index < count; ++index)
		{
			const ImVec2 chipMin { x, y };
			const ImVec2 chipMax { x + chip, y + chip };
			if (images[index] != VK_NULL_HANDLE)
				DrawImage(draw, chipMin, chipMax, images[index]);
			else
				draw->AddRectFilled(chipMin, chipMax, chips[index], 2.0f);
			x += chip + gap;
		}
	}

	void DrawPreview(ImDrawList* draw, ImVec2 min, ImVec2 max, const std::filesystem::path& path, IconKind kind, ImU32 folderColor)
	{
		if (kind == IconKind::Material)
		{
			ImVec4 color { 0.84f, 0.52f, 0.36f, 1.0f };
			float metallic = 0.0f;
			if (MaterialLookFor(path, color, metallic))
			{
				DrawMaterialSwatch(draw, min, max, color, metallic);
				return;
			}
		}

		if (kind == IconKind::Image)
		{
			if (VkDescriptorSet set = Thumbnail(path))
			{
				DrawImage(draw, min, max, set);
				return;
			}
		}

		if (kind == IconKind::Folder)
		{
			DrawFolderIcon(draw, min, max, folderColor == 0 ? IM_COL32(214, 168, 72, 255) : folderColor);
			DrawFolderPeek(draw, min, max, path);
			return;
		}

		DrawIcon(draw, min, max, kind);
	}

	void DrawTileFrame(ImDrawList* draw, ImVec2 origin, ImVec2 size, float hover, bool selected)
	{
		const ImVec2 max { origin.x + size.x, origin.y + size.y };
		if (selected)
			draw->AddRectFilled(origin, max, IM_COL32(70, 110, 170, static_cast<int>(78.0f + hover * 40.0f)), 8.0f);
		else if (hover > 0.01f)
			draw->AddRectFilled(origin, max, IM_COL32(255, 255, 255, static_cast<int>(hover * 28.0f)), 8.0f);

		if (selected || hover > 0.02f)
		{
			const int alpha = selected ? static_cast<int>(150.0f + hover * 70.0f) : static_cast<int>(hover * 90.0f);
			draw->AddRect(origin, max, IM_COL32(126, 170, 224, alpha), 8.0f, 0, 1.0f + hover);
		}
	}

	void ScaleRect(ImVec2& min, ImVec2& max, float hover)
	{
		const float scale = 1.0f + hover * 0.08f;
		const ImVec2 center { (min.x + max.x) * 0.5f, (min.y + max.y) * 0.5f };
		const ImVec2 half { (max.x - min.x) * 0.5f * scale, (max.y - min.y) * 0.5f * scale };
		min = ImVec2(center.x - half.x, center.y - half.y);
		max = ImVec2(center.x + half.x, center.y + half.y);
	}

	const char* KindLabel(IconKind kind)
	{
		switch (kind)
		{
			case IconKind::Folder: return "Folder";
			case IconKind::Scene: return "Scene";
			case IconKind::Image: return "Texture";
			case IconKind::Script: return "Script";
			case IconKind::Prefab: return "Prefab";
			case IconKind::Material: return "Material";
			default: return "File";
		}
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

	return Lite::FileSystem::Contains(m_Project, path);
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
	const std::function<void(const std::string&)>& openProject,
	const std::function<void(const std::string&)>& openPrefab)
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

		if (ImGui::BeginPopupContextItem())
		{
			if (ImGui::MenuItem("Rename", nullptr, false, CanRename(child)))
				m_FocusRename = true;
			ImGui::EndPopup();
		}

		const float hover = EaseHover(child.generic_string(), hovered);
		ImDrawList* draw = ImGui::GetWindowDrawList();
		DrawTileFrame(draw, origin, ImVec2(tileWidth, tileHeight), hover, selected);

		ImVec2 iconMin { origin.x + 22.0f, origin.y + 8.0f };
		ImVec2 iconMax { origin.x + 62.0f, origin.y + 46.0f };
		ScaleRect(iconMin, iconMax, hover);
		DrawPreview(draw, iconMin, iconMax, child, kind, 0);

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
			else if (ExtensionOf(child) == ".prefab")
				openPrefab(child.string());
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

bool Explorer::CanRename(const std::filesystem::path& path) const
{
	if (!InsideProject(path) || path == m_Project)
		return false;

	Lite::Ref<Lite::Project> project = Lite::Project::GetActive();
	if (!project)
		return false;

	std::filesystem::path assetName = project->GetConfig().AssetDirectory.empty() ? std::filesystem::path("assets") : project->GetConfig().AssetDirectory;
	std::filesystem::path assets = assetName.is_absolute() ? assetName : m_Project / assetName;
	if (path == assets)
		return false;

	for (const char* folder : Lite::Project::ContentFolders)
	{
		if (path == assets / folder)
			return false;
	}

	return true;
}

void Explorer::ApplyRename()
{
	const std::string current = m_Selected.filename().string();
	std::string name = m_Rename;
	while (!name.empty() && (name.back() == ' ' || name.back() == '.'))
		name.pop_back();
	while (!name.empty() && name.front() == ' ')
		name.erase(name.begin());

	auto restore = [&]()
	{
		std::snprintf(m_Rename, sizeof(m_Rename), "%s", current.c_str());
	};

	if (name.empty() || name == current)
	{
		restore();
		return;
	}

	if (name.find_first_of("\\/:*?\"<>|") != std::string::npos)
	{
		Lite::Console::Log("Failed to rename: invalid name");
		restore();
		return;
	}

	const std::filesystem::path next = m_Selected.parent_path() / name;
	std::error_code error;
	if (std::filesystem::exists(next, error))
	{
		Lite::Console::Log(std::format("Failed to rename: {} already exists", name));
		restore();
		return;
	}

	std::filesystem::rename(m_Selected, next, error);
	if (error)
	{
		Lite::Console::Log(std::format("Failed to rename {}", current));
		restore();
		return;
	}

	if (Lite::FileSystem::Contains(m_Selected, m_View))
	{
		std::error_code relativeError;
		const std::filesystem::path relative = std::filesystem::relative(m_View, m_Selected, relativeError);
		if (!relativeError)
			m_View = relative.empty() ? next : next / relative;
	}

	m_Selected = next;
	m_RenamePath = next;
	Lite::Console::Log(std::format("Renamed {} to {}", current, name));
}

void Explorer::DrawDetails()
{
	std::error_code error;
	const bool directory = std::filesystem::is_directory(m_Selected, error);
	const IconKind kind = KindFor(m_Selected, directory);
	if (m_RenamePath != m_Selected && !m_RenameEditing)
	{
		m_RenamePath = m_Selected;
		const std::string name = m_Selected.filename().string();
		std::snprintf(m_Rename, sizeof(m_Rename), "%s", name.c_str());
	}

	ImGui::BeginChild("##ExplorerDetails", ImVec2(0.0f, 0.0f), ImGuiChildFlags_Borders);
	const ImVec2 origin = ImGui::GetCursorScreenPos();
	const ImVec2 previewMin { origin.x + 8.0f, origin.y + 10.0f };
	const ImVec2 previewMax { previewMin.x + 64.0f, previewMin.y + 64.0f };
	ImDrawList* draw = ImGui::GetWindowDrawList();
	draw->AddRectFilled(previewMin, previewMax, IM_COL32(18, 20, 26, 255), 8.0f);
	ImVec2 iconMin { previewMin.x + 8.0f, previewMin.y + 8.0f };
	ImVec2 iconMax { previewMax.x - 8.0f, previewMax.y - 8.0f };
	ImU32 folderColor = 0;
	if (kind == IconKind::Folder)
	{
		const std::string folder = LowerName(m_Selected);
		if (folder == "scenes")
			folderColor = IM_COL32(86, 156, 214, 255);
		else if (folder == "scripts")
			folderColor = IM_COL32(126, 146, 214, 255);
		else if (folder == "prefabs")
			folderColor = IM_COL32(176, 124, 214, 255);
		else if (folder == "materials")
			folderColor = IM_COL32(214, 140, 92, 255);
		else if (folder == "textures")
			folderColor = IM_COL32(92, 176, 138, 255);
	}
	DrawPreview(draw, iconMin, iconMax, m_Selected, kind, folderColor);
	ImGui::Dummy(ImVec2(80.0f, 80.0f));
	ImGui::SameLine();
	ImGui::BeginGroup();
	ImGui::TextDisabled("%s", KindLabel(kind));
	const bool editable = CanRename(m_Selected);
	if (m_FocusRename && editable)
		ImGui::SetKeyboardFocusHere();
	m_FocusRename = false;
	ImGui::SetNextItemWidth(220.0f);
	if (editable)
	{
		ImGui::InputText("##Rename", m_Rename, sizeof(m_Rename));
		m_RenameEditing = ImGui::IsItemActive();
		if (ImGui::IsItemDeactivatedAfterEdit())
			ApplyRename();
	}
	else
	{
		ImGui::BeginDisabled();
		ImGui::InputText("##RenameLocked", m_Rename, sizeof(m_Rename), ImGuiInputTextFlags_ReadOnly);
		ImGui::EndDisabled();
		m_RenameEditing = false;
	}
	ImGui::EndGroup();
	ImGui::EndChild();
}

void Explorer::Draw(
	const std::function<void(const std::string&)>& openScene,
	const std::function<void(const std::string&)>& openProject,
	const std::function<void(const std::string&)>& openPrefab)
{
	ImGui::Begin("Explorer");
	TrimThumbs();

	std::filesystem::path root = ProjectRoot();
	if (root.empty())
	{
		ImGui::TextDisabled("Open a project to browse its files");
		ImGui::End();
		return;
	}

	Lite::Ref<Lite::Project> project = Lite::Project::GetActive();
	Lite::Project::EnsureContentFolders();
	std::filesystem::path assetName = project->GetConfig().AssetDirectory.empty() ? std::filesystem::path("assets") : project->GetConfig().AssetDirectory;
	std::filesystem::path assets = assetName.is_absolute() ? assetName : root / assetName;

	struct Library
	{
		std::string Label;
		std::filesystem::path Path;
		ImU32 Color = 0;
	};

	std::vector<Library> libraries;
	libraries.push_back({ "Assets", assets, IM_COL32(214, 168, 72, 255) });
	for (const char* folder : Lite::Project::ContentFolders)
	{
		std::string label = folder;
		if (!label.empty() && label[0] >= 'a' && label[0] <= 'z')
			label[0] = static_cast<char>(label[0] - 'a' + 'A');

		ImU32 color = IM_COL32(150, 156, 166, 255);
		if (label == "Scenes")
			color = IM_COL32(86, 156, 214, 255);
		else if (label == "Scripts")
			color = IM_COL32(126, 146, 214, 255);
		else if (label == "Prefabs")
			color = IM_COL32(176, 124, 214, 255);
		else if (label == "Materials")
			color = IM_COL32(214, 140, 92, 255);
		else if (label == "Textures")
			color = IM_COL32(92, 176, 138, 255);

		libraries.push_back({ label, assets / folder, color });
	}
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
			for (const Library& library : libraries)
				atLibrary = atLibrary || m_View == library.Path;

			if (atLibrary)
				m_View.clear();
			else
				m_View = m_View.parent_path();
			m_Selected = m_View.empty() ? assets : m_View;
		}
	}
	PlaceInline(ButtonWidth("New Folder"));
	if (ImGui::SmallButton("New Folder"))
	{
		m_FolderName[0] = '\0';
		m_ShowNewFolder = true;
	}
	PlaceInline(ButtonWidth("Import"));
	if (ImGui::SmallButton("Import"))
		ImportFiles();

	ImGui::Spacing();
	if (m_View.empty())
	{
		ImGui::TextDisabled("Content");
	}
	else
	{
		std::filesystem::path libraryRoot = assets;
		std::string libraryLabel = "Assets";
		for (const Library& library : libraries)
		{
			if (!Lite::FileSystem::Contains(library.Path, m_View))
				continue;
			libraryRoot = library.Path;
			libraryLabel = library.Label;
			break;
		}

		if (ImGui::SmallButton(libraryLabel.c_str()))
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
				PlaceInline(ImGui::CalcTextSize("/").x);
				ImGui::TextDisabled("/");
				std::string crumb = part.string() + "##crumb" + walked.string();
				PlaceInline(ButtonWidth(part.string().c_str()));
				if (ImGui::SmallButton(crumb.c_str()))
				{
					m_View = walked;
					m_Selected = walked;
				}
			}
		}
	}

	const bool details = !m_Selected.empty() && InsideProject(m_Selected);
	const float footer = details ? 96.0f : 0.0f;
	ImGui::BeginChild("##ExplorerGrid", ImVec2(0.0f, footer > 0.0f ? -footer : 0.0f), ImGuiChildFlags_Borders);
	if (m_View.empty())
	{
		const float tileWidth = 84.0f;
		const float tileHeight = 92.0f;
		const float spacing = 8.0f;
		const float panelWidth = ImGui::GetContentRegionAvail().x;
		const int columns = std::max(1, static_cast<int>((panelWidth + spacing) / (tileWidth + spacing)));
		int column = 0;
		for (const Library& library : libraries)
		{
			if (column > 0)
				ImGui::SameLine(0.0f, spacing);

			ImGui::PushID(library.Label.c_str());
			ImVec2 origin = ImGui::GetCursorScreenPos();
			ImGui::InvisibleButton("##library", ImVec2(tileWidth, tileHeight));
			const bool hovered = ImGui::IsItemHovered();
			const bool selected = m_Selected == library.Path;
			if (ImGui::IsItemClicked())
				m_Selected = library.Path;

			if (ImGui::BeginPopupContextItem())
			{
				if (ImGui::MenuItem("Open"))
				{
					m_View = library.Path;
					m_Selected = library.Path;
				}
				ImGui::EndPopup();
			}

			const float hover = EaseHover(library.Path.generic_string(), hovered);
			ImDrawList* draw = ImGui::GetWindowDrawList();
			DrawTileFrame(draw, origin, ImVec2(tileWidth, tileHeight), hover, selected);

			ImVec2 iconMin { origin.x + 22.0f, origin.y + 8.0f };
			ImVec2 iconMax { origin.x + 62.0f, origin.y + 46.0f };
			ScaleRect(iconMin, iconMax, hover);
			DrawPreview(draw, iconMin, iconMax, library.Path, IconKind::Folder, library.Color);

			ImVec2 textSize = ImGui::CalcTextSize(library.Label.c_str());
			draw->AddText(ImVec2(origin.x + (tileWidth - textSize.x) * 0.5f, origin.y + 54.0f), IM_COL32(220, 224, 230, 255), library.Label.c_str());
			ImGui::PopID();

			if (hovered && ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left))
			{
				m_View = library.Path;
				m_Selected = library.Path;
			}

			column += 1;
			if (column >= columns)
				column = 0;
		}
	}
	else
	{
		DrawGrid(m_View, openScene, openProject, openPrefab);
	}
	ImGui::EndChild();
	if (details)
		DrawDetails();

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
