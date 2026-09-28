#pragma once

#include <Lite/Assets/Asset.h>

#include <filesystem>

namespace Lite {

	class AssetHandler
	{
	public:
		virtual ~AssetHandler() = default;

		virtual AssetType GetType() const = 0;
		virtual Ref<Asset> Load(const std::filesystem::path& path) = 0;
	};

}
