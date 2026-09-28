#pragma once

#include "Base.h"

#include <cstdint>
#include <filesystem>
#include <vector>

namespace Lite {

	class LITE_API FileSystem
	{
	public:
		static std::filesystem::path ExecutableDirectory();
		static bool Exists(const std::filesystem::path& path);
		static std::vector<uint8_t> ReadBinary(const std::filesystem::path& path);
	};

}
