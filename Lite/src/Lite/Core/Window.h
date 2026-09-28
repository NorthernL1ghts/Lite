#pragma once

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
		explicit Window(const WindowProps& props);
		~Window();

		void Update();
		bool IsOpen() const;

		const WindowProps& GetProps() const { return m_Props; }

	private:
		WindowProps m_Props;
		GLFWwindow* m_Window = nullptr;
		bool m_GlfwInitialized = false;
	};

}
