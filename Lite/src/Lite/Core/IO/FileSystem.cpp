#include <Lite/Core/IO/FileSystem.h>

#include <Windows.h>

#include <fstream>
#include <vector>

namespace Lite {

	namespace {

		void Consider(const std::filesystem::path& candidate, std::vector<std::filesystem::path>& matches)
		{
			std::error_code error;
			if (!std::filesystem::is_regular_file(candidate, error) && !std::filesystem::is_directory(candidate, error))
				return;

			std::filesystem::path normal = candidate.lexically_normal();
			for (const std::filesystem::path& match : matches)
			{
				if (match == normal)
					return;
			}

			matches.push_back(std::move(normal));
		}

		void Collect(const std::filesystem::path& start, const std::filesystem::path& relative, std::vector<std::filesystem::path>& matches)
		{
			std::filesystem::path cursor = start;
			for (int step = 0; step < 8 && !cursor.empty(); ++step)
			{
				Consider(cursor / relative, matches);

				std::error_code error;
				if (std::filesystem::is_directory(cursor, error))
				{
					for (const std::filesystem::directory_entry& entry : std::filesystem::directory_iterator(cursor, error))
					{
						std::error_code childError;
						if (error || !entry.is_directory(childError))
							continue;
						Consider(entry.path() / relative, matches);
					}
				}

				std::filesystem::path parent = cursor.parent_path();
				if (parent.empty() || parent == cursor)
					break;
				cursor = parent;
			}
		}

		std::filesystem::path PreferSource(const std::vector<std::filesystem::path>& matches)
		{
			std::filesystem::path executable = FileSystem::ExecutableDirectory();
			std::filesystem::path inside;
			for (const std::filesystem::path& match : matches)
			{
				std::error_code error;
				std::filesystem::path relative = std::filesystem::relative(match, executable, error);
				bool contained = !error && (relative.empty() || relative.begin()->string() != "..");
				if (!contained)
					return match;
				if (inside.empty())
					inside = match;
			}

			return inside;
		}

	}

	std::filesystem::path FileSystem::ExecutableDirectory()
	{
		wchar_t modulePath[MAX_PATH] {};
		DWORD length = GetModuleFileNameW(nullptr, modulePath, MAX_PATH);
		if (length == 0 || length >= MAX_PATH)
			return {};

		return std::filesystem::path(modulePath).parent_path();
	}

	std::filesystem::path FileSystem::Locate(const std::filesystem::path& relative)
	{
		if (relative.empty())
			return {};

		std::vector<std::filesystem::path> matches;
		std::error_code error;
		Collect(std::filesystem::current_path(error), relative, matches);
		Collect(ExecutableDirectory(), relative, matches);

		std::filesystem::path chosen = PreferSource(matches);
		if (chosen.empty())
			return {};

		std::filesystem::path canonical = std::filesystem::weakly_canonical(chosen, error);
		return error ? chosen : canonical;
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
