#pragma once

#include "Base.h"
#include "Window.h"

#include <memory>

namespace Lite {

	class LITE_API Application
	{
	public:
		explicit Application(const WindowProps& props = {});
		virtual ~Application();

		void Run();

	private:
		bool m_Running = true;
		std::unique_ptr<Window> m_Window;
	};

	std::unique_ptr<Application> CreateApplication();

}
