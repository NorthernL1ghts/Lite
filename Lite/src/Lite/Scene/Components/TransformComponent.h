#pragma once

#include <Lite/Math/Math.h>
#include <Lite/Scene/Components/Component.h>

namespace Lite {

	struct TransformComponent
	{
		Transform Local;
	};

	template<>
	struct ComponentTraits<TransformComponent>
	{
		static constexpr ComponentId Id = ComponentId::Transform;
	};

}
