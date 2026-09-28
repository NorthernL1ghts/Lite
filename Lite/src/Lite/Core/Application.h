#pragma once

#include "Base.h"
#include "LayerStack.h"
#include "Window.h"

#include <memory>

namespace Lite {

	class LITE_API Application
	{
	public:
		explicit Application(const WindowProps& props = {});
		virtual ~Application();

		void Run();

		void PushLayer(std::unique_ptr<Layer> layer);
		void PushOverlay(std::unique_ptr<Layer> layer);
		void PopLayer(Layer* layer);
		void PopOverlay(Layer* layer);

	private:
		void OnEvent(Event& event);

		bool m_Running = true;
		std::unique_ptr<Window> m_Window;
		LayerStack m_LayerStack;
	};

	std::unique_ptr<Application> CreateApplication();

}
