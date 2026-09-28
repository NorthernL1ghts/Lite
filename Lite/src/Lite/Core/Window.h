#pragma once

#include "Events/Event.h"

#include <functional>
#include <string>
#include <string_view>

struct GLFWwindow;

namespace Lite {

	struct WindowProps
	{
		std::string Title;
		int Width;
		int Height;

		WindowProps(std::string_view title = "Lite", int width = 1280, int height = 720)
			: Title(title)
			, Width(width)
			, Height(height)
		{
		}
	};

	class Window
	{
	public:
		using EventCallback = std::function<void(Event&)>;

		explicit Window(const WindowProps& props);
		~Window();

		void Update();
		bool IsOpen() const;

		void SetEventCallback(EventCallback callback) { m_Data.Callback = std::move(callback); }
		const WindowProps& GetProps() const { return m_Data.Props; }

	private:
		struct WindowData
		{
			WindowProps Props;
			EventCallback Callback;
		};

		void SetCallbacks();

		WindowData m_Data;
		GLFWwindow* m_Window = nullptr;
		bool m_GlfwInitialized = false;
	};

}
