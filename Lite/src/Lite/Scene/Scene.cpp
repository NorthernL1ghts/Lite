#include "Scene.h"

#include "Console.h"

#include <format>

namespace Lite {

	namespace {

		Scene* s_Active = nullptr;

	}

	Scene::Scene(std::string name)
		: m_Name(std::move(name))
	{
	}

	Scene::~Scene()
	{
		Stop();
		if (s_Active == this)
			s_Active = nullptr;
	}

	SceneObject* Scene::Find(std::string_view name)
	{
		for (SceneObject& object : m_Objects)
		{
			if (object.Name == name)
				return &object;
		}

		return nullptr;
	}

	void Scene::AddObject(SceneObject object)
	{
		m_Objects.push_back(std::move(object));
	}

	void Scene::Load()
	{
		if (m_Loaded)
			return;

		m_Loaded = true;
		Console::Log(std::format("Scene loaded: {}", m_Name));
		for (const SceneObject& object : m_Objects)
			Console::Log(std::format("  {}", object.Name));
	}

	void Scene::Start()
	{
		if (m_Started)
			return;

		if (!m_Loaded)
			Load();

		m_Started = true;
		Console::Log(std::format("Scene started: {}", m_Name));
	}

	void Scene::Stop()
	{
		if (!m_Started)
			return;

		m_Started = false;
		Console::Log(std::format("Scene stopped: {}", m_Name));
	}

	void Scene::SetActive(Scene* scene)
	{
		s_Active = scene;
	}

	Scene* Scene::GetActive()
	{
		return s_Active;
	}

}
