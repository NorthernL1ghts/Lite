#include "EditorLayer.h"

#include "Lite/Core/Events/KeyEvent.h"
#include "Lite/Core/Events/MouseEvent.h"
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

void EditorLayer::SaveScene()
{
	if (!m_Scene)
		return;

	m_Scene->SetName(m_Name);
	m_Scene->Save();
}

void EditorLayer::SyncName()
{
	m_NamedScene = m_Scene.get();
	const char* name = m_Scene != nullptr ? m_Scene->GetName().c_str() : "";
	std::snprintf(m_Name, sizeof(m_Name), "%s", name);
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
			SaveScene();
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

void EditorLayer::OnImGuiRender()
{
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
}

void EditorLayer::DrawScene()
{
	ImGui::Begin("Scene");
	if (ImGui::Button("New"))
		NewScene();
	ImGui::SameLine();
	if (ImGui::Button("Save"))
		SaveScene();
	ImGui::SameLine();
	bool playing = m_Scene && m_Scene->IsPlaying();
	if (playing)
		ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.18f, 0.48f, 0.28f, 1.0f));
	if (ImGui::Button("Play") && m_Scene)
		m_Scene->Play();
	if (playing)
		ImGui::PopStyleColor();
	ImGui::SameLine();
	bool paused = m_Scene && m_Scene->GetPlayback() == Lite::ScenePlayback::Paused;
	if (paused)
		ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.45f, 0.36f, 0.14f, 1.0f));
	if (ImGui::Button("Pause") && m_Scene)
		m_Scene->Pause();
	if (paused)
		ImGui::PopStyleColor();

	if (m_Scene)
	{
		if (m_NamedScene != m_Scene.get())
			SyncName();
		ImGui::SetNextItemWidth(-1.0f);
		if (ImGui::InputText("##SceneName", m_Name, sizeof(m_Name)))
			m_Scene->SetName(m_Name);
	}

	ImGui::TextDisabled("Scenes");
	for (const std::string& path : Lite::Scene::List())
	{
		std::string label = std::filesystem::path(path).stem().string();
		bool current = m_Scene && m_Scene->GetPath() == path;
		if (ImGui::Selectable(label.c_str(), current))
			OpenScene(path);
	}

	ImGui::Separator();
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
	ImVec2 size = ImGui::GetContentRegionAvail();
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
	ImGui::TextDisabled("%s  %s  %.0f x %.0f", name, playback, size.x, size.y);
	ImGui::End();
}

void EditorLayer::DrawInspector()
{
	ImGui::Begin("Inspector");
	Lite::Scene* scene = Lite::Scene::GetActive();
	const Lite::SceneObject* object = scene != nullptr ? scene->Find(m_Selection) : nullptr;
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

	const Lite::Vec3& position = object->Transform.Position;
	const Lite::Vec3& scale = object->Transform.Scale;
	ImGui::TextUnformatted(object->Name.c_str());
	ImGui::Separator();
	ImGui::Text("Kind      %s", kind);
	ImGui::Text("Position  %.2f, %.2f, %.2f", position.x, position.y, position.z);
	ImGui::Text("Rotation  %.2f", object->Transform.GetRotationZ());
	ImGui::Text("Scale     %.2f, %.2f, %.2f", scale.x, scale.y, scale.z);
	if (object->Kind == Lite::SceneObjectKind::Sprite)
		ImGui::Text("Tiling    %.2f, %.2f", object->Tiling.x, object->Tiling.y);
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
