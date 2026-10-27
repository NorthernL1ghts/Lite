#include <Lite/Assets/AssetRegistry.h>
#include <Lite/Core/IO/FileSystem.h>
#include <Lite/Core/Log/Logger.h>
#include <Lite/Project/Project.h>
#include <Lite/Scene/Components/Components.h>
#include <Lite/Scene/Console.h>
#include <Lite/Scene/Scene.h>
#include <Lite/Script/ScriptModule.h>

#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <Windows.h>

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
		if (scene.IsDirty())
			return Fail("Scene check: the loaded scene is dirty");

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

		Lite::Entity crate = scene.Find("Crate");
		float startY = crate.WorldTransform().Position.y;
		scene.Play();
		constexpr int kFrames = 120;
		for (int frame = 0; frame < kFrames; ++frame)
			scene.Update(Lite::kPhysicsStep);

		crate = scene.Find("Crate");
		if (!crate)
			return Fail("Scene check: Crate was lost during play");

		Lite::Vec3 ended = crate.WorldTransform().Position;
		std::cout << std::format("Crate ended at {:.3f}, {:.3f}\n", ended.x, ended.y);
		if (!(ended.y < startY - 0.5f))
			return Fail("Scene check: the crate did not fall");
		if (ended.y < -3.0f)
			return Fail("Scene check: the crate fell through the ground");
		if (scene.IsDirty())
			return Fail("Scene check: play marked the scene dirty");

		scene.Stop();
		if (scene.IsDirty())
			return Fail("Scene check: stop marked the scene dirty");

		return true;
	}

	Lite::Entity AddBox(Lite::Scene& scene, const char* name, Lite::Vec2 position, Lite::BodyType type)
	{
		Lite::Entity entity = scene.CreateEntity(name);
		if (Lite::TransformComponent* transform = entity.Get<Lite::TransformComponent>())
			transform->Local.Position = { position.x, position.y, 0.0f };

		Lite::Rigidbody2DComponent& body = entity.Add<Lite::Rigidbody2DComponent>();
		body.Type = type;
		body.FreezeRotation = true;
		entity.Add<Lite::BoxCollider2DComponent>();
		return entity;
	}

	bool CheckQueries()
	{
		Lite::Scene scene("Queries");
		scene.Create();
		if (scene.IsDirty())
			return Fail("Scene check: a new scene is dirty");

		Lite::Entity solid = AddBox(scene, "Solid", { 0.0f, 2.0f }, Lite::BodyType::Static);
		Lite::Entity trigger = AddBox(scene, "Trigger", { 0.0f, 4.0f }, Lite::BodyType::Static);
		trigger.Get<Lite::BoxCollider2DComponent>()->IsTrigger = true;
		scene.SetTag(solid.GetId(), "marked");
		solid.Get<Lite::BoxCollider2DComponent>()->Layer = 2;
		solid.Get<Lite::BoxCollider2DComponent>()->Mask = 4;
		solid.Get<Lite::BoxCollider2DComponent>()->Friction = 0.2f;
		solid.Get<Lite::BoxCollider2DComponent>()->Restitution = 0.8f;

		const std::string document = scene.Snapshot();
		if (!scene.Restore(document))
			return Fail("Scene check: snapshot restore failed");

		solid = scene.Find("Solid");
		const Lite::BoxCollider2DComponent* box = solid.Get<Lite::BoxCollider2DComponent>();
		if (box == nullptr || box->Layer != 2 || box->Mask != 4)
			return Fail("Scene check: collider layer did not round-trip");
		if (box->Friction != 0.2f || box->Restitution != 0.8f)
			return Fail("Scene check: collider surface did not round-trip");

		trigger = scene.Find("Trigger");
		const Lite::BoxCollider2DComponent* sensor = trigger.Get<Lite::BoxCollider2DComponent>();
		if (sensor == nullptr || sensor->Friction != Lite::kDefaultFriction || sensor->Restitution != Lite::kDefaultRestitution)
			return Fail("Scene check: a default collider surface was rewritten");

		std::vector<Lite::Entity> tagged = scene.FindTagged("marked");
		if (tagged.size() != 1 || tagged[0].GetId() != solid.GetId())
			return Fail("Scene check: tag lookup failed");

		Lite::Entity copy = scene.DuplicateEntity(solid.GetId());
		if (scene.GetTag(copy.GetId()) != "marked")
			return Fail("Scene check: duplicate dropped the tag");

		box = solid.Get<Lite::BoxCollider2DComponent>();
		if (box == nullptr)
			return Fail("Scene check: Solid lost its collider");

		Lite::BoxCollider2DComponent* editable = solid.Get<Lite::BoxCollider2DComponent>();
		editable->Layer = Lite::kDefaultCollisionLayer;
		editable->Mask = Lite::kDefaultCollisionMask;

		if (!scene.IsDirty())
			return Fail("Scene check: creating an entity left the scene clean");

		scene.SetGravity({ 0.0f, -3.0f });
		const std::string gravityDocument = scene.Snapshot();
		if (!scene.Restore(gravityDocument))
			return Fail("Scene check: gravity snapshot restore failed");
		if (scene.GetGravity().y != -3.0f)
			return Fail("Scene check: gravity did not round-trip");
		solid = scene.Find("Solid");
		trigger = scene.Find("Trigger");
		scene.MarkDirty();

		scene.Play();
		std::optional<Lite::RayHit> hit = scene.Raycast({ 0.0f, 6.0f }, { 0.0f, -8.0f });
		if (!hit || hit->Entity != solid.GetId())
			return Fail("Scene check: raycast missed the solid box");
		if (scene.Raycast({ 10.0f, 6.0f }, { 0.0f, -1.0f }))
			return Fail("Scene check: a miss returned a hit");

		std::vector<std::uint32_t> overlapped = scene.Overlap({ 0.0f, 2.0f }, { 0.25f, 0.25f });
		bool foundSolid = false;
		for (std::uint32_t id : overlapped)
			foundSolid = foundSolid || id == solid.GetId();
		if (!foundSolid)
			return Fail("Scene check: overlap missed the solid box");
		for (std::uint32_t id : scene.Overlap({ 0.0f, 4.0f }, { 0.2f, 0.2f }))
		{
			if (id == trigger.GetId())
				return Fail("Scene check: overlap hit a trigger");
		}
		if (!scene.Overlap({ 40.0f, 40.0f }, { 0.2f, 0.2f }).empty())
			return Fail("Scene check: an empty overlap returned a body");

		scene.SetGravity({ 0.0f, 0.0f });
		Lite::Entity floater = AddBox(scene, "Floater", { 6.0f, 3.0f }, Lite::BodyType::Dynamic);
		floater.Get<Lite::Rigidbody2DComponent>()->FreezeRotation = false;
		scene.RefreshPhysics(floater.GetId());
		floater.Get<Lite::Rigidbody2DComponent>()->AngularVelocity = 4.0f;
		const float startY = floater.WorldTransform().Position.y;
		scene.Update(Lite::kPhysicsStep);
		if (std::fabs(scene.Find("Floater").WorldTransform().Position.y - startY) > 0.02f)
			return Fail("Scene check: zero gravity still pulled the body");
		if (std::fabs(scene.Find("Floater").WorldTransform().GetRotationZ()) < 0.01f)
			return Fail("Scene check: angular velocity was ignored");

		scene.Stop();
		if (!scene.IsDirty())
			return Fail("Scene check: stop cleared an unsaved edit");
		if (scene.Raycast({ 0.0f, 6.0f }, { 0.0f, -8.0f }))
			return Fail("Scene check: raycast hit while stopped");
		if (!scene.Overlap({ 0.0f, 2.0f }, { 0.25f, 0.25f }).empty())
			return Fail("Scene check: overlap hit while stopped");

		return true;
	}

	bool CheckLayersAndJoint()
	{
		Lite::Scene scene("Layers");
		scene.Create();
		Lite::Entity ground = AddBox(scene, "Ground", { 0.0f, 0.0f }, Lite::BodyType::Static);
		ground.Get<Lite::BoxCollider2DComponent>()->Size = { 20.0f, 1.0f };

		Lite::Entity dropped = AddBox(scene, "Dropped", { 4.0f, 4.0f }, Lite::BodyType::Dynamic);
		Lite::Entity left = AddBox(scene, "Left", { 0.0f, 4.0f }, Lite::BodyType::Dynamic);
		Lite::Entity right = AddBox(scene, "Right", { 0.0f, 4.0f }, Lite::BodyType::Dynamic);
		left.Get<Lite::BoxCollider2DComponent>()->Layer = 2;
		left.Get<Lite::BoxCollider2DComponent>()->Mask = 1 | 2;
		right.Get<Lite::BoxCollider2DComponent>()->Layer = 4;
		right.Get<Lite::BoxCollider2DComponent>()->Mask = 1 | 4;

		Lite::Entity pull = AddBox(scene, "Pull", { 10.0f, 8.0f }, Lite::BodyType::Dynamic);
		Lite::Entity anchor = AddBox(scene, "Anchor", { 12.0f, 8.0f }, Lite::BodyType::Dynamic);
		pull.Add<Lite::DistanceJoint2DComponent>().Other = "Anchor";
		pull.Get<Lite::Rigidbody2DComponent>()->GravityScale = 0.0f;
		anchor.Get<Lite::Rigidbody2DComponent>()->GravityScale = 0.0f;
		pull.Get<Lite::Rigidbody2DComponent>()->LinearVelocity = { -2.0f, 0.0f };
		anchor.Get<Lite::Rigidbody2DComponent>()->LinearVelocity = { 2.0f, 0.0f };

		scene.Play();
		for (int frame = 0; frame < 90; ++frame)
			scene.Update(Lite::kPhysicsStep);

		Lite::Vec3 droppedAt = scene.Find("Dropped").WorldTransform().Position;
		if (!(droppedAt.y < 3.0f) || droppedAt.y < 0.2f)
			return Fail("Scene check: the default layer did not land on the ground");

		Lite::Vec3 leftAt = scene.Find("Left").WorldTransform().Position;
		Lite::Vec3 rightAt = scene.Find("Right").WorldTransform().Position;
		if (std::fabs(leftAt.x - rightAt.x) > 0.5f)
			return Fail("Scene check: excluded layers blocked each other");
		if (leftAt.y < 0.2f || rightAt.y < 0.2f || leftAt.y > 3.0f || rightAt.y > 3.0f)
			return Fail("Scene check: a layered body missed the ground");

		Lite::Vec3 pullAt = scene.Find("Pull").WorldTransform().Position;
		Lite::Vec3 anchorAt = scene.Find("Anchor").WorldTransform().Position;
		const float dx = pullAt.x - anchorAt.x;
		const float dy = pullAt.y - anchorAt.y;
		const float distance = std::sqrt(dx * dx + dy * dy);
		if (std::fabs(distance - 2.0f) > 0.35f)
			return Fail("Scene check: the distance joint did not hold");

		Lite::Entity pin = AddBox(scene, "Pin", { 24.0f, 6.0f }, Lite::BodyType::Static);
		Lite::Entity bar = AddBox(scene, "Bar", { 24.0f, 7.2f }, Lite::BodyType::Dynamic);
		scene.RefreshPhysics(pin.GetId());
		scene.RefreshPhysics(bar.GetId());
		bar.Add<Lite::RevoluteJoint2DComponent>().Other = "Pin";
		scene.RefreshPhysics(bar.GetId());

		Lite::Entity pushed = AddBox(scene, "Pushed", { 30.0f, 6.0f }, Lite::BodyType::Dynamic);
		pushed.Get<Lite::Rigidbody2DComponent>()->GravityScale = 0.0f;
		scene.RefreshPhysics(pushed.GetId());
		const float pushedStart = pushed.WorldTransform().Position.x;
		scene.ApplyLinearImpulse(pushed.GetId(), { 2.0f, 0.0f });
		scene.Update(Lite::kPhysicsStep);
		if (!(scene.Find("Pushed").WorldTransform().Position.x > pushedStart + 0.01f))
			return Fail("Scene check: an impulse did not move the body");

		for (int frame = 0; frame < 90; ++frame)
			scene.Update(Lite::kPhysicsStep);

		Lite::Vec3 barAt = scene.Find("Bar").WorldTransform().Position;
		Lite::Vec3 pinAt = scene.Find("Pin").WorldTransform().Position;
		const float hingeX = barAt.x - pinAt.x;
		const float hingeY = barAt.y - pinAt.y;
		if (std::sqrt(hingeX * hingeX + hingeY * hingeY) > 0.5f || barAt.y < 4.5f)
			return Fail("Scene check: the revolute joint did not hold");

		scene.DestroyEntity(scene.Find("Pin").GetId(), false);
		if (scene.Find("Bar").Get<Lite::RevoluteJoint2DComponent>() != nullptr)
			return Fail("Scene check: deleting one end left the revolute joint");

		scene.DestroyEntity(scene.Find("Anchor").GetId(), false);
		if (scene.Find("Pull").Get<Lite::DistanceJoint2DComponent>() != nullptr)
			return Fail("Scene check: deleting one end left the joint");

		scene.Stop();
		return true;
	}

	bool CheckSceneSwap()
	{
		std::filesystem::path directory = std::filesystem::temp_directory_path() / "lite-scene-check";
		std::error_code error;
		std::filesystem::remove_all(directory, error);
		std::filesystem::create_directories(directory, error);
		if (error)
			return Fail("Scene check: could not create a temp directory");

		const std::filesystem::path firstPath = directory / "first.scene";
		const std::filesystem::path secondPath = directory / "second.scene";

		{
			Lite::Scene first("First");
			first.Create();
			first.CreateEntity("Keep");
			if (!first.SaveAs(firstPath.string()))
				return Fail("Scene check: could not save the first scene");

			Lite::Scene second("Second");
			second.Create();
			second.CreateEntity("Swap");
			if (!second.SaveAs(secondPath.string()))
				return Fail("Scene check: could not save the second scene");
		}

		Lite::Scope<Lite::Scene> scene = Lite::Scene::Open(firstPath.string());
		if (!scene)
			return Fail("Scene check: the temp scene did not load");
		if (scene->IsDirty())
			return Fail("Scene check: a loaded scene is dirty");

		scene->Play();
		scene->RequestScene(secondPath.string());
		scene->Update(Lite::kPhysicsStep);
		if (!scene->IsPlaying() || !scene->Find("Swap") || scene->Find("Keep"))
			return Fail("Scene check: the scene did not swap");

		scene->Stop();
		if (scene->IsPlaying() || scene->Find("Swap") || !scene->Find("Keep"))
			return Fail("Scene check: stop did not return to the scene from play");
		if (scene->IsDirty())
			return Fail("Scene check: a scene swap marked the scene dirty");

		scene->Play();
		scene->RequestScene("missing-lite-scene.scene");
		scene->Update(Lite::kPhysicsStep);
		if (!scene->IsPlaying() || !scene->Find("Keep"))
			return Fail("Scene check: a missing scene replaced the one that was playing");

		scene->Stop();
		Lite::Scene::Close(scene);
		std::filesystem::remove_all(directory, error);
		return true;
	}

	bool Logged(std::string_view text)
	{
		for (const std::string& line : Lite::Console::GetLines())
		{
			if (line.find(text) != std::string::npos)
				return true;
		}
		return false;
	}

	bool CheckScripts()
	{
		const std::filesystem::path source = Lite::FileSystem::Locate("Sandbox/scripts/SandboxScripts.dll");
		if (source.empty())
			return Fail("Scene check: SandboxScripts.dll was not found");

		const std::filesystem::path directory = std::filesystem::temp_directory_path() / "lite-script-check";
		std::error_code error;
		std::filesystem::remove_all(directory, error);
		std::filesystem::create_directories(directory, error);
		if (error)
			return Fail("Scene check: could not create a script directory");

		const std::filesystem::path module = directory / "Module.dll";
		std::filesystem::copy_file(source, module, std::filesystem::copy_options::overwrite_existing, error);
		if (error)
			return Fail("Scene check: could not copy the script module");

		Lite::Ref<Lite::Project> project = Lite::Project::New();
		project->GetConfig().ScriptModulePath = module;

		Lite::Scene scene("Scripts");
		scene.Create();
		scene.CreateEntity("Box").Add<Lite::ScriptComponent>().Class = "Bounce";

		auto played = [&](const char* loaded)
		{
			Lite::Console::Clear();
			scene.Play();
			const bool started = Logged("Box started");
			const bool moduleLoaded = Logged("Script module loaded");
			scene.Stop();
			if (!started)
				return Fail("Scene check: the script start did not run");
			if (loaded != nullptr && !moduleLoaded)
				return Fail(loaded);
			return true;
		};

		if (!played("Scene check: play did not load the script module"))
			return false;

		std::filesystem::copy_file(source, module, std::filesystem::copy_options::overwrite_existing, error);
		if (error)
			return Fail("Scene check: could not replace the script module");
		if (!played("Scene check: replacing the script module did not load it"))
			return false;

		HANDLE lock = CreateFileW(module.c_str(), GENERIC_READ, 0, nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
		if (lock == INVALID_HANDLE_VALUE)
			return Fail("Scene check: could not lock the script module");

		Lite::Console::Clear();
		scene.Play();
		const bool locked = Logged("is locked");
		const bool started = Logged("Box started");
		const bool replaced = Logged("Script module loaded");
		scene.Stop();
		CloseHandle(lock);
		Lite::ScriptRuntime::Unload();
		std::filesystem::remove_all(directory, error);

		if (!locked)
			return Fail("Scene check: a locked script module was not reported");
		if (!started || replaced)
			return Fail("Scene check: a locked script module changed the loaded scripts");
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
		if (Check(*scene) && CheckQueries() && CheckLayersAndJoint() && CheckSceneSwap() && CheckScripts())
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
