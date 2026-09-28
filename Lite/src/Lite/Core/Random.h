#pragma once

#include <cstdint>
#include <random>
#include <utility>

namespace Lite {

	class Random
	{
	public:
		static void Seed(uint32_t seed)
		{
			Engine().seed(seed);
		}

		static int Int(int min, int max)
		{
			if (min > max)
				std::swap(min, max);

			std::uniform_int_distribution<int> distribution(min, max);
			return distribution(Engine());
		}

		static float Float()
		{
			return Float(0.0f, 1.0f);
		}

		static float Float(float min, float max)
		{
			if (min > max)
				std::swap(min, max);

			std::uniform_real_distribution<float> distribution(min, max);
			return distribution(Engine());
		}

		static bool Bool()
		{
			return Int(0, 1) == 1;
		}

	private:
		static std::mt19937& Engine()
		{
			static std::mt19937 engine { std::random_device{}() };
			return engine;
		}
	};

}
