#include "Time.h"

#include <chrono>
#include <thread>

namespace Lite {

	namespace {

		float s_Delta = 0.0f;
		float s_Elapsed = 0.0f;
		float s_TargetDelta = 0.0f;
		bool s_Started = false;
		std::chrono::steady_clock::time_point s_Last {};

	}

	void Time::SetFPS(float fps)
	{
		if (fps <= 0.0f)
		{
			s_TargetDelta = 0.0f;
			return;
		}

		s_TargetDelta = 1.0f / fps;
	}

	float Time::GetFPS()
	{
		if (s_TargetDelta <= 0.0f)
			return 0.0f;

		return 1.0f / s_TargetDelta;
	}

	float Time::GetDelta()
	{
		return s_Delta;
	}

	float Time::GetElapsed()
	{
		return s_Elapsed;
	}

	void Time::Update()
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

	void Time::Limit()
	{
		if (s_TargetDelta <= 0.0f || !s_Started)
			return;

		auto target = s_Last + std::chrono::duration_cast<std::chrono::steady_clock::duration>(
			std::chrono::duration<double>(s_TargetDelta));

		while (std::chrono::steady_clock::now() < target)
		{
			auto remaining = target - std::chrono::steady_clock::now();
			if (remaining > std::chrono::milliseconds(2))
				std::this_thread::sleep_for(remaining - std::chrono::milliseconds(1));
			else
				std::this_thread::yield();
		}
	}

}
