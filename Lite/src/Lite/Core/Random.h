#pragma once

#include <Lite/Core/Base.h>

#include <cstdint>

namespace Lite {

	class LITE_API Random
	{
	public:
		static void Seed(uint32_t seed);
		static int Int(int min, int max);
		static float Float();
		static float Float(float min, float max);
		static bool Bool();
	};

}
