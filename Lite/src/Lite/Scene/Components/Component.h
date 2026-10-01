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
		Script,
		Count
	};

	template<typename T>
	struct ComponentTraits;

}
