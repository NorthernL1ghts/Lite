#include "Application.h"

#include <print>

namespace Lite {

	Application::Application()
	{
		std::println("Lite Application created");
	}

	Application::~Application()
	{
		std::println("Lite Application destroyed");
	}

	void Application::Run()
	{
		while (m_Running)
		{
			std::println("Lite running");
			m_Running = false;
		}
	}

}
