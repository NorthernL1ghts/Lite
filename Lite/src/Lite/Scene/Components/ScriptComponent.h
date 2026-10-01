#pragma once

#include <Lite/Scene/Components/Component.h>

#include <string>

namespace Lite {

	struct ScriptComponent
	{
		std::string Class;
	};

	template<>
	struct ComponentTraits<ScriptComponent>
	{
		static constexpr ComponentId Id = ComponentId::Script;
	};

}
