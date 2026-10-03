#include <Lite/Assets/AssetRegistry.h>
#include <Lite/Core/IO/FileSystem.h>
#include <Lite/Core/Log/Logger.h>
#include <Lite/Scene/Components/Components.h>
#include <Lite/Scene/Scene.h>

#include <cmath>
#include <filesystem>
#include <format>
#include <iostream>
#include <limits>
#include <string>
#include <string_view>

namespace {

	bool Fail(const std::string& message)
	{
		std::cerr << message << '\n';
		return false;
	}

	const Lite::Scene::Plane* FindPlane(const Lite::Scene& scene, std::string_view name)
	{
		uint32_t id = scene.FindPlane(name);
		if (id == 0)
			return nullptr;

		for (const Lite::Scene::Plane& plane : scene.GetPlanes())
		{
			if (plane.Id == id)
				return &plane;
		}

		return nullptr;
	}

	float HalfExtent(float scale, float size)
	{
		float half = std::fabs(scale) * size * 0.5f;
		if (half < 0.01f)
			half = 0.01f;
		return half;
	}

	bool VerticalSpan(Lite::Entity entity, float& bottom, float& top)
	{
		Lite::Transform world = entity.WorldTransform();
		float sine = std::sin(world.GetRotationZ());
		float cosine = std::cos(world.GetRotationZ());
		bottom = std::numeric_limits<float>::infinity();
		top = -bottom;

		auto include = [&](float localX, float localY)
		{
			float y = world.Position.y + localX * sine + localY * cosine;
			bottom = std::min(bottom, y);
			top = std::max(top, y);
		};

		if (const auto* box = entity.Get<Lite::BoxCollider2DComponent>())
		{
			float halfWidth = HalfExtent(world.Scale.x, box->Size.x);
			float halfHeight = HalfExtent(world.Scale.y, box->Size.y);
			float offsetX = box->Offset.x * world.Scale.x;
			float offsetY = box->Offset.y * world.Scale.y;
			include(offsetX - halfWidth, offsetY - halfHeight);
			include(offsetX + halfWidth, offsetY - halfHeight);
			include(offsetX - halfWidth, offsetY + halfHeight);
			include(offsetX + halfWidth, offsetY + halfHeight);
			return true;
		}

		if (const auto* circle = entity.Get<Lite::CircleCollider2DComponent>())
		{
			float scale = std::fabs(world.Scale.x);
			float other = std::fabs(world.Scale.y);
			if (other > scale)
				scale = other;

			float radius = circle->Radius * scale;
			if (radius < 0.01f)
				radius = 0.01f;

			float centerY = world.Position.y + circle->Offset.x * world.Scale.x * sine + circle->Offset.y * world.Scale.y * cosine;
			bottom = centerY - radius;
			top = centerY + radius;
			return true;
		}

		return false;
	}

	bool AboveGround(Lite::Scene& scene, std::string_view name, float groundTop)
	{
		Lite::Entity entity = scene.Find(name);
		if (!entity)
			return Fail(std::format("Scene check: {} was not found", name));

		float bottom = 0.0f;
		float top = 0.0f;
		if (!VerticalSpan(entity, bottom, top))
			return Fail(std::format("Scene check: {} has no collider", name));

		if (!(bottom > groundTop))
			return Fail(std::format("Scene check: {} starts at or below the ground", name));

		return true;
	}

	bool Check(Lite::Scene& scene)
	{
		if (scene.GetPlayback() != Lite::ScenePlayback::Stopped)
			return Fail("Scene check: scene is already playing");

		const Lite::Scene::Plane* background = FindPlane(scene, "Background");
		const Lite::Scene::Plane* world = FindPlane(scene, "World");
		const Lite::Scene::Plane* foreground = FindPlane(scene, "Foreground");
		if (background == nullptr)
			return Fail("Scene check: Background is missing");
		if (world == nullptr)
			return Fail("Scene check: World is missing");
		if (foreground == nullptr)
			return Fail("Scene check: Foreground is missing");
		if (!(background->Order < world->Order))
			return Fail("Scene check: Background is not behind World");
		if (!(world->Order < foreground->Order))
			return Fail("Scene check: World is not behind Foreground");

		Lite::Entity ground = scene.Find("Ground");
		if (!ground)
			return Fail("Scene check: Ground was not found");

		float groundBottom = 0.0f;
		float groundTop = 0.0f;
		if (!VerticalSpan(ground, groundBottom, groundTop))
			return Fail("Scene check: Ground has no collider");

		if (!AboveGround(scene, "Crate", groundTop))
			return false;
		if (!AboveGround(scene, "Ball", groundTop))
			return false;

		return true;
	}

}

int main()
{
	Lite::Logger::Init();
	Lite::AssetRegistry::Init();

	constexpr const char* sceneFile = "Sandbox/assets/scenes/Sandbox.scene";
	int code = 1;
	std::filesystem::path scenePath = Lite::FileSystem::Locate(sceneFile);
	if (scenePath.empty())
	{
		std::cerr << std::format("Scene check: {} was not found\n", sceneFile);
	}
	else if (Lite::Scope<Lite::Scene> scene = Lite::Scene::Open(scenePath.string()))
	{
		if (Check(*scene))
		{
			std::cout << "Scene check passed\n";
			code = 0;
		}
		Lite::Scene::Close(scene);
	}
	else
	{
		std::cerr << std::format("Scene check: {} failed to load\n", sceneFile);
	}

	Lite::AssetRegistry::Shutdown();
	Lite::Logger::Shutdown();
	return code;
}
