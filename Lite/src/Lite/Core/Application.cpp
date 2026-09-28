#include "Application.h"
#include "Logger.h"
#include "Window.h"

#include "Events/WindowEvent.h"

namespace Lite {

	Application::Application(const WindowProps& props)
		: m_Window(Window::Create(props))
	{
		m_Window->SetEventCallback([this](Event& event) { OnEvent(event); });
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
			for (auto& layer : m_LayerStack)
				layer->OnUpdate();

			m_Window->Update();
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
