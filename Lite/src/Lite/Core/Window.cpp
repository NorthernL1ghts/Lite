#include <Lite/Core/Window.h>

#include <Lite/Platform/GLFW/GLFWWindow.h>

namespace Lite {

	Scope<Window> Window::Create(const WindowProps& props)
	{
		return CreateScope<GLFWWindow>(props);
	}

}
