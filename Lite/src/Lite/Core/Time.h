#pragma once

#include "Base.h"

namespace Lite {

	class Application;

	class LITE_API Time
	{
	public:
		static void SetFPS(float fps);
		static float GetFPS();

		static float GetDelta();
		static float GetElapsed();

	private:
		friend class Application;

		static void Update();
		static void Limit();
	};

}
