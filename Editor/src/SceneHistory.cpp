#include <SceneHistory.h>

namespace {

	constexpr std::size_t kHistoryLimit = 100;

}

void SceneHistory::Clear()
{
	m_Idle.clear();
	m_IdleSelected = static_cast<std::size_t>(-1);
	m_HaveIdle = false;
	m_Undo.clear();
	m_Redo.clear();
}

std::size_t SceneHistory::IndexOf(Lite::Scene& scene, std::uint32_t id)
{
	if (id == 0)
		return static_cast<std::size_t>(-1);

	const std::vector<Lite::Entity> entities = scene.GetEntities();
	for (std::size_t index = 0; index < entities.size(); ++index)
	{
		if (entities[index].GetId() == id)
			return index;
	}

	return static_cast<std::size_t>(-1);
}

std::uint32_t SceneHistory::IdAt(Lite::Scene& scene, std::size_t index)
{
	const std::vector<Lite::Entity> entities = scene.GetEntities();
	if (index >= entities.size())
		return 0;

	return entities[index].GetId();
}

void SceneHistory::Observe(Lite::Scene& scene, std::uint32_t selected, bool idle)
{
	if (!idle)
		return;

	m_Idle = scene.Snapshot();
	m_IdleSelected = IndexOf(scene, selected);
	m_HaveIdle = true;
}

void SceneHistory::Commit(Lite::Scene& scene, std::uint32_t selected)
{
	if (!m_HaveIdle)
	{
		m_Idle = scene.Snapshot();
		m_IdleSelected = IndexOf(scene, selected);
		m_HaveIdle = true;
		return;
	}

	std::string now = scene.Snapshot();
	if (now == m_Idle)
	{
		m_IdleSelected = IndexOf(scene, selected);
		return;
	}

	m_Undo.push_back({ std::move(m_Idle), m_IdleSelected });
	if (m_Undo.size() > kHistoryLimit)
		m_Undo.erase(m_Undo.begin());
	m_Redo.clear();
	m_Idle = std::move(now);
	m_IdleSelected = IndexOf(scene, selected);
}

bool SceneHistory::Apply(Lite::Scene& scene, std::uint32_t& selected, Step& from, std::vector<Step>& to)
{
	Step current { scene.Snapshot(), IndexOf(scene, selected) };
	if (!scene.Restore(from.Document))
		return false;

	to.push_back(std::move(current));
	selected = IdAt(scene, from.Selected);
	m_Idle = from.Document;
	m_IdleSelected = from.Selected;
	m_HaveIdle = true;
	return true;
}

bool SceneHistory::Undo(Lite::Scene& scene, std::uint32_t& selected)
{
	if (m_Undo.empty())
		return false;

	Step step = std::move(m_Undo.back());
	m_Undo.pop_back();
	if (Apply(scene, selected, step, m_Redo))
		return true;

	m_Undo.push_back(std::move(step));
	return false;
}

bool SceneHistory::Redo(Lite::Scene& scene, std::uint32_t& selected)
{
	if (m_Redo.empty())
		return false;

	Step step = std::move(m_Redo.back());
	m_Redo.pop_back();
	if (Apply(scene, selected, step, m_Undo))
		return true;

	m_Redo.push_back(std::move(step));
	return false;
}
