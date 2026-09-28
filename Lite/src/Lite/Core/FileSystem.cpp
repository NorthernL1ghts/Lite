#include "FileSystem.h"

#include <Windows.h>

#include <fstream>

namespace Lite {

	std::filesystem::path FileSystem::ExecutableDirectory()
	{
		wchar_t modulePath[MAX_PATH] {};
		GetModuleFileNameW(nullptr, modulePath, MAX_PATH);
		return std::filesystem::path(modulePath).parent_path();
	}

	bool FileSystem::Exists(const std::filesystem::path& path)
	{
		return std::filesystem::exists(path);
	}

	std::vector<uint8_t> FileSystem::ReadBinary(const std::filesystem::path& path)
	{
		std::ifstream file(path, std::ios::binary | std::ios::ate);
		if (!file)
			return {};

		auto size = static_cast<std::streamsize>(file.tellg());
		if (size <= 0)
			return {};

		std::vector<uint8_t> bytes(static_cast<size_t>(size));
		file.seekg(0);
		file.read(reinterpret_cast<char*>(bytes.data()), size);
		if (!file)
			return {};

		return bytes;
	}

}
