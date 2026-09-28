#include <Lite/Core/IO/FileSystem.h>

#include <Windows.h>

#include <fstream>

namespace Lite {

	std::filesystem::path FileSystem::ExecutableDirectory()
	{
		wchar_t modulePath[MAX_PATH] {};
		DWORD length = GetModuleFileNameW(nullptr, modulePath, MAX_PATH);
		if (length == 0 || length >= MAX_PATH)
			return {};

		return std::filesystem::path(modulePath).parent_path();
	}

	bool FileSystem::Exists(const std::filesystem::path& path)
	{
		std::error_code error;
		return std::filesystem::exists(path, error);
	}

	std::vector<uint8_t> FileSystem::ReadBinary(const std::filesystem::path& path)
	{
		std::error_code error;
		auto size = std::filesystem::file_size(path, error);
		if (error || size == 0)
			return {};

		std::ifstream file(path, std::ios::binary);
		if (!file)
			return {};

		std::vector<uint8_t> bytes(static_cast<size_t>(size));
		file.read(reinterpret_cast<char*>(bytes.data()), static_cast<std::streamsize>(bytes.size()));
		if (!file)
			return {};

		return bytes;
	}

}
