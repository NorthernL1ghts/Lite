#include "Window.h"
#include "Logger.h"

#include <GLFW/glfw3.h>
#include <GL/gl.h>

namespace {

	void OnGlfwError(int error, const char* description)
	{
		LITE_ERROR("GLFW {}: {}", error, description);
	}

}

namespace Lite {

	Window::Window(const WindowProps& props)
		: m_Props(props)
	{
		glfwSetErrorCallback(OnGlfwError);

		if (!glfwInit())
		{
			LITE_ERROR("Failed to initialize GLFW");
			return;
		}

		m_GlfwInitialized = true;
		m_Window = glfwCreateWindow(m_Props.Width, m_Props.Height, m_Props.Title.c_str(), nullptr, nullptr);
		if (!m_Window)
		{
			LITE_ERROR("Failed to create window");
			return;
		}

		glfwMakeContextCurrent(m_Window);
		LITE_INFO("Created window {} ({}x{})", m_Props.Title, m_Props.Width, m_Props.Height);
	}

	Window::~Window()
	{
		if (m_Window)
			glfwDestroyWindow(m_Window);

		if (m_GlfwInitialized)
			glfwTerminate();
	}

	void Window::Update()
	{
		glClearColor(1.0f, 0.0f, 1.0f, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT);

		glfwPollEvents();
		glfwSwapBuffers(m_Window);
	}

	bool Window::IsOpen() const
	{
		return m_Window && !glfwWindowShouldClose(m_Window);
	}

}
