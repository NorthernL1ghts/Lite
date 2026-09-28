#pragma once

#include "Base.h"

#include <cstdint>
#include <filesystem>
#include <vector>

namespace Lite {

	class LITE_API FileSystem
	{
	public:
		[[nodiscard]] static std::filesystem::path ExecutableDirectory();
		[[nodiscard]] static bool Exists(const std::filesystem::path& path);
		[[nodiscard]] static std::vector<uint8_t> ReadBinary(const std::filesystem::path& path);
	};

}
