#pragma once

#include <Lite/Scene/Components/Component.h>

namespace Lite {

	struct SortingComponent
	{
		int Order = 0;
	};

	template<>
	struct ComponentTraits<SortingComponent>
	{
		static constexpr ComponentId Id = ComponentId::Sorting;
	};

}
