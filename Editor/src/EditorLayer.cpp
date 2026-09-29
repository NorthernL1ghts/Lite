#include <EditorLayer.h>

#include <Lite/Core/Events/KeyEvent.h>
#include <Lite/Core/Events/MouseEvent.h>
#include <Lite/Core/Log/Logger.h>
#include <Lite/Core/Profile/Profiler.h>
#include <Lite/ImGui/Instrumentation.h>
#include <Lite/Input/Input.h>
#include <Lite/Input/KeyCodes.h>
#include <Lite/Project/Project.h>
#include <Lite/Renderer/MeshShape.h>
#include <Lite/Renderer/Renderer.h>
#include <Lite/Renderer/Renderer2D.h>
#include <Lite/Scene/Console.h>
#include <Lite/Scene/SceneCamera.h>

#include <imgui.h>
#include <imgui_internal.h>

#include <cmath>
#include <cstdio>
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
	Lite::Ref<Lite::Project> project = Lite::Project::New();
	project->GetConfig().Name = "Untitled";
	project->GetConfig().AssetDirectory = "assets";
	m_ProjectPath.clear();
	NewScene();
	Lite::Console::Log("Project created: Untitled");
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
		std::filesystem::path relative = std::filesystem::relative(scene, root, error);
		bool inside = !error && (relative.empty() || relative.begin()->string() != "..");
		if (inside)
			project->GetConfig().StartScene = relative.generic_string();
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

void EditorLayer::NewScene()
{
	m_Scene = Lite::CreateScope<Lite::Scene>("Untitled");
	m_Scene->Create();
	Lite::Scene::SetActive(m_Scene.get());
	m_Selected = 0;
	m_Inspector.Reset();
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
	Lite::Console::Log(std::format("Duplicated {}", copy.GetName()));
}

void EditorLayer::DeleteSelected()
{
	if (m_Scene == nullptr || m_Selected == 0)
		return;

	Lite::Entity entity = m_Scene->GetEntity(m_Selected);
	if (!entity)
		return;

	uint32_t next = 0;
	uint32_t previous = 0;
	bool passed = false;
	for (Lite::Entity item : m_Scene->GetEntities())
	{
		if (item.GetId() == m_Selected)
		{
			passed = true;
			continue;
		}

		if (!passed)
			previous = item.GetId();
		else
		{
			next = item.GetId();
			break;
		}
	}

	std::string name = entity.GetName();
	m_Scene->DestroyEntity(m_Selected);
	m_Selected = next != 0 ? next : previous;
	m_Inspector.Reset();
	Lite::Console::Log(std::format("Deleted {}", name));
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

void EditorLayer::PickObject(float mouseX, float mouseY)
{
	if (m_Scene == nullptr || m_Scene->IsPlaying())
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

	if (!io.WantTextInput)
	{
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
	m_Inspector.Draw(m_Selected);
	DrawConsole();
	m_Explorer.Draw(
		[this](const std::string& path) { OpenScene(path); },
		[this](const std::string& path) { OpenProject(path); });
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
	}

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

	const char* playback = Lite::PlaybackName(m_Scene->GetPlayback());
	ImGui::TextDisabled("%s  %s", m_Scene->GetName().c_str(), playback);
	if (!m_Scene->GetPath().empty())
		ImGui::TextWrapped("%s", m_Scene->GetPath().c_str());

	size_t componentCount = 0;
	const Lite::ComponentEntry* catalog = Lite::ComponentCatalog(componentCount);
	ImGuiTreeNodeFlags planeFlags = ImGuiTreeNodeFlags_DefaultOpen | ImGuiTreeNodeFlags_OpenOnArrow | ImGuiTreeNodeFlags_SpanAvailWidth;
	bool sceneChanged = false;
	for (const Lite::Scene::Plane& plane : m_Scene->GetPlanes())
	{
		std::string planeLabel = std::format("{}##plane{}", plane.Name, plane.Id);
		if (!ImGui::TreeNodeEx(planeLabel.c_str(), planeFlags))
			continue;

		for (Lite::Entity entity : m_Scene->GetEntities(plane.Id))
		{
			std::string label = std::format("{}##entity{}", entity.GetName(), entity.GetId());
			ImGuiTreeNodeFlags entityFlags = ImGuiTreeNodeFlags_DefaultOpen | ImGuiTreeNodeFlags_OpenOnArrow | ImGuiTreeNodeFlags_SpanAvailWidth;
			if (m_Selected == entity.GetId())
				entityFlags |= ImGuiTreeNodeFlags_Selected;

			bool open = ImGui::TreeNodeEx(label.c_str(), entityFlags);
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
				if (ImGui::MenuItem("Delete", "Del"))
				{
					DeleteSelected();
					sceneChanged = true;
				}
				ImGui::EndPopup();
			}

			if (sceneChanged)
			{
				if (open)
					ImGui::TreePop();
				break;
			}

			if (!open)
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

	ImGui::Separator();
	ImGui::BeginDisabled(m_Selected == 0);
	if (ImGui::Button("Duplicate"))
		DuplicateSelected();
	ImGui::SameLine();
	if (ImGui::Button("Delete"))
		DeleteSelected();
	ImGui::EndDisabled();
	ImGui::End();
}

namespace {

	constexpr float kHandleSize = 7.0f;
	constexpr float kRingGap = 16.0f;
	constexpr float kRingHit = 7.0f;
	constexpr float kMinRing = 36.0f;
	constexpr float kPi = 3.14159265f;

	struct GizmoLayout
	{
		bool Ok = false;
		Lite::Vec2 CenterWorld {};
		ImVec2 Center {};
		Lite::Vec2 World[4] {};
		ImVec2 Screen[4] {};
		int Count = 0;
		float Ring = kMinRing;
	};

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

	float WrapAngle(float radians)
	{
		while (radians > kPi)
			radians -= kPi * 2.0f;
		while (radians < -kPi)
			radians += kPi * 2.0f;
		return radians;
	}

	float ScreenDistance(ImVec2 left, ImVec2 right)
	{
		float x = left.x - right.x;
		float y = left.y - right.y;
		return std::sqrt(x * x + y * y);
	}

	bool Project(const Lite::Mat4& viewProjection, float windowW, float windowH, Lite::Vec2 world, ImVec2& screen)
	{
		if (windowW <= 1.0f || windowH <= 1.0f)
			return false;

		Lite::Vec4 clip = viewProjection * Lite::Vec4(world.x, world.y, 0.0f, 1.0f);
		if (clip.w == 0.0f)
			return false;

		clip /= clip.w;
		screen.x = (clip.x * 0.5f + 0.5f) * windowW;
		screen.y = (1.0f - (clip.y * 0.5f + 0.5f)) * windowH;
		return true;
	}

	bool Unproject(const Lite::Mat4& viewProjection, float windowW, float windowH, float mouseX, float mouseY, Lite::Vec2& world)
	{
		if (windowW <= 1.0f || windowH <= 1.0f)
			return false;

		float ndcX = (mouseX / windowW) * 2.0f - 1.0f;
		float ndcY = 1.0f - (mouseY / windowH) * 2.0f;
		Lite::Vec4 point = viewProjection.Inverse() * Lite::Vec4(ndcX, ndcY, 0.0f, 1.0f);
		if (point.w != 0.0f)
			point /= point.w;
		world = { point.x, point.y };
		return true;
	}

	bool BuildGizmo(const Lite::Mat4& viewProjection, float windowW, float windowH, Lite::Entity entity, GizmoLayout& layout)
	{
		Lite::TransformComponent* transform = entity.Get<Lite::TransformComponent>();
		if (transform == nullptr)
			return false;

		const Lite::Vec2* local = Lite::kQuadCorners;
		int count = 4;
		if (Lite::MeshComponent* mesh = entity.Get<Lite::MeshComponent>())
		{
			if (mesh->Type == Lite::MeshType::Triangle)
			{
				local = Lite::kTriangleCorners;
				count = 3;
			}
		}

		layout = {};
		layout.CenterWorld = { transform->Local.Position.x, transform->Local.Position.y };
		if (!Project(viewProjection, windowW, windowH, layout.CenterWorld, layout.Center))
			return false;

		layout.Count = count;
		float reach = 0.0f;
		for (int index = 0; index < count; ++index)
		{
			Lite::Vec3 transformed = transform->Local.TransformPoint({ local[index].x, local[index].y, 0.0f });
			layout.World[index] = { transformed.x, transformed.y };
			if (!Project(viewProjection, windowW, windowH, layout.World[index], layout.Screen[index]))
				return false;
			reach = std::max(reach, ScreenDistance(layout.Center, layout.Screen[index]));
		}

		layout.Ring = std::max(reach + kRingGap, kMinRing);
		layout.Ok = true;
		return true;
	}

	int HitScale(const GizmoLayout& layout, ImVec2 mouse)
	{
		int hit = -1;
		float best = kHandleSize + 3.0f;
		for (int index = 0; index < layout.Count; ++index)
		{
			float distance = ScreenDistance(mouse, layout.Screen[index]);
			if (distance > best)
				continue;
			best = distance;
			hit = index;
		}

		return hit;
	}

	bool HitRing(const GizmoLayout& layout, ImVec2 mouse)
	{
		float distance = ScreenDistance(mouse, layout.Center);
		return std::abs(distance - layout.Ring) <= kRingHit;
	}

	Lite::Vec2 LocalOnPlane(const Lite::Transform& transform, Lite::Vec2 world)
	{
		Lite::Vec3 offset { world.x - transform.Position.x, world.y - transform.Position.y, 0.0f };
		Lite::Vec3 local = transform.Rotation.Normalized().Conjugate().Rotate(offset);
		return { local.x, local.y };
	}

}

bool EditorLayer::BeginGizmo(float mouseX, float mouseY)
{
	m_Gizmo = GizmoAction::None;
	if (m_Scene == nullptr || m_Selected == 0)
		return false;

	Lite::Entity entity = m_Scene->GetEntity(m_Selected);
	Lite::TransformComponent* transform = entity ? entity.Get<Lite::TransformComponent>() : nullptr;
	if (transform == nullptr)
		return false;

	GizmoLayout layout;
	if (!BuildGizmo(m_ViewProjection, m_WindowW, m_WindowH, entity, layout))
		return false;

	ImVec2 mouse { mouseX, mouseY };
	Lite::Vec2 world;
	if (!Unproject(m_ViewProjection, m_WindowW, m_WindowH, mouseX, mouseY, world))
		return false;

	int corner = HitScale(layout, mouse);
	if (corner >= 0)
		m_Gizmo = GizmoAction::Scale;
	else if (HitRing(layout, mouse))
		m_Gizmo = GizmoAction::Rotate;
	else if (Contains(world, layout.World, layout.Count))
		m_Gizmo = GizmoAction::Move;
	else
		return false;

	m_GizmoCorner = corner;
	m_GizmoMouse = world;
	m_GizmoPosition = transform->Local.Position;
	m_GizmoRotation = transform->Local.GetRotationZ();
	m_GizmoScale = transform->Local.Scale;
	m_GizmoAngle = std::atan2(world.y - m_GizmoPosition.y, world.x - m_GizmoPosition.x);
	m_GizmoSpin = 0.0f;
	return true;
}

void EditorLayer::ApplyGizmo(float mouseX, float mouseY)
{
	if (m_Scene == nullptr || m_Gizmo == GizmoAction::None)
		return;

	Lite::Entity entity = m_Scene->GetEntity(m_Selected);
	Lite::TransformComponent* transform = entity ? entity.Get<Lite::TransformComponent>() : nullptr;
	Lite::Vec2 world;
	if (transform == nullptr || !Unproject(m_ViewProjection, m_WindowW, m_WindowH, mouseX, mouseY, world))
	{
		m_Gizmo = GizmoAction::None;
		return;
	}

	switch (m_Gizmo)
	{
		case GizmoAction::Move:
			transform->Local.Position.x = m_GizmoPosition.x + (world.x - m_GizmoMouse.x);
			transform->Local.Position.y = m_GizmoPosition.y + (world.y - m_GizmoMouse.y);
			break;
		case GizmoAction::Rotate:
		{
			float angle = std::atan2(world.y - m_GizmoPosition.y, world.x - m_GizmoPosition.x);
			m_GizmoSpin += WrapAngle(angle - m_GizmoAngle);
			m_GizmoAngle = angle;
			transform->Local.SetRotationZ(m_GizmoRotation + m_GizmoSpin);
			break;
		}
		case GizmoAction::Scale:
		{
			const Lite::Vec2* local = Lite::kQuadCorners;
			if (Lite::MeshComponent* mesh = entity.Get<Lite::MeshComponent>())
			{
				if (mesh->Type == Lite::MeshType::Triangle)
					local = Lite::kTriangleCorners;
			}

			Lite::Transform basis;
			basis.Position = m_GizmoPosition;
			basis.SetRotationZ(m_GizmoRotation);
			basis.Scale = { 1.0f, 1.0f, 1.0f };
			Lite::Vec2 point = LocalOnPlane(basis, world);
			Lite::Vec2 corner = local[m_GizmoCorner];
			Lite::Vec3 scale = m_GizmoScale;
			if (std::abs(corner.x) > 0.001f)
				scale.x = point.x / corner.x;
			if (std::abs(corner.y) > 0.001f)
				scale.y = point.y / corner.y;
			if (std::abs(scale.x) < 0.02f)
				scale.x = scale.x < 0.0f ? -0.02f : 0.02f;
			if (std::abs(scale.y) < 0.02f)
				scale.y = scale.y < 0.0f ? -0.02f : 0.02f;
			transform->Local.Scale.x = scale.x;
			transform->Local.Scale.y = scale.y;
			break;
		}
		default:
			break;
	}

	m_Scene->RefreshPhysics(entity.GetId());
}

void EditorLayer::DrawGizmo()
{
	if (m_Scene == nullptr || m_Selected == 0)
		return;

	Lite::Entity entity = m_Scene->GetEntity(m_Selected);
	GizmoLayout layout;
	if (!entity || !BuildGizmo(m_ViewProjection, m_WindowW, m_WindowH, entity, layout))
		return;

	ImDrawList* draw = ImGui::GetWindowDrawList();
	ImVec2 mouse = ImGui::GetIO().MousePos;
	int hotCorner = m_Gizmo == GizmoAction::Scale ? m_GizmoCorner : -1;
	bool hotRing = m_Gizmo == GizmoAction::Rotate;
	bool hotMove = m_Gizmo == GizmoAction::Move;
	if (m_Gizmo == GizmoAction::None && ImGui::IsWindowHovered(ImGuiHoveredFlags_AllowWhenBlockedByActiveItem))
	{
		hotCorner = HitScale(layout, mouse);
		hotRing = hotCorner < 0 && HitRing(layout, mouse);
		Lite::Vec2 world;
		hotMove = hotCorner < 0 && !hotRing && Unproject(m_ViewProjection, m_WindowW, m_WindowH, mouse.x, mouse.y, world) && Contains(world, layout.World, layout.Count);
	}

	if (hotMove)
		ImGui::SetMouseCursor(ImGuiMouseCursor_ResizeAll);
	else if (hotRing)
		ImGui::SetMouseCursor(ImGuiMouseCursor_Hand);
	else if (hotCorner >= 0)
		ImGui::SetMouseCursor(ImGuiMouseCursor_ResizeNWSE);

	const ImU32 outline = IM_COL32(236, 240, 246, 210);
	const ImU32 ring = hotRing ? IM_COL32(170, 220, 255, 255) : IM_COL32(110, 176, 240, 210);
	draw->AddPolyline(layout.Screen, layout.Count, outline, 1.6f, ImDrawFlags_Closed);
	draw->AddCircle(layout.Center, layout.Ring, ring, 48, hotRing ? 2.4f : 1.5f);

	Lite::TransformComponent* transform = entity.Get<Lite::TransformComponent>();
	if (transform != nullptr)
	{
		Lite::Vec3 axisX = transform->Local.TransformPoint({ 1.0f, 0.0f, 0.0f });
		Lite::Vec3 axisY = transform->Local.TransformPoint({ 0.0f, 1.0f, 0.0f });
		ImVec2 screenX;
		ImVec2 screenY;
		if (Project(m_ViewProjection, m_WindowW, m_WindowH, { axisX.x, axisX.y }, screenX))
		{
			ImVec2 direction { screenX.x - layout.Center.x, screenX.y - layout.Center.y };
			float length = std::sqrt(direction.x * direction.x + direction.y * direction.y);
			if (length > 0.001f)
			{
				direction.x = direction.x / length * 22.0f;
				direction.y = direction.y / length * 22.0f;
				draw->AddLine(layout.Center, { layout.Center.x + direction.x, layout.Center.y + direction.y }, IM_COL32(230, 74, 74, 230), 2.0f);
			}
		}
		if (Project(m_ViewProjection, m_WindowW, m_WindowH, { axisY.x, axisY.y }, screenY))
		{
			ImVec2 direction { screenY.x - layout.Center.x, screenY.y - layout.Center.y };
			float length = std::sqrt(direction.x * direction.x + direction.y * direction.y);
			if (length > 0.001f)
			{
				direction.x = direction.x / length * 22.0f;
				direction.y = direction.y / length * 22.0f;
				draw->AddLine(layout.Center, { layout.Center.x + direction.x, layout.Center.y + direction.y }, IM_COL32(78, 206, 96, 230), 2.0f);
			}
		}
	}

	for (int index = 0; index < layout.Count; ++index)
	{
		ImVec2 corner = layout.Screen[index];
		ImVec2 min { corner.x - kHandleSize, corner.y - kHandleSize };
		ImVec2 max { corner.x + kHandleSize, corner.y + kHandleSize };
		ImU32 fill = index == hotCorner ? IM_COL32(255, 214, 96, 255) : IM_COL32(246, 248, 252, 240);
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
	ImVec2 mouse = ImGui::GetIO().MousePos;
	if (m_Gizmo != GizmoAction::None)
	{
		if (ImGui::IsMouseDown(ImGuiMouseButton_Left))
			ApplyGizmo(mouse.x, mouse.y);
		else
			m_Gizmo = GizmoAction::None;
	}
	else if (ImGui::IsItemClicked(ImGuiMouseButton_Left))
	{
		if (!BeginGizmo(mouse.x, mouse.y))
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
