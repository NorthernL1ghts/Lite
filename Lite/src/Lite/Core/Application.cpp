#include "Application.h"
#include "Logger.h"
#include "Window.h"

namespace Lite {

	Application::Application(const WindowProps& props)
		: m_Window(std::make_unique<Window>(props))
	{
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

}
