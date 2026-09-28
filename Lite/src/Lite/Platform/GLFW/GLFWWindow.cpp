#include "GLFWWindow.h"

#include "Lite/Core/Logger.h"
#include "Lite/Core/Events/KeyEvent.h"
#include "Lite/Core/Events/MouseEvent.h"
#include "Lite/Core/Events/WindowEvent.h"

#include <GLFW/glfw3.h>

namespace {

	void OnGlfwError(int error, const char* description)
	{
		LITE_ERROR("GLFW {}: {}", error, description);
	}

}

namespace Lite {

	GLFWWindow::GLFWWindow(const WindowProps& props)
		: m_Data{ props, {} }
	{
		glfwSetErrorCallback(OnGlfwError);

		if (!glfwInit())
		{
			LITE_ERROR("Failed to initialize GLFW");
			return;
		}

		m_GlfwInitialized = true;

		glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
		glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);

		m_Window = glfwCreateWindow(m_Data.Props.Width, m_Data.Props.Height, m_Data.Props.Title.c_str(), nullptr, nullptr);
		if (!m_Window)
		{
			LITE_ERROR("Failed to create window");
			return;
		}

		glfwSetWindowUserPointer(m_Window, &m_Data);
		SetCallbacks();
		LITE_INFO("Created window {} ({}x{})", m_Data.Props.Title, m_Data.Props.Width, m_Data.Props.Height);
	}

	GLFWWindow::~GLFWWindow()
	{
		if (m_Window)
			glfwDestroyWindow(m_Window);

		if (m_GlfwInitialized)
			glfwTerminate();
	}

	void GLFWWindow::PollEvents()
	{
		glfwPollEvents();
	}

	void GLFWWindow::Clear()
	{
	}

	void GLFWWindow::SwapBuffers()
	{
	}

	bool GLFWWindow::IsOpen() const
	{
		return m_Window && !glfwWindowShouldClose(m_Window);
	}

	void GLFWWindow::SetEventCallback(EventCallback callback)
	{
		m_Data.Callback = std::move(callback);
	}

	void GLFWWindow::SetCallbacks()
	{
		glfwSetWindowCloseCallback(m_Window, [](GLFWwindow* window)
		{
			auto& data = *static_cast<WindowData*>(glfwGetWindowUserPointer(window));
			WindowCloseEvent event;
			if (data.Callback)
				data.Callback(event);
		});

		glfwSetWindowSizeCallback(m_Window, [](GLFWwindow* window, int width, int height)
		{
			auto& data = *static_cast<WindowData*>(glfwGetWindowUserPointer(window));
			data.Props.Width = width;
			data.Props.Height = height;

			WindowResizeEvent event(width, height);
			if (data.Callback)
				data.Callback(event);
		});

		glfwSetFramebufferSizeCallback(m_Window, [](GLFWwindow* window, int width, int height)
		{
			auto& data = *static_cast<WindowData*>(glfwGetWindowUserPointer(window));
			WindowResizeEvent event(width, height);
			if (data.Callback)
				data.Callback(event);
		});

		glfwSetKeyCallback(m_Window, [](GLFWwindow* window, int key, int scancode, int action, int mods)
		{
			(void)scancode;
			(void)mods;

			auto& data = *static_cast<WindowData*>(glfwGetWindowUserPointer(window));
			if (!data.Callback)
				return;

			switch (action)
			{
				case GLFW_PRESS:
				{
					KeyPressedEvent event(key, false);
					data.Callback(event);
					break;
				}
				case GLFW_RELEASE:
				{
					KeyReleasedEvent event(key);
					data.Callback(event);
					break;
				}
				case GLFW_REPEAT:
				{
					KeyPressedEvent event(key, true);
					data.Callback(event);
					break;
				}
			}
		});

		glfwSetCharCallback(m_Window, [](GLFWwindow* window, unsigned int keyCode)
		{
			auto& data = *static_cast<WindowData*>(glfwGetWindowUserPointer(window));
			if (!data.Callback)
				return;

			KeyTypedEvent event(static_cast<int>(keyCode));
			data.Callback(event);
		});

		glfwSetMouseButtonCallback(m_Window, [](GLFWwindow* window, int button, int action, int mods)
		{
			(void)mods;

			auto& data = *static_cast<WindowData*>(glfwGetWindowUserPointer(window));
			if (!data.Callback)
				return;

			switch (action)
			{
				case GLFW_PRESS:
				{
					MouseButtonPressedEvent event(button);
					data.Callback(event);
					break;
				}
				case GLFW_RELEASE:
				{
					MouseButtonReleasedEvent event(button);
					data.Callback(event);
					break;
				}
			}
		});

		glfwSetScrollCallback(m_Window, [](GLFWwindow* window, double xOffset, double yOffset)
		{
			auto& data = *static_cast<WindowData*>(glfwGetWindowUserPointer(window));
			if (!data.Callback)
				return;

			MouseScrolledEvent event(static_cast<float>(xOffset), static_cast<float>(yOffset));
			data.Callback(event);
		});

		glfwSetCursorPosCallback(m_Window, [](GLFWwindow* window, double x, double y)
		{
			auto& data = *static_cast<WindowData*>(glfwGetWindowUserPointer(window));
			if (!data.Callback)
				return;

			MouseMovedEvent event(static_cast<float>(x), static_cast<float>(y));
			data.Callback(event);
		});
	}

}
