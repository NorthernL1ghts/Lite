#include "Application.h"
#include "Logger.h"

namespace Lite {

	Application::Application()
	{
		LITE_INFO("Application created");
	}

	Application::~Application()
	{
		LITE_INFO("Application destroyed");
	}

	void Application::Run()
	{
		while (m_Running)
		{
			LITE_INFO("Running");
			m_Running = false;
		}
	}

}
