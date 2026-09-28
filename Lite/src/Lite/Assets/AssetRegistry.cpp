#include "AssetRegistry.h"

#include "Shader.h"
#include "Texture.h"

#include "Lite/Core/Assert.h"
#include "Lite/Core/FileSystem.h"
#include "Lite/Core/String.h"

namespace Lite {

	namespace {

		Scope<AssetRegistry> s_Registry;

	}

	AssetRegistry::~AssetRegistry() = default;

	void AssetRegistry::Init()
	{
		if (s_Registry)
			return;

		s_Registry = Scope<AssetRegistry>(new AssetRegistry());
		s_Registry->m_Root = FileSystem::ExecutableDirectory();
		s_Registry->Register(CreateScope<ShaderHandler>());
		s_Registry->Register(CreateScope<TextureHandler>());
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

	void AssetRegistry::Register(Scope<AssetHandler> handler)
	{
		AssetType type = handler->GetType();
		m_Handlers[type] = std::move(handler);
	}

	Ref<Asset> AssetRegistry::LoadAsset(AssetType type, std::string_view path)
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

		Ref<Asset> asset = handler->second->Load(Resolve(path));
		if (!asset)
			return nullptr;

		m_Assets.emplace(key, asset);
		LITE_INFO("Loaded asset {} ({})", key, asset->GetID().ToString());
		return asset;
	}

	Ref<Asset> AssetRegistry::Find(std::string_view path) const
	{
		auto found = m_Assets.find(Key(path));
		if (found == m_Assets.end())
			return nullptr;

		return found->second;
	}

	std::string AssetRegistry::Key(std::string_view path) const
	{
		return ToLower(std::filesystem::path(path).generic_string());
	}

	std::filesystem::path AssetRegistry::Resolve(std::string_view path) const
	{
		std::filesystem::path relative(path);
		if (relative.is_absolute())
			return relative;

		return m_Root / relative;
	}

}
