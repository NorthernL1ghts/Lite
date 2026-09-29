#pragma once

#include <Lite/Scene/Components/Component.h>

namespace Lite {

	enum class MeshType
	{
		Quad,
		Triangle,
		Sprite
	};

	struct MeshComponent
	{
		MeshType Type = MeshType::Quad;
	};

	template<>
	struct ComponentTraits<MeshComponent>
	{
		static constexpr ComponentId Id = ComponentId::Mesh;
	};

}
