#include <EditorLayer.h>

#include <Lite/Core/Events/KeyEvent.h>
#include <Lite/Core/Events/MouseEvent.h>
#include <Lite/Core/Log/Logger.h>
#include <Lite/Core/Profile/Profiler.h>
#include <Lite/ImGui/Instrumentation.h>
#include <Lite/Input/Input.h>
#include <Lite/Input/KeyCodes.h>
#include <Lite/Core/IO/FileSystem.h>
#include <Lite/Project/Project.h>
#include <Lite/Renderer/Renderer.h>
#include <Lite/Renderer/Renderer2D.h>
#include <Lite/Scene/Console.h>
#include <Lite/Scene/SceneCamera.h>

#include <imgui.h>
#include <imgui_internal.h>

#include <cmath>
#include <cstdio>
#include <algorithm>
#include <cctype>
#include <cstring>
#include <filesystem>
#include <format>
#include <string>
#include <vector>

namespace {

	const char* kEditorWindows[] = { "Scene", "Viewport", "Inspector", "Console", "Explorer" };

	bool EditorDockNeedsBuild(ImGuiID dockspaceId)
	{
		ImGuiDockNode* node = ImGui::DockBuilderGetNode(dockspaceId);
		if (node == nullptr || !node->IsSplitNode())
			return true;

		bool live = false;
		bool explorerDocked = false;
		for (const char* name : kEditorWindows)
		{
			ImGuiWindow* window = ImGui::FindWindowByName(name);
			if (window == nullptr || window->DockNode == nullptr)
				continue;

			live = true;
			ImGuiDockNode* root = ImGui::DockNodeGetRootNode(window->DockNode);
			bool dockedHere = root != nullptr && root->ID == dockspaceId;
			if (std::strcmp(name, "Explorer") == 0)
				explorerDocked = dockedHere;
			else if (!dockedHere)
				return true;
		}

		return live && !explorerDocked;
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
		ImGui::DockBuilderDockWindow("Explorer", bottom);
		ImGui::DockBuilderFinish(dockspaceId);
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

	void DrawReset(ImDrawList* draw, ImVec2 min, ImVec2 max, ImU32 color)
	{
		ImVec2 center { (min.x + max.x) * 0.5f, (min.y + max.y) * 0.5f + 0.5f };
		float radius = 5.2f;
		float start = -2.5f;
		float end = 2.4f;
		draw->PathArcTo(center, radius, start, end, 16);
		draw->PathStroke(color, ImDrawFlags_None, 1.6f);

		float tipX = center.x + std::cos(end) * radius;
		float tipY = center.y + std::sin(end) * radius;
		float tangentX = -std::sin(end);
		float tangentY = std::cos(end);
		float normalX = std::cos(end);
		float normalY = std::sin(end);
		draw->AddTriangleFilled(
			ImVec2(tipX + tangentX * 1.2f, tipY + tangentY * 1.2f),
			ImVec2(tipX - tangentX * 4.2f + normalX * 2.6f, tipY - tangentY * 4.2f + normalY * 2.6f),
			ImVec2(tipX - tangentX * 4.2f - normalX * 2.6f, tipY - tangentY * 4.2f - normalY * 2.6f),
			color);
	}

	enum class TransportSymbol
	{
		Play,
		Pause,
		Reset
	};

	bool TransportButton(const char* id, bool enabled, bool active, const ImVec4& activeColor, TransportSymbol symbol)
	{
		ImGui::BeginDisabled(!enabled);
		ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(0.0f, 0.0f));
		ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 4.0f);
		ImGui::PushStyleColor(ImGuiCol_Button, active ? activeColor : ImVec4(0.16f, 0.17f, 0.21f, 1.0f));
		ImGui::PushStyleColor(ImGuiCol_ButtonHovered, active ? activeColor : ImVec4(0.22f, 0.26f, 0.32f, 1.0f));
		ImGui::PushStyleColor(ImGuiCol_ButtonActive, active ? activeColor : ImVec4(0.28f, 0.34f, 0.44f, 1.0f));
		bool pressed = ImGui::Button(id, ImVec2(28.0f, 22.0f));
		ImU32 symbolColor = ImGui::GetColorU32(active ? ImVec4(0.96f, 0.97f, 0.98f, 1.0f) : ImVec4(0.78f, 0.82f, 0.88f, 1.0f));
		ImDrawList* draw = ImGui::GetWindowDrawList();
		ImVec2 itemMin = ImGui::GetItemRectMin();
		ImVec2 itemMax = ImGui::GetItemRectMax();
		if (symbol == TransportSymbol::Play)
			DrawPlay(draw, itemMin, itemMax, symbolColor);
		else if (symbol == TransportSymbol::Pause)
			DrawPause(draw, itemMin, itemMax, symbolColor);
		else
			DrawReset(draw, itemMin, itemMax, symbolColor);
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
	std::filesystem::path projectPath = Lite::Project::Locate("Sandbox/Sandbox.lite");
	if (projectPath.empty() || !Lite::Project::Load(projectPath))
	{
		NewProject();
		return;
	}

	m_ProjectPath = projectPath;
	Lite::Ref<Lite::Project> project = Lite::Project::GetActive();
	Lite::Console::Log(std::format("Project loaded: {} ({})", project->GetConfig().Name, projectPath.string()));

	const std::filesystem::path& start = project->GetConfig().StartScene;
	std::filesystem::path scene;
	if (!start.empty())
		scene = Lite::Project::GetAssetFileSystemPath(start);
	if (!scene.empty())
		OpenScene(scene.string());
	if (scene.empty() || !SceneMatches(scene))
		NewScene();
}

void EditorLayer::OnDetach()
{
	Lite::Scene::Close(m_Scene);
}

void EditorLayer::OnUpdate(Lite::Timestep timestep)
{
	LITE_PROFILE_SCOPE("Editor Update");
	if (!m_Scene || !m_Scene->IsPlaying())
	{
		float rotation = m_Camera.GetRotation();
		Lite::TurnCamera(rotation, timestep.GetSeconds());
		m_Camera.SetRotation(rotation);
	}

	if (m_Scene)
		m_Scene->Update(timestep.GetSeconds());
}

void EditorLayer::OnRender()
{
	LITE_PROFILE_SCOPE("Editor Render");
	if (!m_Scene)
		return;

	ApplyPlayCamera();
	m_Scene->Render();
}

void EditorLayer::NewProject()
{
	std::filesystem::path parent = std::filesystem::current_path();
	if (Lite::Ref<Lite::Project> current = Lite::Project::GetActive())
	{
		if (!current->GetProjectDirectory().empty())
			parent = current->GetProjectDirectory().parent_path();
	}

	std::filesystem::path folder = Lite::FileSystem::Unused(parent, "Untitled", "");
	std::error_code error;
	std::filesystem::create_directories(folder, error);

	Lite::Ref<Lite::Project> project = Lite::Project::New();
	project->GetConfig().Name = folder.filename().string();
	project->GetConfig().AssetDirectory = "assets";
	project->GetConfig().StartScene = "scenes/Untitled.scene";
	std::filesystem::path projectFile = folder / (project->GetConfig().Name + ".lite");
	if (!Lite::Project::SaveActive(projectFile))
	{
		Lite::Console::Log("Failed to create project");
		return;
	}

	m_ProjectPath = projectFile;
	NewScene();
	Lite::Console::Log(std::format("Project created: {}", projectFile.string()));
}

void EditorLayer::OpenProject(const std::filesystem::path& path)
{
	if (!Lite::Project::Load(path))
		return;

	m_ProjectPath = path;
	Lite::Ref<Lite::Project> project = Lite::Project::GetActive();
	Lite::Console::Log(std::format("Project loaded: {} ({})", project->GetConfig().Name, path.string()));

	const std::filesystem::path& start = project->GetConfig().StartScene;
	std::filesystem::path scene;
	if (!start.empty())
		scene = Lite::Project::GetAssetFileSystemPath(start);
	if (!scene.empty())
		OpenScene(scene.string());
	if (!m_Scene || !SceneMatches(scene))
		NewScene();
}

bool EditorLayer::SaveProjectTo(const std::filesystem::path& path)
{
	Lite::Ref<Lite::Project> project = Lite::Project::GetActive();
	if (!project)
		project = Lite::Project::New();

	project->GetConfig().Name = path.stem().string();
	if (m_Scene && !m_Scene->GetPath().empty())
	{
		std::filesystem::path assetRoot = path.parent_path() / project->GetConfig().AssetDirectory;
		std::error_code error;
		std::filesystem::path scene = std::filesystem::weakly_canonical(m_Scene->GetPath(), error);
		std::filesystem::path root = std::filesystem::weakly_canonical(assetRoot, error);
		if (Lite::FileSystem::Contains(root, scene))
		{
			std::filesystem::path relative = std::filesystem::relative(scene, root, error);
			if (!error)
				project->GetConfig().StartScene = relative.generic_string();
		}
	}

	if (!Lite::Project::SaveActive(path))
		return false;

	m_ProjectPath = path;
	Lite::Console::Log(std::format("Project saved: {}", path.string()));
	return true;
}

void EditorLayer::SaveProject()
{
	if (m_ProjectPath.empty())
	{
		m_Browser.ShowSaveProject();
		return;
	}

	SaveProjectTo(m_ProjectPath);
}

void EditorLayer::AssignScriptModule(const std::filesystem::path& picked)
{
	Lite::Ref<Lite::Project> project = Lite::Project::GetActive();
	if (!project)
		return;

	std::filesystem::path path = picked;
	if (path.is_absolute() && !project->GetProjectDirectory().empty())
	{
		std::error_code error;
		std::filesystem::path relative = std::filesystem::relative(path, project->GetProjectDirectory(), error);
		if (!error)
			path = relative;
	}

	project->GetConfig().ScriptModulePath = path.generic_string();
	m_ScriptSynced.clear();
	if (!m_ProjectPath.empty())
		SaveProject();
}

void EditorLayer::DrawScriptModule()
{
	Lite::Ref<Lite::Project> project = Lite::Project::GetActive();
	if (!project)
		return;

	const std::string current = project->GetConfig().ScriptModulePath.generic_string();
	if (m_ScriptSynced != current)
	{
		m_ScriptSynced = current;
		std::snprintf(m_ScriptModule, sizeof(m_ScriptModule), "%s", current.c_str());
	}

	ImGui::AlignTextToFramePadding();
	ImGui::TextUnformatted("Script module");
	ImGui::SetNextItemWidth(-80.0f);
	ImGui::InputText("##ScriptModule", m_ScriptModule, sizeof(m_ScriptModule));
	if (ImGui::IsItemDeactivatedAfterEdit())
		AssignScriptModule(m_ScriptModule);
	ImGui::SameLine();
	if (ImGui::Button("Browse"))
	{
		m_PickScript = true;
		std::filesystem::path start = project->GetProjectDirectory();
		if (!current.empty())
		{
			std::filesystem::path file = std::filesystem::path(current);
			if (!file.is_absolute())
				file = start / file;
			if (!file.parent_path().empty())
				start = file.parent_path();
		}
		m_ScriptBrowserDir = start.empty() ? Lite::FileSystem::ExecutableDirectory().string() : start.string();
	}

	if (m_PickScript)
	{
		ImGui::OpenPopup("Script Module");
		m_PickScript = false;
	}

	ImGui::SetNextWindowSize(ImVec2(520.0f, 360.0f), ImGuiCond_Appearing);
	if (!ImGui::BeginPopupModal("Script Module", nullptr, ImGuiWindowFlags_NoResize))
		return;

	if (ImGui::Button("Up"))
	{
		std::filesystem::path parent = std::filesystem::path(m_ScriptBrowserDir).parent_path();
		if (!parent.empty())
			m_ScriptBrowserDir = parent.string();
	}
	ImGui::SameLine();
	ImGui::TextUnformatted(m_ScriptBrowserDir.c_str());
	ImGui::BeginChild("##ScriptFiles", ImVec2(0.0f, -40.0f));

	std::error_code error;
	std::vector<std::filesystem::directory_entry> entries;
	for (const std::filesystem::directory_entry& entry : std::filesystem::directory_iterator(m_ScriptBrowserDir, error))
		entries.push_back(entry);
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

	for (const std::filesystem::directory_entry& entry : entries)
	{
		std::error_code kindError;
		const bool directory = entry.is_directory(kindError);
		std::string extension = entry.path().extension().string();
		for (char& character : extension)
			character = static_cast<char>(std::tolower(static_cast<unsigned char>(character)));
		if (!directory && extension != ".dll")
			continue;

		const std::string label = entry.path().filename().string();
		if (!ImGui::Selectable(label.c_str(), false, ImGuiSelectableFlags_AllowDoubleClick))
			continue;

		if (directory)
			m_ScriptBrowserDir = entry.path().string();
		else if (ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left))
		{
			AssignScriptModule(entry.path());
			ImGui::CloseCurrentPopup();
		}
	}

	ImGui::EndChild();
	if (ImGui::Button("Cancel"))
		ImGui::CloseCurrentPopup();
	ImGui::EndPopup();
}

void EditorLayer::NewScene()
{
	m_Scene = Lite::CreateScope<Lite::Scene>("Untitled");
	m_Scene->Create();
	m_History.Clear();
	Lite::Project::EnsureContentFolders();
	Lite::Scene::SetActive(m_Scene.get());
	m_Selected = 0;
	m_Inspector.Reset();

	if (Lite::Ref<Lite::Project> project = Lite::Project::GetActive())
	{
		if (!project->GetProjectDirectory().empty())
		{
			std::filesystem::path file = Lite::FileSystem::Unused(Lite::Project::GetAssetDirectory() / "scenes", "Untitled", ".scene");
			m_Scene->SetName(file.stem().string());
			if (m_Scene->SaveAs(file.string()) && project->GetConfig().StartScene.empty())
			{
				std::error_code error;
				std::filesystem::path relative = std::filesystem::relative(file, Lite::Project::GetAssetDirectory(), error);
				if (!error)
				{
					project->GetConfig().StartScene = relative.generic_string();
					if (!m_ProjectPath.empty())
						Lite::Project::SaveActive(m_ProjectPath);
				}
			}
		}
	}

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
	m_History.Clear();
	Lite::Project::EnsureContentFolders();
	Lite::Scene::SetActive(m_Scene.get());
	m_Gizmo = {};
	m_Selected = 0;
	for (Lite::Entity entity : m_Scene->GetEntities())
	{
		if (entity.Has<Lite::MeshComponent>())
		{
			m_Selected = entity.GetId();
			break;
		}
	}
	m_Inspector.Reset();
	SyncName();
}

bool EditorLayer::SceneMatches(const std::filesystem::path& path) const
{
	if (!m_Scene || m_Scene->GetPath().empty() || path.empty())
		return false;

	std::error_code error;
	std::filesystem::path current = std::filesystem::weakly_canonical(m_Scene->GetPath(), error);
	std::filesystem::path expected = std::filesystem::weakly_canonical(path, error);
	return !error && current == expected;
}

void EditorLayer::SyncName()
{
	m_NamedScene = m_Scene.get();
	const char* name = m_Scene != nullptr ? m_Scene->GetName().c_str() : "";
	std::snprintf(m_Name, sizeof(m_Name), "%s", name);
}

void EditorLayer::DuplicateSelected()
{
	if (m_Scene == nullptr || m_Selected == 0)
		return;

	Lite::Entity copy = m_Scene->DuplicateEntity(m_Selected);
	if (!copy)
		return;

	m_Selected = copy.GetId();
	m_Inspector.Reset();
	m_History.Commit(*m_Scene, m_Selected);
	Lite::Console::Log(std::format("Duplicated {}", copy.GetName()));
}

void EditorLayer::DeleteSelected()
{
	if (m_Scene == nullptr || m_Selected == 0)
		return;

	Lite::Entity entity = m_Scene->GetEntity(m_Selected);
	if (!entity)
		return;

	uint32_t next = m_Scene->NextEntity(m_Selected);
	std::string name = entity.GetName();
	m_Scene->DestroyEntity(m_Selected);
	m_Selected = next;
	m_Inspector.Reset();
	m_History.Commit(*m_Scene, m_Selected);
	Lite::Console::Log(std::format("Deleted {}", name));
}

void EditorLayer::UndoSelected()
{
	if (m_Scene == nullptr || !m_History.Undo(*m_Scene, m_Selected))
		return;

	m_Gizmo = {};
	m_Inspector.Reset();
	SyncName();
	Lite::Console::Log("Undone");
}

void EditorLayer::RedoSelected()
{
	if (m_Scene == nullptr || !m_History.Redo(*m_Scene, m_Selected))
		return;

	m_Gizmo = {};
	m_Inspector.Reset();
	SyncName();
	Lite::Console::Log("Redone");
}

void EditorLayer::SaveScene()
{
	if (m_Scene == nullptr)
		return;

	if (m_Scene->GetPath().empty())
		m_Browser.ShowSave();
	else if (m_Scene->Save())
		SyncName();
}

void EditorLayer::ApplyPlayCamera()
{
	float aspect = Lite::kDefaultAspect;
	if (m_ViewportH > 1.0f)
		aspect = Lite::AspectRatio(m_ViewportW, m_ViewportH);
	else
	{
		VkExtent2D extent = Lite::Renderer::GetExtent();
		aspect = Lite::AspectRatio(static_cast<float>(extent.width), static_cast<float>(extent.height));
	}

	Lite::CameraComponent camera;
	camera.Size = m_ViewSize;
	Lite::Mat4 viewProjection = m_Scene->ViewProjection(aspect, m_Camera.GetTransform(), camera);
	m_ViewProjection = Lite::FitViewport(viewProjection, m_ViewportX, m_ViewportY, m_ViewportW, m_ViewportH, m_WindowW, m_WindowH);
	Lite::Renderer2D::SetViewProjection(m_ViewProjection);
}

void EditorLayer::DropSprite(const std::string& path, float mouseX, float mouseY)
{
	if (m_Scene == nullptr || path.empty())
		return;

	const Lite::Vec2 world = Lite::ScreenToWorld(m_ViewProjection, m_WindowW, m_WindowH, mouseX, mouseY).value_or(Lite::Vec2 {});
	float aspect = m_ViewportH > 1.0f ? m_ViewportW / m_ViewportH : 1.0f;
	Lite::Vec2 viewCenter { m_Camera.GetPosition().x, m_Camera.GetPosition().y };
	Lite::Scene::SpritePlacement placed = m_Scene->PlaceSprite(path, world, viewCenter, m_ViewSize, aspect);
	if (placed.Entity == 0)
		return;

	m_Gizmo = {};
	m_Selected = placed.Entity;
	m_Inspector.Reset();
	std::string name = m_Scene->GetEntity(placed.Entity).GetName();
	if (!placed.Loaded)
		Lite::Console::Log(std::format("Failed to load texture {}", path));
	else if (placed.Background)
		Lite::Console::Log(std::format("Placed {} as the background", name));
	else
		Lite::Console::Log(std::format("Placed {}", name));
}

void EditorLayer::SaveSelectedPrefab()
{
	if (m_Scene == nullptr || m_Selected == 0)
		return;
	if (Lite::Project::GetActive() == nullptr)
	{
		Lite::Console::Log("Failed to save prefab: open a project first");
		return;
	}

	Lite::Entity entity = m_Scene->GetEntity(m_Selected);
	if (!entity)
		return;

	std::string name = entity.GetName();
	if (name.empty())
		name = "Prefab";
	for (char& character : name)
	{
		if (std::string("\\/:*?\"<>|").find(character) != std::string::npos)
			character = '_';
	}

	Lite::Project::EnsureContentFolders();
	std::filesystem::path file = Lite::Project::GetAssetDirectory() / "prefabs" / (name + ".prefab");
	m_Scene->SavePrefab(entity.GetId(), file);
}

void EditorLayer::SaveSelectedMaterial()
{
	if (m_Scene == nullptr || m_Selected == 0)
		return;
	if (Lite::Project::GetActive() == nullptr)
	{
		Lite::Console::Log("Failed to save material: open a project first");
		return;
	}

	Lite::Entity entity = m_Scene->GetEntity(m_Selected);
	if (!entity || !entity.Has<Lite::MaterialComponent>())
	{
		Lite::Console::Log("Failed to save material: the entity has no material");
		return;
	}

	std::string name = entity.GetName();
	if (name.empty())
		name = "Material";
	for (char& character : name)
	{
		if (std::string("\\/:*?\"<>|").find(character) != std::string::npos)
			character = '_';
	}

	Lite::Project::EnsureContentFolders();
	std::filesystem::path file = Lite::Project::GetAssetDirectory() / "materials" / (name + ".material");
	m_Scene->SaveMaterial(entity.GetId(), file);
}

void EditorLayer::PlacePrefab(const std::string& path, float mouseX, float mouseY, bool atMouse)
{
	if (m_Scene == nullptr || path.empty())
		return;

	std::optional<Lite::Vec2> position;
	if (atMouse)
		position = Lite::ScreenToWorld(m_ViewProjection, m_WindowW, m_WindowH, mouseX, mouseY);

	Lite::Entity entity = m_Scene->PlacePrefab(path, position);
	if (!entity)
		return;

	m_Gizmo = {};
	m_Selected = entity.GetId();
	m_Inspector.Reset();
}

void EditorLayer::PickObject(float mouseX, float mouseY)
{
	if (m_Scene == nullptr)
		return;

	uint32_t id = m_Scene->Pick(m_ViewProjection, mouseX, mouseY, m_WindowW, m_WindowH);
	if (id == 0)
		return;

	if (m_Selected != id)
		Lite::Console::Log(std::format("Selected: {}", m_Scene->GetEntity(id).GetName()));
	m_Selected = id;
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
		bool shift = Lite::Input::IsKeyPressed(Lite::Key::LeftShift) || Lite::Input::IsKeyPressed(Lite::Key::RightShift);
		if (control && shift && key.GetKeyCode() == Lite::Key::N)
		{
			NewProject();
			return true;
		}

		if (control && shift && key.GetKeyCode() == Lite::Key::S)
		{
			SaveProject();
			return true;
		}

		if (control && shift && key.GetKeyCode() == Lite::Key::O)
		{
			m_Browser.ShowOpenProject();
			return true;
		}

		if (control && key.GetKeyCode() == Lite::Key::N)
		{
			NewScene();
			return true;
		}

		if (control && key.GetKeyCode() == Lite::Key::S)
		{
			SaveScene();
			return true;
		}

		if (control && key.GetKeyCode() == Lite::Key::O)
		{
			m_Browser.ShowOpen();
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

		if (key.GetKeyCode() == Lite::Key::F7 && m_Scene)
		{
			m_Scene->Restart();
			return true;
		}

		return false;
	});

	dispatcher.Dispatch<Lite::MouseScrolledEvent>([this](Lite::MouseScrolledEvent& scroll)
	{
		float steps = scroll.GetYOffset();
		if (steps == 0.0f || (m_Scene && m_Scene->IsPlaying()))
			return false;

		Lite::ZoomCamera(m_ViewSize, steps);
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
		if (ImGui::MenuItem("New Project", "Ctrl+Shift+N"))
			NewProject();
		if (ImGui::MenuItem("Open Project...", "Ctrl+Shift+O"))
			m_Browser.ShowOpenProject();
		if (ImGui::MenuItem("Save Project", "Ctrl+Shift+S"))
			SaveProject();
		ImGui::Separator();
		if (ImGui::MenuItem("New Scene", "Ctrl+N"))
			NewScene();
		if (ImGui::MenuItem("Open Scene...", "Ctrl+O"))
			m_Browser.ShowOpen();
		if (ImGui::MenuItem("Save Scene", "Ctrl+S", false, m_Scene != nullptr))
			SaveScene();
		if (ImGui::MenuItem("Save Scene As...", nullptr, false, m_Scene != nullptr))
			m_Browser.ShowSave();
		if (ImGui::MenuItem("Save Prefab", nullptr, false, m_Scene != nullptr && m_Selected != 0))
			SaveSelectedPrefab();
		if (ImGui::MenuItem("Save Material", nullptr, false, m_Scene != nullptr && m_Selected != 0))
			SaveSelectedMaterial();
		ImGui::EndMenu();
	}

	if (ImGui::BeginMenu("Edit"))
	{
		if (ImGui::MenuItem("Undo", "Ctrl+Z", false, m_Scene != nullptr))
			UndoSelected();
		if (ImGui::MenuItem("Redo", "Ctrl+Y", false, m_Scene != nullptr))
			RedoSelected();
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

	float transport = 28.0f * 3.0f + 8.0f * 2.0f;
	ImGui::SetCursorPosX((ImGui::GetWindowWidth() - transport) * 0.5f);
	bool playing = m_Scene && m_Scene->IsPlaying();
	bool paused = m_Scene && m_Scene->GetPlayback() == Lite::ScenePlayback::Paused;
	if (TransportButton("##Play", m_Scene != nullptr, playing, ImVec4(0.18f, 0.48f, 0.28f, 1.0f), TransportSymbol::Play))
		m_Scene->Play();
	if (ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenDisabled))
		ImGui::SetTooltip("Play");
	ImGui::SameLine(0.0f, 8.0f);
	if (TransportButton("##Pause", m_Scene != nullptr, paused, ImVec4(0.45f, 0.36f, 0.14f, 1.0f), TransportSymbol::Pause))
		m_Scene->Pause();
	if (ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenDisabled))
		ImGui::SetTooltip("Pause");
	ImGui::SameLine(0.0f, 8.0f);
	if (TransportButton("##Reset", m_Scene != nullptr, false, ImVec4(0.16f, 0.17f, 0.21f, 1.0f), TransportSymbol::Reset))
		m_Scene->Restart();
	if (ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenDisabled))
		ImGui::SetTooltip("Reset");

	ImGui::EndMainMenuBar();
	ImGui::PopStyleVar();
}

void EditorLayer::OnImGuiRender()
{
	ImGuiIO& io = ImGui::GetIO();
	m_WindowW = io.DisplaySize.x;
	m_WindowH = io.DisplaySize.y;

	if (m_Scene != nullptr)
		m_History.Observe(*m_Scene, m_Selected, ImGui::GetActiveID() == 0);

	if (!io.WantTextInput)
	{
		if (io.KeyCtrl && ImGui::IsKeyPressed(ImGuiKey_Z, false))
			UndoSelected();
		if (io.KeyCtrl && ImGui::IsKeyPressed(ImGuiKey_Y, false))
			RedoSelected();
		if (ImGui::IsKeyPressed(ImGuiKey_Delete, true))
			DeleteSelected();
		if (io.KeyCtrl && ImGui::IsKeyPressed(ImGuiKey_D, false))
			DuplicateSelected();
	}

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
	m_Inspector.Draw(m_Selected, m_History);
	DrawConsole();
	m_Explorer.Draw(
		[this](const std::string& path) { OpenScene(path); },
		[this](const std::string& path) { OpenProject(path); },
		[this](const std::string& path) { PlacePrefab(path, 0.0f, 0.0f, false); });
	if (m_ShowInfo)
	{
		Lite::DrawInstrumentation(!m_InfoPlaced);
		m_InfoPlaced = true;
	}
	m_Browser.Draw(
		m_Scene.get(),
		[this](const std::string& path) { OpenScene(path); },
		[this] { SyncName(); },
		[this](const std::string& path) { OpenProject(path); },
		[this](const std::string& path) { return SaveProjectTo(path); });
}

void EditorLayer::DrawScene()
{
	ImGui::Begin("Scene");
	if (Lite::Ref<Lite::Project> project = Lite::Project::GetActive())
	{
		ImGui::TextDisabled("%s", project->GetConfig().Name.c_str());
		if (!m_ProjectPath.empty())
			ImGui::TextWrapped("%s", m_ProjectPath.string().c_str());
		DrawScriptModule();
	}

	if (m_Scene)
	{
		if (m_NamedScene != m_Scene.get())
			SyncName();
		ImGui::SetNextItemWidth(-1.0f);
		if (ImGui::InputText("##SceneName", m_Name, sizeof(m_Name)))
			m_Scene->SetName(m_Name);
		if (ImGui::IsItemDeactivatedAfterEdit())
			m_History.Commit(*m_Scene, m_Selected);
	}

	if (m_Scene == nullptr)
	{
		ImGui::TextDisabled("No scene");
		ImGui::End();
		return;
	}

	const char* playback = Lite::PlaybackName(m_Scene->GetPlayback());
	ImGui::TextDisabled("%s  %s", m_Scene->GetName().c_str(), playback);
	if (!m_Scene->GetPath().empty())
		ImGui::TextWrapped("%s", m_Scene->GetPath().c_str());

	size_t componentCount = 0;
	const Lite::ComponentEntry* catalog = Lite::ComponentCatalog(componentCount);
	ImGuiTreeNodeFlags planeFlags = ImGuiTreeNodeFlags_DefaultOpen | ImGuiTreeNodeFlags_OpenOnArrow;
	bool sceneChanged = false;

	struct PlaneDraft
	{
		uint32_t Id = 0;
		char Name[128] {};
	};
	static std::vector<PlaneDraft> drafts;
	const std::vector<Lite::Scene::Plane>& planes = m_Scene->GetPlanes();
	drafts.erase(std::remove_if(drafts.begin(), drafts.end(), [&](const PlaneDraft& draft)
	{
		for (const Lite::Scene::Plane& plane : planes)
		{
			if (plane.Id == draft.Id)
				return false;
		}
		return true;
	}), drafts.end());

	auto describeRemoval = [&](const std::string& name, int entityCount, Lite::Scene::PlaneRemove result)
	{
		switch (result)
		{
			case Lite::Scene::PlaneRemove::Removed:
				m_PlaneNotice = std::format("Removed {}.", name);
				break;
			case Lite::Scene::PlaneRemove::MovedToWorld:
				m_PlaneNotice = std::format("Moved {} {} to World and removed {}.", entityCount, entityCount == 1 ? "entity" : "entities", name);
				break;
			case Lite::Scene::PlaneRemove::HasEntities:
				m_PlaneNotice = "Move the entities off World before deleting it.";
				break;
			case Lite::Scene::PlaneRemove::LastPlane:
				m_PlaneNotice = "The scene needs a plane.";
				break;
			default:
				break;
		}
		if (!m_PlaneNotice.empty())
			Lite::Console::Log(m_PlaneNotice);
	};

	for (const Lite::Scene::Plane& plane : planes)
	{
		PlaneDraft* draft = nullptr;
		for (PlaneDraft& item : drafts)
		{
			if (item.Id == plane.Id)
				draft = &item;
		}
		if (draft == nullptr)
		{
			drafts.push_back({});
			draft = &drafts.back();
			draft->Id = plane.Id;
			std::snprintf(draft->Name, sizeof(draft->Name), "%s", plane.Name.c_str());
		}

		std::string planeId = std::format("##plane{}", plane.Id);
		bool planeOpen = ImGui::TreeNodeEx(planeId.c_str(), planeFlags);
		const int entityCount = static_cast<int>(m_Scene->GetEntities(plane.Id).size());
		const bool isWorld = m_Scene->FindPlane("World") == plane.Id;
		if (ImGui::BeginPopupContextItem())
		{
			const bool lastPlane = planes.size() <= 1;
			const bool blocked = lastPlane || (entityCount > 0 && isWorld);
			const char* label = entityCount > 0 && !isWorld ? "Move to World and Delete" : "Delete";
			if (ImGui::MenuItem(label, nullptr, false, !blocked))
			{
				std::string name = plane.Name;
				Lite::Scene::PlaneRemove result = m_Scene->RemovePlane(plane.Id);
				describeRemoval(name, entityCount, result);
				sceneChanged = result == Lite::Scene::PlaneRemove::Removed || result == Lite::Scene::PlaneRemove::MovedToWorld;
			}
			if (lastPlane)
				ImGui::TextDisabled("The scene needs a plane.");
			else if (entityCount > 0 && isWorld)
				ImGui::TextDisabled("Move the entities off World first.");
			ImGui::EndPopup();
		}

		ImGui::SameLine();
		ImGui::SetNextItemWidth(-72.0f);
		ImGui::PushID(static_cast<int>(plane.Id));
		const ImGuiID nameId = ImGui::GetID("##PlaneName");
		if (ImGui::GetActiveID() != nameId && std::strcmp(draft->Name, plane.Name.c_str()) != 0)
			std::snprintf(draft->Name, sizeof(draft->Name), "%s", plane.Name.c_str());
		ImGui::InputText("##PlaneName", draft->Name, sizeof(draft->Name));
		if (ImGui::IsItemDeactivatedAfterEdit())
		{
			if (draft->Name[0] == '\0' || !m_Scene->SetPlaneName(plane.Id, draft->Name))
			{
				m_PlaneNotice = draft->Name[0] == '\0' ? "A plane needs a name." : "A plane already has that name.";
				Lite::Console::Log(m_PlaneNotice);
				std::snprintf(draft->Name, sizeof(draft->Name), "%s", plane.Name.c_str());
			}
			else
				m_PlaneNotice.clear();
		}
		ImGui::SameLine();
		ImGui::SetNextItemWidth(64.0f);
		int order = plane.Order;
		if (ImGui::DragInt("##PlaneOrder", &order, 0.1f))
			m_Scene->SetPlaneOrder(plane.Id, order);
		if (ImGui::IsItemHovered())
			ImGui::SetTooltip("Draw order. A higher plane renders in front.");
		ImGui::PopID();
		if (sceneChanged)
		{
			if (planeOpen)
				ImGui::TreePop();
			break;
		}
		if (!planeOpen)
			continue;

		for (Lite::Entity entity : m_Scene->GetEntities(plane.Id))
		{
			std::string label = std::format("{}##entity{}", entity.GetName(), entity.GetId());
			ImGuiTreeNodeFlags entityFlags = ImGuiTreeNodeFlags_DefaultOpen | ImGuiTreeNodeFlags_OpenOnArrow | ImGuiTreeNodeFlags_SpanAvailWidth;
			if (m_Selected == entity.GetId())
				entityFlags |= ImGuiTreeNodeFlags_Selected;

			bool entityOpen = ImGui::TreeNodeEx(label.c_str(), entityFlags);
			if (ImGui::IsItemClicked() && !ImGui::IsItemToggledOpen())
				m_Selected = entity.GetId();
			if (ImGui::BeginPopupContextItem())
			{
				m_Selected = entity.GetId();
				if (ImGui::MenuItem("Duplicate", "Ctrl+D"))
				{
					DuplicateSelected();
					sceneChanged = true;
				}
				if (ImGui::MenuItem("Save Prefab"))
					SaveSelectedPrefab();
				if (ImGui::MenuItem("Save Material"))
					SaveSelectedMaterial();
				if (ImGui::MenuItem("Delete", "Del"))
				{
					DeleteSelected();
					sceneChanged = true;
				}
				ImGui::EndPopup();
			}

			if (sceneChanged)
			{
				if (entityOpen)
					ImGui::TreePop();
				break;
			}

			if (!entityOpen)
				continue;

			for (size_t index = 0; index < componentCount; ++index)
			{
				if (const char* component = catalog[index].Label(entity))
					ImGui::TextDisabled("%s", component);
			}
			ImGui::TreePop();
		}

		ImGui::TreePop();
		if (sceneChanged)
			break;
	}

	if (ImGui::Button("Add Plane"))
	{
		m_Scene->AddPlane();
		m_PlaneNotice.clear();
	}
	if (!m_PlaneNotice.empty())
		ImGui::TextWrapped("%s", m_PlaneNotice.c_str());

	ImGui::Separator();
	ImGui::BeginDisabled(m_Selected == 0);
	if (ImGui::Button("Duplicate"))
		DuplicateSelected();
	ImGui::SameLine();
	if (ImGui::Button("Delete"))
		DeleteSelected();
	ImGui::EndDisabled();

	ImGuiWindow* window = ImGui::GetCurrentWindow();
	if (ImGui::BeginDragDropTargetCustom(window->InnerRect, ImGui::GetID("##SceneFileDrop")))
	{
		if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("LITE_SCENE"))
			OpenScene(static_cast<const char*>(payload->Data));
		ImGui::EndDragDropTarget();
	}
	ImGui::End();
}


void EditorLayer::DrawGizmo()
{
	if (m_Scene == nullptr || m_Selected == 0)
		return;

	Lite::Entity entity = m_Scene->GetEntity(m_Selected);
	Lite::GizmoLayout layout;
	if (!entity || !Lite::BuildGizmo(m_ViewProjection, m_WindowW, m_WindowH, entity, layout))
		return;

	ImDrawList* draw = ImGui::GetWindowDrawList();
	ImVec2 mouse = ImGui::GetIO().MousePos;
	Lite::GizmoHot hot;
	if (m_Gizmo.Action != Lite::GizmoAction::None)
	{
		hot.Action = m_Gizmo.Action;
		hot.Corner = m_Gizmo.Corner;
	}
	else if (ImGui::IsWindowHovered(ImGuiHoveredFlags_AllowWhenBlockedByActiveItem))
	{
		hot = Lite::HitGizmo(layout, { mouse.x, mouse.y }, Lite::ScreenToWorld(m_ViewProjection, m_WindowW, m_WindowH, mouse.x, mouse.y));
	}

	if (hot.Action == Lite::GizmoAction::Move)
		ImGui::SetMouseCursor(ImGuiMouseCursor_ResizeAll);
	else if (hot.Action == Lite::GizmoAction::Rotate)
		ImGui::SetMouseCursor(ImGuiMouseCursor_Hand);
	else if (hot.Action == Lite::GizmoAction::Scale)
		ImGui::SetMouseCursor(ImGuiMouseCursor_ResizeNWSE);

	ImVec2 screen[4];
	for (int index = 0; index < layout.Count; ++index)
		screen[index] = { layout.Screen[index].x, layout.Screen[index].y };
	ImVec2 center { layout.Center.x, layout.Center.y };

	const ImU32 outline = IM_COL32(236, 240, 246, 210);
	const bool hotRing = hot.Action == Lite::GizmoAction::Rotate;
	const ImU32 ring = hotRing ? IM_COL32(170, 220, 255, 255) : IM_COL32(110, 176, 240, 210);
	draw->AddPolyline(screen, layout.Count, outline, 1.6f, ImDrawFlags_Closed);
	draw->AddCircle(center, layout.Ring, ring, 48, hotRing ? 2.4f : 1.5f);

	auto axis = [&](Lite::Vec2 world, ImU32 color)
	{
		const std::optional<Lite::Vec2> point = Lite::WorldToScreen(m_ViewProjection, m_WindowW, m_WindowH, world);
		if (!point)
			return;
		ImVec2 direction { point->x - center.x, point->y - center.y };
		float length = std::sqrt(direction.x * direction.x + direction.y * direction.y);
		if (length <= 0.001f)
			return;
		direction.x = direction.x / length * 22.0f;
		direction.y = direction.y / length * 22.0f;
		draw->AddLine(center, { center.x + direction.x, center.y + direction.y }, color, 2.0f);
	};

	if (Lite::TransformComponent* transform = entity.Get<Lite::TransformComponent>())
	{
		Lite::Vec3 axisX = transform->Local.TransformPoint({ 1.0f, 0.0f, 0.0f });
		Lite::Vec3 axisY = transform->Local.TransformPoint({ 0.0f, 1.0f, 0.0f });
		axis({ axisX.x, axisX.y }, IM_COL32(230, 74, 74, 230));
		axis({ axisY.x, axisY.y }, IM_COL32(78, 206, 96, 230));
	}

	for (int index = 0; index < layout.Count; ++index)
	{
		ImVec2 corner = screen[index];
		ImVec2 min { corner.x - Lite::kGizmoHandle, corner.y - Lite::kGizmoHandle };
		ImVec2 max { corner.x + Lite::kGizmoHandle, corner.y + Lite::kGizmoHandle };
		ImU32 fill = hot.Action == Lite::GizmoAction::Scale && index == hot.Corner ? IM_COL32(255, 214, 96, 255) : IM_COL32(246, 248, 252, 240);
		draw->AddRectFilled(min, max, fill);
		draw->AddRect(min, max, IM_COL32(24, 28, 34, 255));
	}
}

void EditorLayer::DrawViewport()
{
	ImGui::Begin("Viewport", nullptr, ImGuiWindowFlags_NoBackground | ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse);
	ImVec2 origin = ImGui::GetCursorScreenPos();
	ImVec2 size = ImGui::GetContentRegionAvail();
	m_ViewportX = origin.x;
	m_ViewportY = origin.y;
	m_ViewportW = size.x;
	m_ViewportH = size.y;
	ImGui::InvisibleButton("##ViewportPick", size);
	if (ImGui::BeginDragDropTarget())
	{
		if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("LITE_SCENE"))
			OpenScene(static_cast<const char*>(payload->Data));
		if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("LITE_TEXTURE"))
		{
			ImVec2 mouse = ImGui::GetIO().MousePos;
			DropSprite(static_cast<const char*>(payload->Data), mouse.x, mouse.y);
		}
		if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("LITE_PREFAB"))
		{
			ImVec2 mouse = ImGui::GetIO().MousePos;
			PlacePrefab(static_cast<const char*>(payload->Data), mouse.x, mouse.y, true);
		}
		ImGui::EndDragDropTarget();
	}
	ImVec2 mouse = ImGui::GetIO().MousePos;
	if (m_Gizmo.Action != Lite::GizmoAction::None)
	{
		if (m_Scene != nullptr && ImGui::IsMouseDown(ImGuiMouseButton_Left))
			Lite::ApplyGizmo(*m_Scene, m_ViewProjection, m_WindowW, m_WindowH, m_Selected, mouse.x, mouse.y, m_Gizmo);
		else
		{
			m_Gizmo = {};
			if (m_Scene != nullptr)
				m_History.Commit(*m_Scene, m_Selected);
		}
	}
	else if (ImGui::IsItemClicked(ImGuiMouseButton_Left) && m_Scene != nullptr)
	{
		if (!Lite::BeginGizmo(*m_Scene, m_ViewProjection, m_WindowW, m_WindowH, m_Selected, mouse.x, mouse.y, m_Gizmo))
			PickObject(mouse.x, mouse.y);
	}
	DrawGizmo();

	const char* name = m_Scene != nullptr ? m_Scene->GetName().c_str() : "No scene";
	const char* playback = m_Scene != nullptr ? Lite::PlaybackName(m_Scene->GetPlayback()) : "stopped";
	ImGui::SetCursorScreenPos(origin);
	ImGui::TextDisabled("%s  %s  %.0f x %.0f", name, playback, size.x, size.y);
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