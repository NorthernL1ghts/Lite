#pragma once

#include "Lite/Assets/Texture.h"
#include "Lite/Core/Base.h"
#include "Lite/Math/Math.h"

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

	struct SceneObject
	{
		std::string Name;
		SceneObjectKind Kind = SceneObjectKind::Quad;
		Transform Transform;
		Vec4 Color { 1.0f, 1.0f, 1.0f, 1.0f };
		Vec4 CornerColors[4] {};
		bool UseCornerColors = false;
		Vec2 Tiling { 1.0f, 1.0f };
		Ref<Texture> Texture;
	};

	class LITE_API Scene
	{
	public:
		explicit Scene(std::string name);
		~Scene();

		Scene(const Scene&) = delete;
		Scene& operator=(const Scene&) = delete;

		const std::string& GetName() const { return m_Name; }
		const std::vector<SceneObject>& GetObjects() const { return m_Objects; }
		SceneObject* Find(std::string_view name);

		void AddObject(SceneObject object);
		void Load();
		void Start();
		void Stop();

		bool IsLoaded() const { return m_Loaded; }
		bool IsStarted() const { return m_Started; }

		static void SetActive(Scene* scene);
		static Scene* GetActive();

	private:
		std::string m_Name;
		std::vector<SceneObject> m_Objects;
		bool m_Loaded = false;
		bool m_Started = false;
	};

}
