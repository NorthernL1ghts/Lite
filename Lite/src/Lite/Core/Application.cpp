#include "Application.h"
#include "Logger.h"
#include "Window.h"

#include "Events/WindowEvent.h"

namespace Lite {

	Application::Application(const WindowProps& props)
		: m_Window(std::make_unique<Window>(props))
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
			m_Window->Update();
	}

	void Application::OnEvent(Event& event)
	{
		EventDispatcher dispatcher(event);
		dispatcher.Dispatch<WindowCloseEvent>([this](WindowCloseEvent&)
		{
			m_Running = false;
			return true;
		});

		if (event.GetType() != EventType::MouseMoved)
			LITE_TRACE("{}", event.ToString());
	}

}
