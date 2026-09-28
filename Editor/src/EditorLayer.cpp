#include "EditorLayer.h"

#include "Lite/Core/Events/KeyEvent.h"
#include "Lite/Core/Events/MouseEvent.h"
#include "Lite/Assets/AssetRegistry.h"
#include "Lite/Assets/Texture.h"
#include "Lite/Core/FileSystem.h"
#include "Lite/Core/Logger.h"
#include "Lite/Core/Profiler.h"
#include "Lite/Core/Time.h"
#include "Lite/Input/Input.h"
#include "Lite/Input/KeyCodes.h"
#include "Lite/Renderer/Renderer.h"
#include "Lite/Renderer/Renderer2D.h"
#include "Lite/Scene/Console.h"
#include "Lite/Scene/Scene.h"

#include <imgui.h>
#include <imgui_internal.h>

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <filesystem>
#include <format>
#include <string>
#include <string_view>
#include <vector>

namespace {

	const ImVec4 kLabelColor { 0.62f, 0.68f, 0.76f, 1.0f };

	const char* kEditorWindows[] = { "Scene", "Viewport", "Inspector", "Console" };

	bool EditorDockNeedsBuild(ImGuiID dockspaceId)
	{
		ImGuiDockNode* node = ImGui::DockBuilderGetNode(dockspaceId);
		if (node == nullptr || !node->IsSplitNode())
			return true;

		for (const char* name : kEditorWindows)
		{
			ImGuiWindow* window = ImGui::FindWindowByName(name);
			if (window == nullptr || window->DockNode == nullptr)
				continue;

			ImGuiDockNode* root = ImGui::DockNodeGetRootNode(window->DockNode);
			if (root == nullptr || root->ID != dockspaceId)
				return true;
		}

		return false;
	}

	void BuildEditorDock(ImGuiID dockspaceId, ImVec2 size)
	{
		ImGui::DockBuilderRemoveNode(dockspaceId);
		ImGuiDockNodeFlags nodeFlags = static_cast<ImGuiDockNodeFlags>(
			static_cast<int>(ImGuiDockNodeFlags_DockSpace) | static_cast<int>(ImGuiDockNodeFlags_PassthruCentralNode));
		ImGui::DockBuilderAddNode(dockspaceId, nodeFlags);
		ImGui::DockBuilderSetNodeSize(dockspaceId, size);

		ImGuiID center = dockspaceId;
		ImGuiID left = ImGui::DockBuilderSplitNode(center, ImGuiDir_Left, 0.22f, nullptr, &center);
		ImGuiID right = ImGui::DockBuilderSplitNode(center, ImGuiDir_Right, 0.26f, nullptr, &center);
		ImGuiID bottom = ImGui::DockBuilderSplitNode(center, ImGuiDir_Down, 0.22f, nullptr, &center);

		ImGui::DockBuilderDockWindow("Scene", left);
		ImGui::DockBuilderDockWindow("Viewport", center);
		ImGui::DockBuilderDockWindow("Inspector", right);
		ImGui::DockBuilderDockWindow("Console", bottom);
		ImGui::DockBuilderFinish(dockspaceId);
	}

	void BeginDocked(const char* beside, const char* name, bool place)
	{
		if (place)
		{
			ImGuiWindow* host = ImGui::FindWindowByName(beside);
			if (host != nullptr && host->DockId != 0)
				ImGui::SetNextWindowDockID(host->DockId, ImGuiCond_Always);
		}

		ImGui::Begin(name);
	}

	bool BeginRows(const char* id)
	{
		if (!ImGui::BeginTable(id, 2, ImGuiTableFlags_RowBg | ImGuiTableFlags_PadOuterX | ImGuiTableFlags_SizingStretchProp))
			return false;

		ImGui::TableSetupColumn("Field", ImGuiTableColumnFlags_WidthStretch, 0.46f);
		ImGui::TableSetupColumn("Value", ImGuiTableColumnFlags_WidthStretch, 0.54f);
		return true;
	}

	void Row(const char* label, std::string_view value)
	{
		ImGui::TableNextRow();
		ImGui::TableSetColumnIndex(0);
		ImGui::PushStyleColor(ImGuiCol_Text, kLabelColor);
		ImGui::TextUnformatted(label);
		ImGui::PopStyleColor();
		ImGui::TableSetColumnIndex(1);
		ImGui::TextUnformatted(value.data(), value.data() + value.size());
	}

	const char* FormatName(VkFormat format)
	{
		switch (format)
		{
		case VK_FORMAT_B8G8R8A8_SRGB: return "B8G8R8A8 sRGB";
		case VK_FORMAT_R8G8B8A8_SRGB: return "R8G8B8A8 sRGB";
		case VK_FORMAT_B8G8R8A8_UNORM: return "B8G8R8A8 UNORM";
		case VK_FORMAT_R8G8B8A8_UNORM: return "R8G8B8A8 UNORM";
		default: return "Other";
		}
	}

	const char* ColorSpaceName(VkColorSpaceKHR space)
	{
		switch (space)
		{
		case VK_COLOR_SPACE_SRGB_NONLINEAR_KHR: return "sRGB nonlinear";
		default: return "Other";
		}
	}

	const char* PresentModeName(VkPresentModeKHR mode)
	{
		switch (mode)
		{
		case VK_PRESENT_MODE_MAILBOX_KHR: return "Mailbox";
		case VK_PRESENT_MODE_FIFO_KHR: return "FIFO";
		case VK_PRESENT_MODE_FIFO_RELAXED_KHR: return "FIFO relaxed";
		case VK_PRESENT_MODE_IMMEDIATE_KHR: return "Immediate";
		default: return "Other";
		}
	}

	std::string DeviceMemory()
	{
		VkPhysicalDeviceMemoryProperties memory {};
		vkGetPhysicalDeviceMemoryProperties(Lite::Renderer::GetPhysicalDevice(), &memory);

		VkDeviceSize local = 0;
		for (uint32_t index = 0; index < memory.memoryHeapCount; ++index)
		{
			if (memory.memoryHeaps[index].flags & VK_MEMORY_HEAP_DEVICE_LOCAL_BIT)
				local += memory.memoryHeaps[index].size;
		}

		double gigabytes = static_cast<double>(local) / (1024.0 * 1024.0 * 1024.0);
		return std::format("{:.1f} GB", gigabytes);
	}

	bool Contains(Lite::Vec2 point, const Lite::Vec2* vertices, int count)
	{
		bool positive = false;
		bool negative = false;
		for (int index = 0; index < count; ++index)
		{
			const Lite::Vec2& current = vertices[index];
			const Lite::Vec2& next = vertices[(index + 1) % count];
			float cross = (next.x - current.x) * (point.y - current.y) - (next.y - current.y) * (point.x - current.x);
			if (cross > 0.0f)
				positive = true;
			if (cross < 0.0f)
				negative = true;
			if (positive && negative)
				return false;
		}

		return true;
	}

	bool Hits(const Lite::SceneObject& object, Lite::Vec2 point)
	{
		if (object.Kind == Lite::SceneObjectKind::Triangle)
		{
			const Lite::Vec2 local[3] = {
				{ 0.00f, -0.72f },
				{ -0.78f, 0.58f },
				{ 0.78f, 0.58f }
			};
			Lite::Vec2 world[3];
			for (int index = 0; index < 3; ++index)
			{
				Lite::Vec3 transformed = object.Transform.TransformPoint({ local[index].x, local[index].y, 0.0f });
				world[index] = { transformed.x, transformed.y };
			}
			return Contains(point, world, 3);
		}

		const Lite::Vec2 local[4] = {
			{ -0.5f, -0.5f },
			{ 0.5f, -0.5f },
			{ 0.5f, 0.5f },
			{ -0.5f, 0.5f }
		};
		Lite::Vec2 world[4];
		for (int index = 0; index < 4; ++index)
		{
			Lite::Vec3 transformed = object.Transform.TransformPoint({ local[index].x, local[index].y, 0.0f });
			world[index] = { transformed.x, transformed.y };
		}
		return Contains(point, world, 4);
	}

	void ColorField(const char* label, Lite::Vec4& color)
	{
		ImGui::AlignTextToFramePadding();
		ImGui::TextUnformatted(label);
		ImGui::SameLine();
		ImGui::SetNextItemWidth(-1.0f);
		ImGui::ColorEdit4(label, &color.x, ImGuiColorEditFlags_Float | ImGuiColorEditFlags_AlphaBar | ImGuiColorEditFlags_NoLabel);
	}

	void DrawPlay(ImDrawList* draw, ImVec2 min, ImVec2 max, ImU32 color)
	{
		ImVec2 center { (min.x + max.x) * 0.5f, (min.y + max.y) * 0.5f };
		float height = 10.0f;
		float width = 8.0f;
		draw->AddTriangleFilled(
			ImVec2(center.x - width * 0.35f, center.y - height * 0.5f),
			ImVec2(center.x - width * 0.35f, center.y + height * 0.5f),
			ImVec2(center.x + width * 0.65f, center.y),
			color);
	}

	void DrawPause(ImDrawList* draw, ImVec2 min, ImVec2 max, ImU32 color)
	{
		ImVec2 center { (min.x + max.x) * 0.5f, (min.y + max.y) * 0.5f };
		float height = 10.0f;
		float width = 2.5f;
		float gap = 2.5f;
		draw->AddRectFilled(
			ImVec2(center.x - gap - width, center.y - height * 0.5f),
			ImVec2(center.x - gap, center.y + height * 0.5f),
			color, 1.0f);
		draw->AddRectFilled(
			ImVec2(center.x + gap, center.y - height * 0.5f),
			ImVec2(center.x + gap + width, center.y + height * 0.5f),
			color, 1.0f);
	}

	bool TransportButton(const char* id, bool enabled, bool active, const ImVec4& activeColor, bool play)
	{
		ImGui::BeginDisabled(!enabled);
		ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(0.0f, 0.0f));
		ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 4.0f);
		ImGui::PushStyleColor(ImGuiCol_Button, active ? activeColor : ImVec4(0.16f, 0.17f, 0.21f, 1.0f));
		ImGui::PushStyleColor(ImGuiCol_ButtonHovered, active ? activeColor : ImVec4(0.22f, 0.26f, 0.32f, 1.0f));
		ImGui::PushStyleColor(ImGuiCol_ButtonActive, active ? activeColor : ImVec4(0.28f, 0.34f, 0.44f, 1.0f));
		bool pressed = ImGui::Button(id, ImVec2(28.0f, 22.0f));
		ImU32 symbol = ImGui::GetColorU32(active ? ImVec4(0.96f, 0.97f, 0.98f, 1.0f) : ImVec4(0.78f, 0.82f, 0.88f, 1.0f));
		ImDrawList* draw = ImGui::GetWindowDrawList();
		if (play)
			DrawPlay(draw, ImGui::GetItemRectMin(), ImGui::GetItemRectMax(), symbol);
		else
			DrawPause(draw, ImGui::GetItemRectMin(), ImGui::GetItemRectMax(), symbol);
		ImGui::PopStyleColor(3);
		ImGui::PopStyleVar(2);
		ImGui::EndDisabled();
		return pressed && enabled;
	}

}

EditorLayer::EditorLayer()
	: Lite::Layer("Editor")
{
}

void EditorLayer::OnAttach()
{
	OpenScene("assets/scenes/Sandbox.scene");
	if (!m_Scene)
		NewScene();

	m_Camera.SetProjection(m_ViewSize, 16.0f / 9.0f);
}

void EditorLayer::OnDetach()
{
	if (m_Scene)
	{
		m_Scene->Stop();
		if (Lite::Scene::GetActive() == m_Scene.get())
			Lite::Scene::SetActive(nullptr);
		m_Scene.reset();
	}
}

void EditorLayer::OnUpdate(Lite::Timestep timestep)
{
	LITE_PROFILE_SCOPE("Editor Update");
	float rotation = m_Camera.GetRotation();
	float step = 1.6f * timestep.GetSeconds();
	if (Lite::Input::IsKeyPressed(Lite::Key::Q))
		rotation += step;
	if (Lite::Input::IsKeyPressed(Lite::Key::E))
		rotation -= step;
	m_Camera.SetRotation(rotation);

	if (m_Scene)
		m_Scene->Update(timestep.GetSeconds());
}

void EditorLayer::OnRender()
{
	LITE_PROFILE_SCOPE("Editor Render");
	if (!m_Scene)
		return;

	VkExtent2D extent = Lite::Renderer::GetExtent();
	float aspect = extent.height > 0 ? static_cast<float>(extent.width) / static_cast<float>(extent.height) : 1.0f;
	m_Camera.SetProjection(m_ViewSize, aspect);
	Lite::Renderer2D::SetViewProjection(m_Camera.GetViewProjection());
	m_Scene->Render();
}

void EditorLayer::NewScene()
{
	m_Scene = Lite::CreateScope<Lite::Scene>("Untitled");
	m_Scene->Create();
	Lite::Scene::SetActive(m_Scene.get());
	m_Selection.clear();
	SyncName();
}

void EditorLayer::OpenScene(const std::string& path)
{
	Lite::Scope<Lite::Scene> scene = Lite::Scene::Open(path);
	if (!scene)
		return;

	if (m_Scene && Lite::Scene::GetActive() == m_Scene.get())
		Lite::Scene::SetActive(nullptr);

	m_Scene = std::move(scene);
	Lite::Scene::SetActive(m_Scene.get());
	m_Selection = m_Scene->Find("Triangle") != nullptr ? "Triangle" : std::string{};
	SyncName();
}

void EditorLayer::SyncName()
{
	m_NamedScene = m_Scene.get();
	const char* name = m_Scene != nullptr ? m_Scene->GetName().c_str() : "";
	std::snprintf(m_Name, sizeof(m_Name), "%s", name);
}

void EditorLayer::SyncObjectFields(const Lite::SceneObject& object)
{
	if (m_ObjectEdit == object.Name)
		return;

	m_ObjectEdit = object.Name;
	std::snprintf(m_ObjectName, sizeof(m_ObjectName), "%s", object.Name.c_str());
	std::snprintf(m_ShaderText, sizeof(m_ShaderText), "%s", object.Shader.c_str());
	std::snprintf(m_TextureText, sizeof(m_TextureText), "%s", object.TexturePath.c_str());
}

void EditorLayer::PickObject(float mouseX, float mouseY)
{
	if (m_Scene == nullptr || m_Scene->IsPlaying())
		return;

	ImVec2 display = ImGui::GetIO().DisplaySize;
	if (display.x <= 0.0f || display.y <= 0.0f)
		return;

	float ndcX = (mouseX / display.x) * 2.0f - 1.0f;
	float ndcY = 1.0f - (mouseY / display.y) * 2.0f;
	Lite::Vec4 world = m_Camera.GetViewProjection().Inverse() * Lite::Vec4(ndcX, ndcY, 0.0f, 1.0f);
	if (world.w != 0.0f)
		world /= world.w;

	const std::vector<Lite::SceneObject>& objects = m_Scene->GetObjects();
	for (int index = static_cast<int>(objects.size()) - 1; index >= 0; --index)
	{
		if (!Hits(objects[static_cast<size_t>(index)], { world.x, world.y }))
			continue;

		const std::string& name = objects[static_cast<size_t>(index)].Name;
		if (m_Selection != name)
			Lite::Console::Log(std::format("Selected: {}", name));
		m_Selection = name;
		return;
	}
}

void EditorLayer::OnEvent(Lite::Event& event)
{
	Lite::EventDispatcher dispatcher(event);
	dispatcher.Dispatch<Lite::KeyPressedEvent>([this](Lite::KeyPressedEvent& key)
	{
		if (key.IsRepeat())
			return false;

		if (key.GetKeyCode() == Lite::Key::I)
		{
			m_ShowInfo = !m_ShowInfo;
			LITE_CLIENT_INFO("Instrumentation {}", m_ShowInfo ? "shown" : "hidden");
			return true;
		}

		bool control = Lite::Input::IsKeyPressed(Lite::Key::LeftControl) || Lite::Input::IsKeyPressed(Lite::Key::RightControl);
		if (control && key.GetKeyCode() == Lite::Key::N)
		{
			NewScene();
			return true;
		}

		if (control && key.GetKeyCode() == Lite::Key::S)
		{
			if (m_Scene)
				m_ShowSave = true;
			return true;
		}

		if (control && key.GetKeyCode() == Lite::Key::O)
		{
			m_ShowOpen = true;
			return true;
		}

		if (key.GetKeyCode() == Lite::Key::F5 && m_Scene)
		{
			m_Scene->Play();
			return true;
		}

		if (key.GetKeyCode() == Lite::Key::F6 && m_Scene)
		{
			m_Scene->Pause();
			return true;
		}

		return false;
	});

	dispatcher.Dispatch<Lite::MouseScrolledEvent>([this](Lite::MouseScrolledEvent& scroll)
	{
		float steps = scroll.GetYOffset();
		if (steps == 0.0f)
			return false;

		m_ViewSize *= std::pow(0.85f, steps);
		if (m_ViewSize < 0.25f)
			m_ViewSize = 0.25f;
		if (m_ViewSize > 12.0f)
			m_ViewSize = 12.0f;
		return false;
	});
}

void EditorLayer::DrawMenu()
{
	ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(12.0f, 8.0f));
	if (!ImGui::BeginMainMenuBar())
	{
		ImGui::PopStyleVar();
		return;
	}

	ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.78f, 0.88f, 1.0f, 1.0f));
	ImGui::TextUnformatted("Lite");
	ImGui::PopStyleColor();
	ImGui::SameLine(0.0f, 14.0f);
	ImGui::TextDisabled("|");
	ImGui::SameLine(0.0f, 14.0f);

	if (ImGui::BeginMenu("File"))
	{
		if (ImGui::MenuItem("New Scene", "Ctrl+N"))
			NewScene();
		if (ImGui::MenuItem("Open Scene...", "Ctrl+O"))
			m_ShowOpen = true;
		if (ImGui::MenuItem("Save Scene...", "Ctrl+S", false, m_Scene != nullptr))
			m_ShowSave = true;
		ImGui::EndMenu();
	}

	ImGui::SameLine(0.0f, 8.0f);
	ImGui::TextDisabled("|");
	ImGui::SameLine(0.0f, 8.0f);

	if (ImGui::BeginMenu("View"))
	{
		if (ImGui::MenuItem("Instrumentation", "I", &m_ShowInfo))
			LITE_CLIENT_INFO("Instrumentation {}", m_ShowInfo ? "shown" : "hidden");
		ImGui::EndMenu();
	}

	float transport = 28.0f * 2.0f + 8.0f;
	ImGui::SetCursorPosX((ImGui::GetWindowWidth() - transport) * 0.5f);
	bool playing = m_Scene && m_Scene->IsPlaying();
	bool paused = m_Scene && m_Scene->GetPlayback() == Lite::ScenePlayback::Paused;
	if (TransportButton("##Play", m_Scene != nullptr, playing, ImVec4(0.18f, 0.48f, 0.28f, 1.0f), true))
		m_Scene->Play();
	if (ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenDisabled))
		ImGui::SetTooltip("Play");
	ImGui::SameLine(0.0f, 8.0f);
	if (TransportButton("##Pause", m_Scene != nullptr, paused, ImVec4(0.45f, 0.36f, 0.14f, 1.0f), false))
		m_Scene->Pause();
	if (ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenDisabled))
		ImGui::SetTooltip("Pause");

	ImGui::EndMainMenuBar();
	ImGui::PopStyleVar();
}

void EditorLayer::OpenBrowser()
{
	std::filesystem::path directory = Lite::FileSystem::ExecutableDirectory() / "assets" / "scenes";
	std::string fileName = m_Scene != nullptr ? m_Scene->GetName() : "Untitled";
	if (m_Scene != nullptr && !m_Scene->GetPath().empty())
	{
		std::filesystem::path current(m_Scene->GetPath());
		if (!current.is_absolute())
			current = Lite::FileSystem::ExecutableDirectory() / current;
		if (current.has_parent_path())
			directory = current.parent_path();
		if (!current.stem().empty())
			fileName = current.stem().string();
	}

	std::error_code error;
	if (!std::filesystem::is_directory(directory, error))
		directory = Lite::FileSystem::ExecutableDirectory();

	m_BrowserDirectory = directory.string();
	std::snprintf(m_DirectoryText, sizeof(m_DirectoryText), "%s", m_BrowserDirectory.c_str());
	std::snprintf(m_FileName, sizeof(m_FileName), "%s", fileName.c_str());
	m_FocusFile = true;
}

void EditorLayer::ApplyDirectoryText()
{
	std::filesystem::path typed(m_DirectoryText);
	if (typed.empty())
	{
		m_BrowserDirectory.clear();
		return;
	}

	std::error_code error;
	if (std::filesystem::is_directory(typed, error))
	{
		m_BrowserDirectory = std::filesystem::absolute(typed, error).string();
		std::snprintf(m_DirectoryText, sizeof(m_DirectoryText), "%s", m_BrowserDirectory.c_str());
		return;
	}

	if (std::filesystem::is_regular_file(typed, error))
	{
		m_BrowserDirectory = std::filesystem::absolute(typed.parent_path(), error).string();
		std::snprintf(m_DirectoryText, sizeof(m_DirectoryText), "%s", m_BrowserDirectory.c_str());
		std::snprintf(m_FileName, sizeof(m_FileName), "%s", typed.stem().string().c_str());
	}
}

std::string EditorLayer::BrowserSelection() const
{
	std::filesystem::path entered(m_FileName);
	if (entered.empty())
		return {};

	if (!entered.is_absolute())
	{
		if (m_BrowserDirectory.empty())
			return {};
		entered = std::filesystem::path(m_BrowserDirectory) / entered;
	}

	if (entered.extension().empty())
		entered.replace_extension(".scene");

	return entered.lexically_normal().string();
}

void EditorLayer::DrawFileBrowser(bool save)
{
	ImGuiInputTextFlags directoryFlags = ImGuiInputTextFlags_EnterReturnsTrue;
	ImGui::SetNextItemWidth(-1.0f);
	if (ImGui::InputText("##Directory", m_DirectoryText, sizeof(m_DirectoryText), directoryFlags) || ImGui::IsItemDeactivatedAfterEdit())
		ApplyDirectoryText();

	ImGui::BeginChild(save ? "##SaveFiles" : "##OpenFiles", ImVec2(0.0f, -ImGui::GetFrameHeightWithSpacing() * 2.0f), ImGuiChildFlags_Borders);

	if (m_BrowserDirectory.empty())
	{
		for (char letter = 'A'; letter <= 'Z'; ++letter)
		{
			std::string root = std::string(1, letter) + ":\\";
			std::error_code error;
			if (!std::filesystem::exists(root, error))
				continue;
			if (ImGui::Selectable(root.c_str()))
			{
				m_BrowserDirectory = root;
				std::snprintf(m_DirectoryText, sizeof(m_DirectoryText), "%s", m_BrowserDirectory.c_str());
			}
		}
	}
	else
	{
		std::filesystem::path current(m_BrowserDirectory);
		if (ImGui::Selectable(".."))
		{
			std::filesystem::path parent = current.parent_path();
			if (parent.empty() || parent == current)
				m_BrowserDirectory.clear();
			else
				m_BrowserDirectory = parent.string();
			std::snprintf(m_DirectoryText, sizeof(m_DirectoryText), "%s", m_BrowserDirectory.c_str());
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
				m_BrowserDirectory = entry.Path.string();
				std::snprintf(m_DirectoryText, sizeof(m_DirectoryText), "%s", m_BrowserDirectory.c_str());
				continue;
			}

			std::snprintf(m_FileName, sizeof(m_FileName), "%s", entry.Path.stem().string().c_str());
			if (!save && ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left))
			{
				OpenScene(entry.Path.string());
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
	if ((ImGui::Button(action, ImVec2(96.0f, 0.0f)) || confirm))
	{
		std::string path = BrowserSelection();
		if (!path.empty())
		{
			if (save && m_Scene != nullptr && m_Scene->SaveAs(path))
			{
				SyncName();
				ImGui::CloseCurrentPopup();
			}
			else if (!save)
			{
				OpenScene(path);
				ImGui::CloseCurrentPopup();
			}
		}
	}
	ImGui::SameLine();
	if (ImGui::Button("Cancel", ImVec2(96.0f, 0.0f)))
		ImGui::CloseCurrentPopup();
}

void EditorLayer::DrawOpenDialog()
{
	if (m_ShowOpen)
	{
		OpenBrowser();
		ImGui::OpenPopup("Open Scene");
		m_ShowOpen = false;
	}

	ImGui::SetNextWindowSize(ImVec2(560.0f, 460.0f), ImGuiCond_Appearing);
	if (!ImGui::BeginPopupModal("Open Scene", nullptr, ImGuiWindowFlags_NoResize))
		return;

	DrawFileBrowser(false);
	ImGui::EndPopup();
}

void EditorLayer::DrawSaveDialog()
{
	if (m_ShowSave)
	{
		OpenBrowser();
		ImGui::OpenPopup("Save Scene");
		m_ShowSave = false;
	}

	ImGui::SetNextWindowSize(ImVec2(560.0f, 460.0f), ImGuiCond_Appearing);
	if (!ImGui::BeginPopupModal("Save Scene", nullptr, ImGuiWindowFlags_NoResize))
		return;

	DrawFileBrowser(true);
	ImGui::EndPopup();
}

void EditorLayer::OnImGuiRender()
{
	DrawMenu();

	ImGuiWindowFlags hostFlags = ImGuiWindowFlags_NoDocking | ImGuiWindowFlags_NoTitleBar
		| ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove
		| ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoNavFocus
		| ImGuiWindowFlags_NoBackground;

	const ImGuiViewport* viewport = ImGui::GetMainViewport();
	ImGui::SetNextWindowPos(viewport->WorkPos);
	ImGui::SetNextWindowSize(viewport->WorkSize);
	ImGui::SetNextWindowViewport(viewport->ID);
	ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
	ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
	ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
	ImGui::Begin("EditorDockSpace", nullptr, hostFlags);
	ImGui::PopStyleVar(3);

	ImGuiID dockspaceId = ImGui::GetID("EditorView");
	if (EditorDockNeedsBuild(dockspaceId))
		BuildEditorDock(dockspaceId, viewport->WorkSize);

	ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0.0f, 0.0f, 0.0f, 0.0f));
	ImGui::DockSpace(dockspaceId, ImVec2(0.0f, 0.0f), ImGuiDockNodeFlags_PassthruCentralNode);
	ImGui::PopStyleColor();
	ImGui::End();

	DrawScene();
	DrawViewport();
	DrawInspector();
	DrawConsole();
	if (m_ShowInfo)
		DrawInstrumentation();
	DrawOpenDialog();
	DrawSaveDialog();
}

void EditorLayer::DrawScene()
{
	ImGui::Begin("Scene");
	if (m_Scene)
	{
		if (m_NamedScene != m_Scene.get())
			SyncName();
		ImGui::SetNextItemWidth(-1.0f);
		if (ImGui::InputText("##SceneName", m_Name, sizeof(m_Name)))
			m_Scene->SetName(m_Name);
	}

	if (m_Scene == nullptr)
	{
		ImGui::TextDisabled("No scene");
		ImGui::End();
		return;
	}

	const char* playback = "stopped";
	switch (m_Scene->GetPlayback())
	{
		case Lite::ScenePlayback::Playing: playback = "playing"; break;
		case Lite::ScenePlayback::Paused: playback = "paused"; break;
		case Lite::ScenePlayback::Stopped: playback = "stopped"; break;
	}
	ImGui::TextDisabled("%s  %s", m_Scene->GetName().c_str(), playback);
	for (const Lite::SceneObject& object : m_Scene->GetObjects())
	{
		if (ImGui::Selectable(object.Name.c_str(), m_Selection == object.Name))
			m_Selection = object.Name;
	}
	ImGui::End();
}

void EditorLayer::DrawViewport()
{
	ImGui::Begin("Viewport", nullptr, ImGuiWindowFlags_NoBackground | ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse);
	ImVec2 origin = ImGui::GetCursorScreenPos();
	ImVec2 size = ImGui::GetContentRegionAvail();
	ImGui::InvisibleButton("##ViewportPick", size);
	if (ImGui::IsItemClicked(ImGuiMouseButton_Left))
	{
		ImVec2 mouse = ImGui::GetIO().MousePos;
		PickObject(mouse.x, mouse.y);
	}

	const char* name = m_Scene != nullptr ? m_Scene->GetName().c_str() : "No scene";
	const char* playback = "stopped";
	if (m_Scene)
	{
		switch (m_Scene->GetPlayback())
		{
			case Lite::ScenePlayback::Playing: playback = "playing"; break;
			case Lite::ScenePlayback::Paused: playback = "paused"; break;
			case Lite::ScenePlayback::Stopped: playback = "stopped"; break;
		}
	}
	ImGui::SetCursorScreenPos(origin);
	ImGui::TextDisabled("%s  %s  %.0f x %.0f", name, playback, size.x, size.y);
	ImGui::End();
}

void EditorLayer::DrawInspector()
{
	ImGui::Begin("Inspector");
	Lite::Scene* scene = Lite::Scene::GetActive();
	Lite::SceneObject* object = scene != nullptr ? scene->Find(m_Selection) : nullptr;
	if (object == nullptr)
	{
		ImGui::TextDisabled("No selection");
		ImGui::End();
		return;
	}

	const char* kind = "Quad";
	switch (object->Kind)
	{
		case Lite::SceneObjectKind::Sprite: kind = "Sprite"; break;
		case Lite::SceneObjectKind::Triangle: kind = "Triangle"; break;
		case Lite::SceneObjectKind::Quad: kind = "Quad"; break;
	}

	SyncObjectFields(*object);
	bool editing = scene->GetPlayback() != Lite::ScenePlayback::Playing;
	if (!editing)
		ImGui::BeginDisabled();

	ImGui::SetNextItemWidth(-1.0f);
	if (ImGui::InputText("##ObjectName", m_ObjectName, sizeof(m_ObjectName)))
	{
		object->Name = m_ObjectName;
		m_Selection = object->Name;
		m_ObjectEdit = object->Name;
	}

	ImGui::TextDisabled("%s", kind);
	ImGui::Separator();

	ImGui::AlignTextToFramePadding();
	ImGui::TextUnformatted("Shader");
	ImGui::SameLine();
	ImGui::SetNextItemWidth(-1.0f);
	if (ImGui::InputText("##Shader", m_ShaderText, sizeof(m_ShaderText)))
		object->Shader = m_ShaderText;

	ImGui::DragFloat3("Position", &object->Transform.Position.x, 0.01f);
	float rotation = object->Transform.GetRotationZ();
	if (ImGui::DragFloat("Rotation", &rotation, 0.01f))
		object->Transform.SetRotationZ(rotation);
	ImGui::DragFloat3("Scale", &object->Transform.Scale.x, 0.01f);

	if (object->Kind == Lite::SceneObjectKind::Triangle || object->UseCornerColors)
	{
		int colors = object->Kind == Lite::SceneObjectKind::Triangle ? 3 : 4;
		for (int index = 0; index < colors; ++index)
			ColorField(std::format("Color {}", index + 1).c_str(), object->CornerColors[index]);
	}
	else
	{
		ColorField("Color", object->Color);
	}

	if (object->Kind == Lite::SceneObjectKind::Sprite)
	{
		ImGui::DragFloat2("Tiling", &object->Tiling.x, 0.01f);
		ImGui::AlignTextToFramePadding();
		ImGui::TextUnformatted("Texture");
		ImGui::SameLine();
		ImGui::SetNextItemWidth(-1.0f);
		if (ImGui::InputText("##Texture", m_TextureText, sizeof(m_TextureText)))
		{
			object->TexturePath = m_TextureText;
			object->Texture = object->TexturePath.empty()
				? Lite::Ref<Lite::Texture>{}
				: Lite::AssetRegistry::Get().Load<Lite::Texture>(object->TexturePath);
		}
	}

	ImGui::DragFloat("Spin", &object->Spin, 0.01f);

	if (!editing)
	{
		ImGui::EndDisabled();
		ImGui::TextDisabled("Pause to edit");
	}

	ImGui::End();
}

void EditorLayer::DrawConsole()
{
	ImGui::Begin("Console");
	const std::vector<std::string>& lines = Lite::Console::GetLines();
	const bool follow = ImGui::GetScrollY() >= ImGui::GetScrollMaxY() - 1.0f;
	if (lines.empty())
		ImGui::TextDisabled("No messages");

	for (const std::string& line : lines)
		ImGui::TextUnformatted(line.c_str());

	if (follow)
		ImGui::SetScrollHereY(1.0f);
	ImGui::End();
}

void EditorLayer::DrawInstrumentation()
{
	LITE_PROFILE_SCOPE("ImGui Panels");

	VkPhysicalDeviceProperties properties {};
	vkGetPhysicalDeviceProperties(Lite::Renderer::GetPhysicalDevice(), &properties);
	VkExtent2D extent = Lite::Renderer::GetExtent();

	auto version = [](uint32_t value)
	{
		return std::format("{}.{}.{}", VK_VERSION_MAJOR(value), VK_VERSION_MINOR(value), VK_VERSION_PATCH(value));
	};

	const char* deviceType = "Other";
	switch (properties.deviceType)
	{
		case VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU: deviceType = "Discrete GPU"; break;
		case VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU: deviceType = "Integrated GPU"; break;
		case VK_PHYSICAL_DEVICE_TYPE_VIRTUAL_GPU: deviceType = "Virtual GPU"; break;
		case VK_PHYSICAL_DEVICE_TYPE_CPU: deviceType = "CPU"; break;
		default: break;
	}

	bool place = !m_InfoPlaced;
	BeginDocked("Console", "Profile", place);
	const ImGuiIO& io = ImGui::GetIO();
	float cap = Lite::Time::GetFPS();
	if (BeginRows("##session"))
	{
		Row("FPS", std::format("{:.1f}", io.Framerate));
		Row("Elapsed", std::format("{:.1f} s", Lite::Time::GetElapsed()));
		Row("Cap", cap > 0.0f ? std::format("{:.0f}", cap) : "off");
		ImGui::EndTable();
	}

	ImGui::Dummy(ImVec2(0.0f, 6.0f));
	const std::vector<Lite::ProfileSample>& samples = Lite::Profiler::GetSamples();
	if (samples.empty())
	{
		ImGui::TextDisabled("Waiting for a completed frame");
	}
	else if (ImGui::BeginTable("##profile", 2, ImGuiTableFlags_RowBg | ImGuiTableFlags_BordersInnerV | ImGuiTableFlags_PadOuterX | ImGuiTableFlags_SizingStretchProp))
	{
		ImGui::TableSetupColumn("Scope", ImGuiTableColumnFlags_WidthStretch, 0.72f);
		ImGui::TableSetupColumn("Time", ImGuiTableColumnFlags_WidthStretch, 0.28f);
		ImGui::TableHeadersRow();

		for (const Lite::ProfileSample& sample : samples)
		{
			ImGui::TableNextRow();
			ImGui::TableSetColumnIndex(0);
			ImGui::SetCursorPosX(ImGui::GetCursorPosX() + static_cast<float>(sample.Depth) * 14.0f);
			ImGui::TextUnformatted(sample.Name);
			ImGui::TableSetColumnIndex(1);
			ImGui::TextUnformatted(std::format("{:.3f} ms", sample.Milliseconds).c_str());
		}

		ImGui::EndTable();
	}
	ImGui::Spacing();
	ImGui::TextDisabled("Press I to hide");
	ImGui::End();

	BeginDocked("Inspector", "Draw", place);
	if (BeginRows("##draw"))
	{
		Row("Frame active", Lite::Renderer::IsFrameActive() ? "yes" : "no");
		Row("Draw calls", std::format("{}", Lite::Renderer::GetDrawCalls()));
		Row("Quads", std::format("{}", Lite::Renderer::GetQuadCount()));
		Row("Triangles", std::format("{}", Lite::Renderer::GetTriangleCount()));
		Row("Indices", std::format("{}", Lite::Renderer::GetIndexCount()));
		Row("Samples", "1");
		Row("Blend", "Premultiplied");
		ImGui::EndTable();
	}
	ImGui::End();

	BeginDocked("Scene", "GPU", place);
	if (BeginRows("##gpu"))
	{
		Row("Name", properties.deviceName);
		Row("Type", deviceType);
		Row("Vendor", std::format("{:04X}", properties.vendorID));
		Row("Device", std::format("{:04X}", properties.deviceID));
		Row("Memory", DeviceMemory());
		Row("API", version(properties.apiVersion));
		Row("Driver", version(properties.driverVersion));
		Row("Max texture", std::format("{}", properties.limits.maxImageDimension2D));
		Row("Max framebuffer", std::format("{} x {}", properties.limits.maxFramebufferWidth, properties.limits.maxFramebufferHeight));
		Row("Max uniform", std::format("{}", properties.limits.maxUniformBufferRange));
		Row("Max anisotropy", std::format("{:.0f}", properties.limits.maxSamplerAnisotropy));
		Row("Max samplers", std::format("{}", properties.limits.maxPerStageDescriptorSamplers));
		ImGui::EndTable();
	}
	ImGui::End();

	BeginDocked("Scene", "Swapchain", place);
	if (BeginRows("##swapchain"))
	{
		Row("Extent", std::format("{} x {}", extent.width, extent.height));
		Row("Format", FormatName(Lite::Renderer::GetSwapchainFormat()));
		Row("Color space", ColorSpaceName(Lite::Renderer::GetColorSpace()));
		Row("Present", PresentModeName(Lite::Renderer::GetPresentMode()));
		Row("Images", std::format("{}", Lite::Renderer::GetImageCount()));
		Row("Min images", std::format("{}", Lite::Renderer::GetMinImageCount()));
		Row("Image index", std::format("{}", Lite::Renderer::GetImageIndex()));
		Row("Frame index", std::format("{}", Lite::Renderer::GetFrameIndex()));
		Row("Frames in flight", std::format("{}", Lite::VulkanSync::FramesInFlight));
		Row("Queue family", std::format("{}", Lite::Renderer::GetGraphicsQueueFamily()));
		ImGui::EndTable();
	}
	ImGui::End();

	m_InfoPlaced = true;
}
