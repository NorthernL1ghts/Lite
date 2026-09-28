#pragma once

#include <chrono>

namespace Lite {

	class Application;

	class Time
	{
	public:
		static float GetDelta() { return s_Delta; }
		static float GetElapsed() { return s_Elapsed; }

	private:
		friend class Application;

		static void Update()
		{
			auto now = std::chrono::steady_clock::now();
			if (!s_Started)
			{
				s_Last = now;
				s_Started = true;
				s_Delta = 0.0f;
				return;
			}

			std::chrono::duration<float> step = now - s_Last;
			s_Last = now;
			s_Delta = step.count();
			if (s_Delta > 0.1f)
				s_Delta = 0.1f;

			s_Elapsed += s_Delta;
		}

		static inline float s_Delta = 0.0f;
		static inline float s_Elapsed = 0.0f;
		static inline bool s_Started = false;
		static inline std::chrono::steady_clock::time_point s_Last {};
	};

}
