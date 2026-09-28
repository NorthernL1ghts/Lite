#include "Random.h"

#include <random>
#include <utility>

namespace Lite {

	namespace {

		std::mt19937& Engine()
		{
			static std::mt19937 engine { std::random_device{}() };
			return engine;
		}

	}

	void Random::Seed(uint32_t seed)
	{
		Engine().seed(seed);
	}

	int Random::Int(int min, int max)
	{
		if (min > max)
			std::swap(min, max);

		std::uniform_int_distribution<int> distribution(min, max);
		return distribution(Engine());
	}

	float Random::Float()
	{
		return Float(0.0f, 1.0f);
	}

	float Random::Float(float min, float max)
	{
		if (min > max)
			std::swap(min, max);

		std::uniform_real_distribution<float> distribution(min, max);
		return distribution(Engine());
	}

	bool Random::Bool()
	{
		return Int(0, 1) == 1;
	}

}
