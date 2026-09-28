#pragma once

#include "Lite/Core/Base.h"

#include <string>

namespace Lite {

	enum class AssetType
	{
		None = 0,
		Shader,
		Texture
	};

	class LITE_API Asset
	{
	public:
		virtual ~Asset() = default;

		virtual AssetType GetType() const = 0;

		const std::string& GetName() const { return m_Name; }
		const std::string& GetPath() const { return m_Path; }
		bool IsLoaded() const { return m_Loaded; }

	protected:
		void SetIdentity(std::string path, std::string name)
		{
			m_Path = std::move(path);
			m_Name = std::move(name);
		}

		std::string m_Path;
		std::string m_Name;
		bool m_Loaded = false;
	};

}
