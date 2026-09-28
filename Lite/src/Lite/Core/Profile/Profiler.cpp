#include <Lite/Core/Profile/Profiler.h>

#include <chrono>

namespace Lite {

	namespace {

		std::vector<ProfileSample> s_Writing;
		std::vector<ProfileSample> s_Published;
		int s_Depth = 0;

	}

	void Profiler::BeginFrame()
	{
		s_Writing.clear();
		s_Depth = 0;
	}

	void Profiler::EndFrame()
	{
		s_Published.swap(s_Writing);
		s_Writing.clear();
		s_Depth = 0;
	}

	double Profiler::Now()
	{
		using Clock = std::chrono::steady_clock;
		return std::chrono::duration<double>(Clock::now().time_since_epoch()).count();
	}

	int Profiler::Push(const char* name)
	{
		int index = static_cast<int>(s_Writing.size());
		s_Writing.push_back({ name, 0.0f, s_Depth });
		++s_Depth;
		return index;
	}

	void Profiler::Pop(int index, double start)
	{
		if (s_Depth > 0)
			--s_Depth;

		if (index < 0 || index >= static_cast<int>(s_Writing.size()))
			return;

		s_Writing[static_cast<size_t>(index)].Milliseconds = static_cast<float>((Now() - start) * 1000.0);
	}

	const std::vector<ProfileSample>& Profiler::GetSamples()
	{
		return s_Published;
	}

}
