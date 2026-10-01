#include <Lite/Assets/AssetRegistry.h>

#include <Lite/Assets/Shader.h>
#include <Lite/Assets/Texture.h>

#include <Lite/Core/Assert.h>
#include <Lite/Core/IO/FileSystem.h>
#include <Lite/Core/String.h>
#include <Lite/Project/Project.h>

#include <vector>

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

	namespace {

		std::filesystem::path Canonical(const std::filesystem::path& path)
		{
			std::error_code error;
			std::filesystem::path canonical = std::filesystem::weakly_canonical(path, error);
			return error ? path.lexically_normal() : canonical;
		}

		std::filesystem::path ProjectAssets()
		{
			if (!Project::GetActive())
				return {};

			std::filesystem::path assets = Project::GetAssetDirectory();
			if (assets.empty())
				return {};

			return Canonical(assets);
		}

		std::filesystem::path WithoutAssetsPrefix(const std::filesystem::path& path)
		{
			auto it = path.begin();
			if (it == path.end() || ToLower(it->generic_string()) != "assets")
				return {};

			std::filesystem::path tail;
			for (++it; it != path.end(); ++it)
				tail /= *it;
			return tail;
		}

		std::string RelativeTo(const std::filesystem::path& root, const std::filesystem::path& file)
		{
			if (root.empty() || !FileSystem::Contains(root, file))
				return {};

			std::error_code error;
			std::filesystem::path relative = std::filesystem::relative(file, root, error);
			if (error)
				return {};

			return relative.generic_string();
		}

	}

	std::string AssetRegistry::Key(std::string_view path) const
	{
		std::filesystem::path resolved = Resolve(path);
		if (resolved.empty())
			return ToLower(std::filesystem::path(path).generic_string());

		return ToLower(resolved.generic_string());
	}

	std::filesystem::path AssetRegistry::Resolve(std::string_view path) const
	{
		if (path.empty())
			return {};

		std::filesystem::path file { std::string(path) };
		const std::filesystem::path assets = ProjectAssets();
		if (file.is_absolute())
			return FileSystem::Exists(file) ? Canonical(file) : file.lexically_normal();

		std::vector<std::filesystem::path> candidates;
		if (!assets.empty())
		{
			candidates.push_back(assets / file);
			const std::filesystem::path tail = WithoutAssetsPrefix(file);
			if (!tail.empty())
				candidates.push_back(assets / tail);
		}
		candidates.push_back(Canonical(m_Root) / file);

		for (const std::filesystem::path& candidate : candidates)
		{
			if (FileSystem::Exists(candidate))
				return Canonical(candidate);
		}

		if (!assets.empty())
			return (assets / file).lexically_normal();

		return (m_Root / file).lexically_normal();
	}

	std::string AssetRegistry::Store(std::string_view path) const
	{
		if (path.empty())
			return {};

		const std::filesystem::path resolved = Resolve(path);
		const std::filesystem::path assets = ProjectAssets();
		const std::string fromProject = RelativeTo(assets, resolved);
		if (!fromProject.empty())
			return fromProject;

		const std::string fromExecutable = RelativeTo(Canonical(m_Root), resolved);
		if (!assets.empty() && !fromExecutable.empty())
		{
			const std::filesystem::path tail = WithoutAssetsPrefix(fromExecutable);
			const std::filesystem::path projectFile = assets / (tail.empty() ? std::filesystem::path(fromExecutable) : tail);
			if (FileSystem::Exists(projectFile))
			{
				const std::string stored = RelativeTo(assets, Canonical(projectFile));
				if (!stored.empty())
					return stored;
			}
		}

		if (resolved.is_absolute())
			return resolved.generic_string();

		return std::filesystem::path(std::string(path)).generic_string();
	}

}
