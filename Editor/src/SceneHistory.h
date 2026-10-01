#pragma once

#include <Lite/Scene/Scene.h>

#include <cstddef>
#include <string>
#include <vector>

class SceneHistory
{
public:
	void Clear();
	void Observe(Lite::Scene& scene, std::uint32_t selected, bool idle);
	void Commit(Lite::Scene& scene, std::uint32_t selected);
	bool Undo(Lite::Scene& scene, std::uint32_t& selected);
	bool Redo(Lite::Scene& scene, std::uint32_t& selected);

private:
	struct Step
	{
		std::string Document;
		std::size_t Selected = static_cast<std::size_t>(-1);
	};

	static std::size_t IndexOf(Lite::Scene& scene, std::uint32_t id);
	static std::uint32_t IdAt(Lite::Scene& scene, std::size_t index);
	bool Apply(Lite::Scene& scene, std::uint32_t& selected, Step& from, std::vector<Step>& to);

	std::string m_Idle;
	std::size_t m_IdleSelected = static_cast<std::size_t>(-1);
	bool m_HaveIdle = false;
	std::vector<Step> m_Undo;
	std::vector<Step> m_Redo;
};
