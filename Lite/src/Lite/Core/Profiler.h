#pragma once

#include "Base.h"

#include <vector>

namespace Lite {

	struct ProfileSample
	{
		const char* Name = nullptr;
		float Milliseconds = 0.0f;
		int Depth = 0;
	};

	class LITE_API Profiler
	{
	public:
		static void BeginFrame();
		static void EndFrame();
		static double Now();
		static int Push(const char* name);
		static void Pop(int index, double start);

		static const std::vector<ProfileSample>& GetSamples();
	};

	class ProfileScope
	{
	public:
		explicit ProfileScope(const char* name)
			: m_Index(Profiler::Push(name))
			, m_Start(Profiler::Now())
		{
		}

		~ProfileScope()
		{
			Profiler::Pop(m_Index, m_Start);
		}

		ProfileScope(const ProfileScope&) = delete;
		ProfileScope& operator=(const ProfileScope&) = delete;

	private:
		int m_Index = -1;
		double m_Start = 0.0;
	};

}

#define LITE_PROFILE_CONCAT_INNER(left, right) left##right
#define LITE_PROFILE_CONCAT(left, right) LITE_PROFILE_CONCAT_INNER(left, right)
#define LITE_PROFILE_SCOPE(name) ::Lite::ProfileScope LITE_PROFILE_CONCAT(liteProfileScope, __LINE__)(name)
#define LITE_PROFILE_FUNCTION() LITE_PROFILE_SCOPE(__FUNCTION__)
