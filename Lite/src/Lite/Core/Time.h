#pragma once

#include "Base.h"

namespace Lite {

	class Timestep
	{
	public:
		Timestep(float seconds = 0.0f)
			: m_Seconds(seconds)
		{
		}

		float GetSeconds() const { return m_Seconds; }
		float GetMilliseconds() const { return m_Seconds * 1000.0f; }

		operator float() const { return m_Seconds; }

	private:
		float m_Seconds;
	};

	class Application;

	class LITE_API Time
	{
	public:
		static void SetFPS(float fps);
		static float GetFPS();
		static float GetFrameRate();

		static float GetDelta();
		static float GetDeltaMilliseconds();
		static float GetElapsed();
		static Timestep GetTimestep();

	private:
		friend class Application;

		static void Update();
		static void Limit();
	};

}
