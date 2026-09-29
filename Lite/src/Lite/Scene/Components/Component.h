#pragma once

#include <cstdint>

namespace Lite {

	enum class ComponentId : uint8_t
	{
		Transform,
		Camera,
		Mesh,
		Material,
		Spin,
		Rigidbody2D,
		BoxCollider2D,
		CircleCollider2D,
		Sorting,
		Count
	};

	template<typename T>
	struct ComponentTraits;

	struct ComponentField
	{
		bool InSection = false;
		bool Legacy = false;
		bool Loose = false;
		bool Extra = false;
	};

}
