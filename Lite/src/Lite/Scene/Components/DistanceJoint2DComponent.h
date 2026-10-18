#pragma once

#include <Lite/Scene/Components/Component.h>

#include <string>

namespace Lite {

	struct DistanceJoint2DComponent
	{
		std::string Other;
		float Length = 0.0f;
	};

	template<>
	struct ComponentTraits<DistanceJoint2DComponent>
	{
		static constexpr ComponentId Id = ComponentId::DistanceJoint2D;
	};

}
