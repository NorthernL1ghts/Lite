#include <Lite/Script/ScriptModule.h>

#include <Lite/Project/Project.h>
#include <Lite/Scene/Console.h>
#include <Lite/Scene/Components/ScriptComponent.h>

#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <Windows.h>

#include <format>
#include <string>
#include <vector>

namespace Lite {

	namespace {

		struct LoadedScript
		{
			std::string Name;
			void (*Start)(Entity entity) = nullptr;
			void (*Update)(Entity entity, float seconds) = nullptr;
			void (*OnCollision)(Entity entity, uint32_t other) = nullptr;
		};

		HMODULE s_Module = nullptr;
		std::vector<LoadedScript> s_Classes;

		const LoadedScript* FindScript(std::string_view name)
		{
			for (const LoadedScript& script : s_Classes)
			{
				if (script.Name == name)
					return &script;
			}

			return nullptr;
		}

		void AddScript(const ScriptClass* script)
		{
			if (script == nullptr || script->Name == nullptr || script->Name[0] == '\0')
				return;

			if (FindScript(script->Name) != nullptr)
				return;

			LoadedScript loaded;
			loaded.Name = script->Name;
			loaded.Start = script->Start;
			loaded.Update = script->Update;
			loaded.OnCollision = script->OnCollision;
			s_Classes.push_back(std::move(loaded));
		}

		void StartScripts(Scene& scene)
		{
			for (Entity entity : scene.GetEntities())
			{
				ScriptComponent* script = entity.Get<ScriptComponent>();
				if (script == nullptr || script->Class.empty())
					continue;

				const LoadedScript* type = FindScript(script->Class);
				if (type == nullptr)
				{
					Console::Log(std::format("Script class not found: {}", script->Class));
					continue;
				}

				if (type->Start != nullptr)
					type->Start(entity);
			}
		}

		void CallCollision(Scene& scene, uint32_t self, uint32_t other)
		{
			if (self == 0)
				return;

			Entity entity = scene.GetEntity(self);
			ScriptComponent* script = entity.Get<ScriptComponent>();
			if (script == nullptr || script->Class.empty())
				return;

			const LoadedScript* type = FindScript(script->Class);
			if (type != nullptr && type->OnCollision != nullptr)
				type->OnCollision(entity, other);
		}

	}

	void ScriptRuntime::Load(Scene& scene)
	{
		Unload();

		Ref<Project> project = Project::GetActive();
		if (!project)
			return;

		const std::filesystem::path& configured = project->GetConfig().ScriptModulePath;
		if (configured.empty())
		{
			for (Entity entity : scene.GetEntities())
			{
				ScriptComponent* script = entity.Get<ScriptComponent>();
				if (script != nullptr && !script->Class.empty())
				{
					Console::Log("Failed to load script module: no module is set");
					break;
				}
			}
			return;
		}

		std::filesystem::path file = configured;
		if (!file.is_absolute())
		{
			if (project->GetProjectDirectory().empty())
			{
				Console::Log(std::format("Failed to load script module: {}", configured.generic_string()));
				return;
			}

			file = project->GetProjectDirectory() / configured;
		}

		std::error_code error;
		if (!std::filesystem::is_regular_file(file, error))
		{
			Console::Log(std::format("Failed to load script module: {}", file.string()));
			return;
		}

		HMODULE module = LoadLibraryW(file.c_str());
		if (module == nullptr)
		{
			Console::Log(std::format("Failed to load script module: {} (error {})", file.string(), GetLastError()));
			return;
		}

		auto registerScripts = reinterpret_cast<void (*)(ScriptAddFn)>(GetProcAddress(module, ScriptRegisterExport));
		if (registerScripts == nullptr)
		{
			Console::Log(std::format("Failed to load script module: {} has no LiteRegisterScripts", file.filename().string()));
			FreeLibrary(module);
			return;
		}

		registerScripts(&AddScript);
		s_Module = module;
		Console::Log(std::format("Script module loaded: {}", configured.generic_string()));
		StartScripts(scene);
	}

	void ScriptRuntime::Unload()
	{
		s_Classes.clear();
		if (s_Module == nullptr)
			return;

		FreeLibrary(s_Module);
		s_Module = nullptr;
	}

	void ScriptRuntime::Update(Scene& scene, float seconds)
	{
		if (s_Module == nullptr)
			return;

		for (Entity entity : scene.GetEntities())
		{
			ScriptComponent* script = entity.Get<ScriptComponent>();
			if (script == nullptr || script->Class.empty())
				continue;

			const LoadedScript* type = FindScript(script->Class);
			if (type != nullptr && type->Update != nullptr)
				type->Update(entity, seconds);
		}
	}

	void ScriptRuntime::OnCollision(Scene& scene, uint32_t first, uint32_t second)
	{
		if (s_Module == nullptr)
			return;

		CallCollision(scene, first, second);
		CallCollision(scene, second, first);
	}

}
