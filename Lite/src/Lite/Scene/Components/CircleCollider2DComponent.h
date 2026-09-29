#pragma once

#include <Lite/Math/Math.h>
#include <Lite/Scene/Components/Component.h>

namespace Lite {

	struct CircleCollider2DComponent
	{
		float Radius = 0.5f;
		Vec2 Offset {};
		bool IsTrigger = false;
	};

	template<>
	struct ComponentTraits<CircleCollider2DComponent>
	{
		static constexpr ComponentId Id = ComponentId::CircleCollider2D;
	};

}
