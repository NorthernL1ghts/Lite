#include <Lite/Scene/Console.h>
#include <Lite/Scene/Scene.h>
#include <Lite/Script/ScriptModule.h>

#include <cmath>
#include <format>
#include <string>

namespace {

	float s_BounceTime = 0.0f;
	float s_PatrolTime = 0.0f;
	float s_PatrolOrigin = 0.0f;
	float s_SensorFlash = 0.0f;

	std::string OtherName(std::uint32_t other)
	{
		if (Lite::Scene* scene = Lite::Scene::GetActive())
		{
			Lite::Entity hit = scene->GetEntity(other);
			if (hit && !hit.GetName().empty())
				return hit.GetName();
		}

		return "entity";
	}

	void BounceStart(Lite::Entity entity)
	{
		Lite::Console::Log(std::format("{} started", entity.GetName()));
	}

	void BounceUpdate(Lite::Entity entity, float seconds)
	{
		s_BounceTime += seconds;
		Lite::MaterialComponent* material = entity.Get<Lite::MaterialComponent>();
		if (material == nullptr)
			return;

		material->Color.x = 0.55f + 0.45f * std::sin(s_BounceTime * 6.0f);
	}

	void BounceCollision(Lite::Entity self, std::uint32_t other)
	{
		Lite::Console::Log(std::format("{} collided with {} ({})", self.GetName(), OtherName(other), other));
	}

	void PatrolStart(Lite::Entity entity)
	{
		if (Lite::TransformComponent* transform = entity.Get<Lite::TransformComponent>())
			s_PatrolOrigin = transform->Local.Position.x;
		Lite::Console::Log(std::format("{} started", entity.GetName()));
	}

	void PatrolUpdate(Lite::Entity entity, float seconds)
	{
		s_PatrolTime += seconds;
		Lite::TransformComponent* transform = entity.Get<Lite::TransformComponent>();
		if (transform == nullptr)
			return;

		transform->Local.Position.x = s_PatrolOrigin + std::sin(s_PatrolTime * 0.85f) * 1.35f;
	}

	void SensorUpdate(Lite::Entity entity, float seconds)
	{
		s_SensorFlash -= seconds * 1.6f;
		if (s_SensorFlash < 0.0f)
			s_SensorFlash = 0.0f;

		Lite::MaterialComponent* material = entity.Get<Lite::MaterialComponent>();
		if (material == nullptr)
			return;

		material->Color.w = 0.35f + 0.55f * s_SensorFlash;
	}

	void SensorCollision(Lite::Entity self, std::uint32_t other)
	{
		s_SensorFlash = 1.0f;
		Lite::Console::Log(std::format("{} overlapped {} ({})", self.GetName(), OtherName(other), other));
	}

}

extern "C" __declspec(dllexport) void LiteRegisterScripts(Lite::ScriptAddFn add)
{
	static const Lite::ScriptClass bounce { "Bounce", BounceStart, BounceUpdate, BounceCollision };
	static const Lite::ScriptClass patrol { "Patrol", PatrolStart, PatrolUpdate, nullptr };
	static const Lite::ScriptClass sensor { "Sensor", nullptr, SensorUpdate, SensorCollision };
	add(&bounce);
	add(&patrol);
	add(&sensor);
}
