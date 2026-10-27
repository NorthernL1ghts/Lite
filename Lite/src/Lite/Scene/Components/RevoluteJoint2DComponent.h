#pragma once

#include <Lite/Scene/Components/Component.h>

#include <string>

namespace Lite {

	struct RevoluteJoint2DComponent
	{
		std::string Other;
		bool Motor = false;
		float MotorSpeed = 0.0f;
		float MaxTorque = 0.0f;
		bool Limit = false;
		float LowerAngle = 0.0f;
		float UpperAngle = 0.0f;
	};

	template<>
	struct ComponentTraits<RevoluteJoint2DComponent>
	{
		static constexpr ComponentId Id = ComponentId::RevoluteJoint2D;
	};

}
