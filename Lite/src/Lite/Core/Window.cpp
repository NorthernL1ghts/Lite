#include "Window.h"

#include "Lite/Platform/GLFW/GLFWWindow.h"

namespace Lite {

	std::unique_ptr<Window> Window::Create(const WindowProps& props)
	{
		return std::make_unique<GLFWWindow>(props);
	}

}
