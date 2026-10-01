#pragma once

#include <Lite/Assets/AssetHandler.h>

#include <filesystem>
#include <memory>
#include <string>
#include <string_view>
#include <unordered_map>

namespace Lite {

	class LITE_API AssetRegistry
	{
	public:
		static void Init();
		static void Shutdown();
		static AssetRegistry& Get();

		template<typename T>
		Ref<T> Load(std::string_view path)
		{
			return std::static_pointer_cast<T>(LoadAsset(T::Type, path));
		}

		template<typename T>
		Ref<T> Get(std::string_view path) const
		{
			return std::dynamic_pointer_cast<T>(Find(path));
		}

		const std::filesystem::path& GetRoot() const { return m_Root; }

		std::filesystem::path Resolve(std::string_view path) const;
		std::string Store(std::string_view path) const;

		~AssetRegistry();

	private:
		AssetRegistry() = default;
		AssetRegistry(const AssetRegistry&) = delete;
		AssetRegistry& operator=(const AssetRegistry&) = delete;
		AssetRegistry(AssetRegistry&&) = delete;
		AssetRegistry& operator=(AssetRegistry&&) = delete;

		void Register(Scope<AssetHandler> handler);
		Ref<Asset> LoadAsset(AssetType type, std::string_view path);
		Ref<Asset> Find(std::string_view path) const;
		std::string Key(std::string_view path) const;

		std::filesystem::path m_Root;
		std::unordered_map<AssetType, Scope<AssetHandler>> m_Handlers;
		std::unordered_map<std::string, Ref<Asset>> m_Assets;
	};

}
