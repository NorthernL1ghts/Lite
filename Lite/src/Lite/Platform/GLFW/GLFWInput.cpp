#include "Lite/Input/Input.h"

#include <GLFW/glfw3.h>

namespace {

	GLFWwindow* s_Window = nullptr;

}

namespace Lite {

	void Input::SetWindow(void* nativeWindow)
	{
		s_Window = static_cast<GLFWwindow*>(nativeWindow);
	}

	bool Input::IsKeyPressed(KeyCode key)
	{
		if (!s_Window)
			return false;

		auto state = glfwGetKey(s_Window, static_cast<int>(key));
		return state == GLFW_PRESS || state == GLFW_REPEAT;
	}

	bool Input::IsMouseButtonPressed(MouseCode button)
	{
		if (!s_Window)
			return false;

		auto state = glfwGetMouseButton(s_Window, static_cast<int>(button));
		return state == GLFW_PRESS;
	}

	std::pair<float, float> Input::GetMousePosition()
	{
		if (!s_Window)
			return { 0.0f, 0.0f };

		double x = 0.0;
		double y = 0.0;
		glfwGetCursorPos(s_Window, &x, &y);
		return { static_cast<float>(x), static_cast<float>(y) };
	}

	float Input::GetMouseX()
	{
		auto [x, y] = GetMousePosition();
		(void)y;
		return x;
	}

	float Input::GetMouseY()
	{
		auto [x, y] = GetMousePosition();
		(void)x;
		return y;
	}

}
