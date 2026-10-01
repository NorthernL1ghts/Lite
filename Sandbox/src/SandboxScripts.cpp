#include <Lite/Scene/Console.h>
#include <Lite/Scene/Scene.h>
#include <Lite/Script/ScriptModule.h>

#include <cmath>
#include <format>

namespace {

	float s_Time = 0.0f;

	void BounceStart(Lite::Entity entity)
	{
		Lite::Console::Log(std::format("{} started", entity.GetName()));
	}

	void BounceUpdate(Lite::Entity entity, float seconds)
	{
		s_Time += seconds;
		Lite::MaterialComponent* material = entity.Get<Lite::MaterialComponent>();
		if (material == nullptr)
			return;

		float pulse = 0.55f + 0.45f * std::sin(s_Time * 6.0f);
		material->Color.x = pulse;
	}

	void BounceCollision(Lite::Entity self, std::uint32_t other)
	{
		std::string name = "entity";
		if (Lite::Scene* scene = Lite::Scene::GetActive())
		{
			Lite::Entity hit = scene->GetEntity(other);
			if (hit && !hit.GetName().empty())
				name = hit.GetName();
		}

		Lite::Console::Log(std::format("{} collided with {} ({})", self.GetName(), name, other));
	}

}

extern "C" __declspec(dllexport) void LiteRegisterScripts(Lite::ScriptAddFn add)
{
	static const Lite::ScriptClass bounce { "Bounce", BounceStart, BounceUpdate, BounceCollision };
	add(&bounce);
}
