#pragma once

#include <Lite/Math/Math.h>
#include <Lite/Scene/Components/Component.h>

namespace Lite {

	enum class BodyType
	{
		Static,
		Kinematic,
		Dynamic
	};

	struct Rigidbody2DComponent
	{
		BodyType Type = BodyType::Dynamic;
		float Mass = 1.0f;
		float GravityScale = 1.0f;
		Vec2 LinearVelocity {};
		float AngularVelocity = 0.0f;
		bool FreezeRotation = false;
	};

	template<>
	struct ComponentTraits<Rigidbody2DComponent>
	{
		static constexpr ComponentId Id = ComponentId::Rigidbody2D;
	};

}
