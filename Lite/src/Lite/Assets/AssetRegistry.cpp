#include "AssetRegistry.h"

#include "Shader.h"
#include "Texture.h"

#include "Lite/Core/Assert.h"

#include <Windows.h>

#include <cctype>

namespace {

	std::filesystem::path ExecutableDirectory()
	{
		wchar_t modulePath[MAX_PATH] {};
		GetModuleFileNameW(nullptr, modulePath, MAX_PATH);
		return std::filesystem::path(modulePath).parent_path();
	}

}

namespace Lite {

	namespace {

		std::unique_ptr<AssetRegistry> s_Registry;

	}

	AssetRegistry::~AssetRegistry() = default;

	void AssetRegistry::Init()
	{
		if (s_Registry)
			return;

		s_Registry.reset(new AssetRegistry());
		s_Registry->m_Root = ExecutableDirectory();
		s_Registry->Register(std::make_unique<ShaderHandler>());
		s_Registry->Register(std::make_unique<TextureHandler>());
		LITE_INFO("Asset registry ready ({})", s_Registry->m_Root.string());
	}

	void AssetRegistry::Shutdown()
	{
		s_Registry.reset();
	}

	AssetRegistry& AssetRegistry::Get()
	{
		LITE_CORE_ASSERT(s_Registry, "Asset registry is not initialised");
		return *s_Registry;
	}

	void AssetRegistry::Register(std::unique_ptr<AssetHandler> handler)
	{
		AssetType type = handler->GetType();
		m_Handlers[type] = std::move(handler);
	}

	std::shared_ptr<Asset> AssetRegistry::LoadAsset(AssetType type, std::string_view path)
	{
		std::string key = Key(path);
		if (auto found = m_Assets.find(key); found != m_Assets.end())
		{
			if (found->second->GetType() != type)
			{
				LITE_ERROR("Asset {} is already loaded as a different type", key);
				return nullptr;
			}

			return found->second;
		}

		auto handler = m_Handlers.find(type);
		if (handler == m_Handlers.end())
		{
			LITE_ERROR("No asset handler for {}", key);
			return nullptr;
		}

		std::shared_ptr<Asset> asset = handler->second->Load(Resolve(path));
		if (!asset)
			return nullptr;

		m_Assets.emplace(key, asset);
		LITE_INFO("Loaded asset {}", key);
		return asset;
	}

	std::shared_ptr<Asset> AssetRegistry::Find(std::string_view path) const
	{
		auto found = m_Assets.find(Key(path));
		if (found == m_Assets.end())
			return nullptr;

		return found->second;
	}

	std::string AssetRegistry::Key(std::string_view path) const
	{
		std::filesystem::path relative(path);
		std::string key = relative.generic_string();
		for (char& character : key)
			character = static_cast<char>(std::tolower(static_cast<unsigned char>(character)));

		return key;
	}

	std::filesystem::path AssetRegistry::Resolve(std::string_view path) const
	{
		std::filesystem::path relative(path);
		if (relative.is_absolute())
			return relative;

		return m_Root / relative;
	}

}
