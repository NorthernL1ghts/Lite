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
		DistanceJoint2D,
		RevoluteJoint2D,
		Count
	};

	inline constexpr uint64_t kDefaultCollisionLayer = 1;
	inline constexpr uint64_t kDefaultCollisionMask = ~uint64_t{0};
	inline constexpr float kDefaultFriction = 0.5f;
	inline constexpr float kDefaultRestitution = 0.15f;

	template<typename T>
	struct ComponentTraits;

}
