#pragma once

#include "Asset.h"

#include <filesystem>
#include <memory>

namespace Lite {

	class AssetHandler
	{
	public:
		virtual ~AssetHandler() = default;

		virtual AssetType GetType() const = 0;
		virtual std::shared_ptr<Asset> Load(const std::filesystem::path& path) = 0;
	};

}
