#include "Application.h"
#include "Assert.h"
#include "Logger.h"
#include "Profiler.h"
#include "Time.h"
#include "Window.h"

#include "Events/WindowEvent.h"
#include "Lite/Assets/AssetRegistry.h"
#include "Lite/ImGui/ImGuiLayer.h"
#include "Lite/Input/Input.h"
#include "Lite/Renderer/Renderer2D.h"

namespace Lite {

	Application* Application::s_Instance = nullptr;

	Application::Application(const WindowProps& props)
		: m_Props(props)
	{
		InitCore();
	}

	Application::~Application()
	{
		ShutdownCore();
	}

	void Application::InitCore()
	{
		Logger::Init();
		LITE_CORE_ASSERT(s_Instance == nullptr, "Application already exists");
		if (s_Instance)
			return;

		s_Instance = this;

		AssetRegistry::Init();
		m_Window = Window::Create(m_Props);
		m_Window->SetEventCallback([this](Event& event) { OnEvent(event); });
		Input::SetWindow(m_Window->GetNativeHandle());
		Renderer2D::Init(m_Window->GetNativeHandle());

		auto imgui = CreateScope<ImGuiLayer>(static_cast<GLFWwindow*>(m_Window->GetNativeHandle()));
		m_ImGuiLayer = imgui.get();
		PushOverlay(std::move(imgui));

		m_Initialized = true;
		LITE_INFO("Application created");
	}

	void Application::ShutdownCore()
	{
		if (!m_Initialized)
			return;

		m_LayerStack.Clear();
		m_ImGuiLayer = nullptr;
		AssetRegistry::Shutdown();
		Renderer2D::Shutdown();
		Input::SetWindow(nullptr);
		m_Window.reset();

		m_Initialized = false;
		s_Instance = nullptr;
		LITE_INFO("Application destroyed");
		Logger::Shutdown();
	}

	void Application::Run()
	{
		while (m_Running && m_Window->IsOpen())
		{
			Profiler::BeginFrame();
			Time::Update();

			{
				LITE_PROFILE_SCOPE("Window");
				m_Window->PollEvents();
			}

			{
				LITE_PROFILE_SCOPE("Renderer Begin");
				Renderer2D::BeginFrame();
			}

			if (Renderer2D::IsFrameActive())
			{
				Timestep timestep = Time::GetTimestep();

				{
					LITE_PROFILE_SCOPE("Update");
					for (auto& layer : m_LayerStack)
						layer->OnUpdate(timestep);
				}

				{
					LITE_PROFILE_SCOPE("Render");
					for (auto& layer : m_LayerStack)
						layer->OnRender();
				}

				Renderer2D::Flush();

				{
					LITE_PROFILE_SCOPE("ImGui");
					m_ImGuiLayer->Begin();
					for (auto& layer : m_LayerStack)
						layer->OnImGuiRender();
					m_ImGuiLayer->End();
				}

				{
					LITE_PROFILE_SCOPE("Renderer End");
					Renderer2D::EndFrame();
				}
			}

			{
				LITE_PROFILE_SCOPE("Frame Limit");
				Time::Limit();
			}

			Profiler::EndFrame();
		}
	}

	void Application::OnEvent(Event& event)
	{
		for (auto it = m_LayerStack.rbegin(); it != m_LayerStack.rend(); ++it)
		{
			if (event.Handled)
				break;

			(*it)->OnEvent(event);
		}

		EventDispatcher dispatcher(event);
		dispatcher.Dispatch<WindowCloseEvent>([this](WindowCloseEvent&)
		{
			m_Running = false;
			return true;
		});

		dispatcher.Dispatch<WindowResizeEvent>([](WindowResizeEvent& event)
		{
			Renderer2D::OnResize(event.GetWidth(), event.GetHeight());
			return false;
		});

		if (event.GetType() != EventType::MouseMoved)
			LITE_TRACE("{}", event.ToString());
	}

	void Application::PushLayer(Scope<Layer> layer)
	{
		m_LayerStack.PushLayer(std::move(layer));
	}

	void Application::PushOverlay(Scope<Layer> layer)
	{
		m_LayerStack.PushOverlay(std::move(layer));
	}

	void Application::PopLayer(Layer* layer)
	{
		m_LayerStack.PopLayer(layer);
	}

	void Application::PopOverlay(Layer* layer)
	{
		m_LayerStack.PopOverlay(layer);
	}

}
