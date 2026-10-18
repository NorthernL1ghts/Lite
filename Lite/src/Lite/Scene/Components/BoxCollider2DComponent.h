#pragma once

#include <Lite/Math/Math.h>
#include <Lite/Scene/Components/Component.h>

namespace Lite {

	struct BoxCollider2DComponent
	{
		Vec2 Size { 1.0f, 1.0f };
		Vec2 Offset {};
		bool IsTrigger = false;
		uint64_t Layer = kDefaultCollisionLayer;
		uint64_t Mask = kDefaultCollisionMask;
	};

	template<>
	struct ComponentTraits<BoxCollider2DComponent>
	{
		static constexpr ComponentId Id = ComponentId::BoxCollider2D;
	};

}
