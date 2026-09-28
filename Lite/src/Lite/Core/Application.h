#pragma once

#include <Lite/Core/Base.h>
#include <Lite/Core/LayerStack.h>
#include <Lite/Core/Window.h>

namespace Lite {

	class ImGuiLayer;

	class LITE_API Application
	{
	public:
		explicit Application(const WindowProps& props = {});
		virtual ~Application();

		void Run();

		void PushLayer(Scope<Layer> layer);
		void PushOverlay(Scope<Layer> layer);
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
		Scope<Window> m_Window;
		ImGuiLayer* m_ImGuiLayer = nullptr;
		LayerStack m_LayerStack;
	};

	Scope<Application> CreateApplication();

}
