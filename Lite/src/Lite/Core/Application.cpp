#include "Application.h"
#include "Logger.h"
#include "Window.h"

#include "Events/WindowEvent.h"
#include "Lite/ImGui/ImGuiLayer.h"

#include <GLFW/glfw3.h>

namespace Lite {

	Application::Application(const WindowProps& props)
		: m_Window(Window::Create(props))
	{
		m_Window->SetEventCallback([this](Event& event) { OnEvent(event); });

		auto imgui = std::make_unique<ImGuiLayer>(static_cast<GLFWwindow*>(m_Window->GetNativeHandle()));
		m_ImGuiLayer = imgui.get();
		PushOverlay(std::move(imgui));

		LITE_INFO("Application created");
	}

	Application::~Application()
	{
		LITE_INFO("Application destroyed");
	}

	void Application::Run()
	{
		while (m_Running && m_Window->IsOpen())
		{
			m_Window->PollEvents();
			m_Window->Clear();

			for (auto& layer : m_LayerStack)
				layer->OnUpdate();

			m_ImGuiLayer->Begin();
			for (auto& layer : m_LayerStack)
				layer->OnImGuiRender();
			m_ImGuiLayer->End();

			m_Window->SwapBuffers();
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
