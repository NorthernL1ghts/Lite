#pragma once

#include <Lite/Scene/Components/Component.h>

namespace Lite {

	struct SpinComponent
	{
		float Rate = 0.0f;
	};

	template<>
	struct ComponentTraits<SpinComponent>
	{
		static constexpr ComponentId Id = ComponentId::Spin;
	};

}
