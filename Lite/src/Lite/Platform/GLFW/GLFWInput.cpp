#include <Lite/Input/Input.h>
#include <Lite/Scene/SceneCamera.h>

#include <GLFW/glfw3.h>

namespace {

	GLFWwindow* s_Window = nullptr;
	float s_ViewX = 0.0f;
	float s_ViewY = 0.0f;
	float s_ViewW = 0.0f;
	float s_ViewH = 0.0f;
	Lite::Mat4 s_View = Lite::Mat4::Identity();

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

	void Input::SetCursorPlane(float x, float y, float width, float height, const Mat4& viewProjection)
	{
		s_ViewX = x;
		s_ViewY = y;
		s_ViewW = width;
		s_ViewH = height;
		s_View = viewProjection;
	}

	bool Input::CursorWorld(Vec2& world)
	{
		if (s_Window == nullptr || s_ViewW <= 1.0f || s_ViewH <= 1.0f)
			return false;

		auto [x, y] = GetMousePosition();
		std::optional<Vec2> point = ScreenToWorld(s_View, s_ViewW, s_ViewH, x - s_ViewX, y - s_ViewY);
		if (!point)
			return false;

		world = *point;
		return true;
	}

}
