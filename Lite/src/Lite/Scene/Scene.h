#pragma once

#include "Lite/Assets/Texture.h"
#include "Lite/Core/Base.h"
#include "Lite/Math/Math.h"

#include <iosfwd>
#include <string>
#include <string_view>
#include <vector>

namespace Lite {

	enum class SceneObjectKind
	{
		Quad,
		Sprite,
		Triangle
	};

	enum class ScenePlayback
	{
		Stopped,
		Playing,
		Paused
	};

	struct SceneObject
	{
		std::string Name;
		SceneObjectKind Kind = SceneObjectKind::Quad;
		Transform Transform;
		Vec4 Color { 1.0f, 1.0f, 1.0f, 1.0f };
		Vec4 CornerColors[4] {};
		bool UseCornerColors = false;
		Vec2 Tiling { 1.0f, 1.0f };
		std::string TexturePath;
		Ref<Texture> Texture;
		std::string Shader = "Batch";
		float Spin = 0.0f;
	};

	class LITE_API Scene
	{
	public:
		explicit Scene(std::string name);
		~Scene();

		Scene(const Scene&) = delete;
		Scene& operator=(const Scene&) = delete;

		const std::string& GetName() const { return m_Name; }
		void SetName(std::string name) { m_Name = std::move(name); }
		const std::string& GetPath() const { return m_Path; }
		const std::vector<SceneObject>& GetObjects() const { return m_Objects; }
		SceneObject* Find(std::string_view name);
		ScenePlayback GetPlayback() const { return m_Playback; }

		void AddObject(SceneObject object);
		void Create();
		void Load();
		void Play();
		void Pause();
		void Stop();
		void Update(float seconds);
		void Render() const;

		bool Save();
		bool SaveAs(std::string_view path);
		static Scope<Scene> Open(std::string_view path);
		static std::vector<std::string> List();

		bool IsLoaded() const { return m_Loaded; }
		bool IsPlaying() const { return m_Playback == ScenePlayback::Playing; }

		static void SetActive(Scene* scene);
		static Scene* GetActive();

	private:
		bool Read(std::istream& input);
		void Write(std::ostream& output) const;
		void ResolveTextures();

		std::string m_Name;
		std::string m_Path;
		std::vector<SceneObject> m_Objects;
		bool m_Loaded = false;
		ScenePlayback m_Playback = ScenePlayback::Stopped;
	};

}
