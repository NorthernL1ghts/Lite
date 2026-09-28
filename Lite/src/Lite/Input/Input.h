#pragma once

#include <Lite/Core/Base.h>
#include <Lite/Input/KeyCodes.h>
#include <Lite/Input/MouseCodes.h>

#include <utility>

namespace Lite {

	class Application;

	class LITE_API Input
	{
	public:
		static bool IsKeyPressed(KeyCode key);
		static bool IsMouseButtonPressed(MouseCode button);

		static float GetMouseX();
		static float GetMouseY();
		static std::pair<float, float> GetMousePosition();

	private:
		friend class Application;

		static void SetWindow(void* nativeWindow);
	};

}
