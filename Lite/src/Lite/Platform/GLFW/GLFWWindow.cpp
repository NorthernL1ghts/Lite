#include <Lite/Platform/GLFW/GLFWWindow.h>

#include <Lite/Core/Log/Logger.h>
#include <Lite/Core/Events/KeyEvent.h>
#include <Lite/Core/Events/MouseEvent.h>
#include <Lite/Core/Events/WindowEvent.h>

#include <GLFW/glfw3.h>

#include <algorithm>
#include <cmath>

namespace {

	void PaintIcon(unsigned char* pixels, int size)
	{
		const float half = static_cast<float>(size) * 0.5f;
		const float radius = static_cast<float>(size) * 0.22f;
		const float extent = half - 1.0f;
		for (int y = 0; y < size; ++y)
		{
			for (int x = 0; x < size; ++x)
			{
				const float px = static_cast<float>(x) + 0.5f;
				const float py = static_cast<float>(y) + 0.5f;
				const float dx = std::fabs(px - half);
				const float dy = std::fabs(py - half);
				const float boxX = dx - (extent - radius);
				const float boxY = dy - (extent - radius);
				const float outside = std::sqrt(std::max(boxX, 0.0f) * std::max(boxX, 0.0f) + std::max(boxY, 0.0f) * std::max(boxY, 0.0f));
				const float distance = outside + std::min(std::max(boxX, boxY), 0.0f) - radius;
				unsigned char* pixel = pixels + (y * size + x) * 4;
				if (distance > 0.6f)
				{
					pixel[0] = pixel[1] = pixel[2] = pixel[3] = 0;
					continue;
				}

				const float ax = half - static_cast<float>(size) * 0.12f;
				const float ay = half - static_cast<float>(size) * 0.16f;
				const float bx = half - static_cast<float>(size) * 0.12f;
				const float by = half + static_cast<float>(size) * 0.16f;
				const float cx = half + static_cast<float>(size) * 0.18f;
				const float cy = half;
				const float area = (bx - ax) * (cy - ay) - (by - ay) * (cx - ax);
				const float w0 = ((bx - px) * (cy - py) - (by - py) * (cx - px)) / area;
				const float w1 = ((cx - px) * (ay - py) - (cy - py) * (ax - px)) / area;
				const float w2 = 1.0f - w0 - w1;
				const bool mark = w0 >= 0.0f && w1 >= 0.0f && w2 >= 0.0f;
				if (mark)
				{
					pixel[0] = 246;
					pixel[1] = 248;
					pixel[2] = 252;
				}
				else
				{
					pixel[0] = 58;
					pixel[1] = 112;
					pixel[2] = 196;
				}
				pixel[3] = distance > 0.0f ? static_cast<unsigned char>((0.6f - distance) / 0.6f * 255.0f) : 255;
			}
		}
	}

	void ApplyWindowIcon(GLFWwindow* window)
	{
		unsigned char large[32 * 32 * 4];
		unsigned char small[16 * 16 * 4];
		PaintIcon(large, 32);
		PaintIcon(small, 16);
		GLFWimage images[2] = {
			{ 32, 32, large },
			{ 16, 16, small }
		};
		glfwSetWindowIcon(window, 2, images);
	}

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
		ApplyWindowIcon(m_Window);
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
