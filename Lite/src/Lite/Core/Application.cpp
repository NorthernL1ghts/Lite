#include "Application.h"
#include "Logger.h"
#include "Window.h"

#include "Events/WindowEvent.h"
#include "Lite/ImGui/ImGuiLayer.h"
#include "Lite/Input/Input.h"
#include "Lite/Renderer/Renderer2D.h"

namespace Lite {

	Application::Application(const WindowProps& props)
		: m_Window(Window::Create(props))
	{
		m_Window->SetEventCallback([this](Event& event) { OnEvent(event); });
		Input::SetWindow(m_Window->GetNativeHandle());
		Renderer2D::Init(m_Window->GetNativeHandle());

		auto imgui = std::make_unique<ImGuiLayer>(static_cast<GLFWwindow*>(m_Window->GetNativeHandle()));
		m_ImGuiLayer = imgui.get();
		PushOverlay(std::move(imgui));

		LITE_INFO("Application created");
	}

	Application::~Application()
	{
		m_LayerStack.Clear();
		Renderer2D::Shutdown();
		LITE_INFO("Application destroyed");
	}

	void Application::Run()
	{
		while (m_Running && m_Window->IsOpen())
		{
			m_Window->PollEvents();
			Renderer2D::BeginFrame();

			if (Renderer2D::IsFrameActive())
			{
				for (auto& layer : m_LayerStack)
					layer->OnUpdate();

				m_ImGuiLayer->Begin();
				for (auto& layer : m_LayerStack)
					layer->OnImGuiRender();
				m_ImGuiLayer->End();

				Renderer2D::EndFrame();
			}
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

	void Application::PushLayer(std::unique_ptr<Layer> layer)
	{
		m_LayerStack.PushLayer(std::move(layer));
	}

	void Application::PushOverlay(std::unique_ptr<Layer> layer)
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
