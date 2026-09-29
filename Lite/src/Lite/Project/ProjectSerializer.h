#pragma once

#include <Lite/Project/Project.h>

#include <filesystem>

namespace Lite {

	class ProjectSerializer
	{
	public:
		explicit ProjectSerializer(Ref<Project> project);

		bool Serialize(const std::filesystem::path& filepath);
		bool Deserialize(const std::filesystem::path& filepath);

	private:
		Ref<Project> m_Project;
	};

}
