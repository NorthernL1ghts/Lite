#pragma once

#include "Base.h"
#include "LayerStack.h"
#include "Window.h"

#include <memory>

namespace Lite {

	class ImGuiLayer;

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
		void InitCore();
		void ShutdownCore();
		void OnEvent(Event& event);

		static Application* s_Instance;

		WindowProps m_Props;
		bool m_Running = true;
		bool m_Initialized = false;
		std::unique_ptr<Window> m_Window;
		ImGuiLayer* m_ImGuiLayer = nullptr;
		LayerStack m_LayerStack;
	};

	std::unique_ptr<Application> CreateApplication();

}
