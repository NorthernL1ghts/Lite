#include <Lite/Script/ScriptModule.h>

#include <Lite/Project/Project.h>
#include <Lite/Scene/Console.h>
#include <Lite/Scene/Components/ScriptComponent.h>

#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <Windows.h>

#include <cstdint>
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
			void (*OnCollision)(Entity entity, uint32_t other, bool begin) = nullptr;
		};

		HMODULE s_Module = nullptr;
		std::filesystem::path s_Copy;
		std::filesystem::path s_LoadedSource;
		std::vector<LoadedScript> s_Classes;
		std::vector<LoadedScript>* s_Registering = nullptr;

		std::vector<LoadedScript>& Registering()
		{
			return s_Registering != nullptr ? *s_Registering : s_Classes;
		}

		const LoadedScript* FindScript(std::string_view name)
		{
			for (const LoadedScript& script : Registering())
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
			Registering().push_back(std::move(loaded));
		}

		bool IsLocked(const std::error_code& error)
		{
			return error == std::errc::permission_denied
				|| error.value() == ERROR_SHARING_VIOLATION
				|| error.value() == ERROR_LOCK_VIOLATION;
		}

		void DeleteCopy(std::filesystem::path path)
		{
			if (path.empty())
				return;

			std::error_code error;
			std::filesystem::remove(path, error);
		}

		bool ReadModule(const std::filesystem::path& file, HMODULE& module, std::filesystem::path& copy, std::vector<LoadedScript>& classes)
		{
			std::error_code error;
			const std::filesystem::path directory = std::filesystem::temp_directory_path(error);
			if (error)
			{
				Console::Log(std::format("Failed to load script module: {}", file.string()));
				return false;
			}

			static uint64_t copyIndex = 0;
			const std::filesystem::path extension = file.extension().empty() ? std::filesystem::path(".dll") : file.extension();
			const std::filesystem::path dest = directory / std::format("lite-script-{}-{}{}", GetTickCount64(), ++copyIndex, extension.string());
			std::filesystem::copy_file(file, dest, error);
			if (error)
			{
				if (IsLocked(error))
					Console::Log(std::format("Failed to load script module: {} is locked", file.string()));
				else
					Console::Log(std::format("Failed to load script module: {}", file.string()));
				DeleteCopy(dest);
				return false;
			}

			HMODULE loaded = LoadLibraryW(dest.c_str());
			if (loaded == nullptr)
			{
				Console::Log(std::format("Failed to load script module: {} (error {})", file.string(), GetLastError()));
				DeleteCopy(dest);
				return false;
			}

			auto registerScripts = reinterpret_cast<void (*)(ScriptAddFn)>(GetProcAddress(loaded, ScriptRegisterExport));
			if (registerScripts == nullptr)
			{
				Console::Log(std::format("Failed to load script module: {} has no LiteRegisterScripts", file.filename().string()));
				FreeLibrary(loaded);
				DeleteCopy(dest);
				return false;
			}

			s_Registering = &classes;
			registerScripts(&AddScript);
			s_Registering = nullptr;
			module = loaded;
			copy = dest;
			return true;
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

		void CallCollision(Scene& scene, uint32_t self, uint32_t other, bool begin)
		{
			if (self == 0)
				return;

			Entity entity = scene.GetEntity(self);
			ScriptComponent* script = entity.Get<ScriptComponent>();
			if (script == nullptr || script->Class.empty())
				return;

			const LoadedScript* type = FindScript(script->Class);
			if (type != nullptr && type->OnCollision != nullptr)
				type->OnCollision(entity, other, begin);
		}

	}

	void ScriptRuntime::Load(Scene& scene)
	{
		Ref<Project> project = Project::GetActive();
		if (!project)
		{
			Unload();
			return;
		}

		const std::filesystem::path& configured = project->GetConfig().ScriptModulePath;
		if (configured.empty())
		{
			Unload();
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
				Unload();
				return;
			}

			file = project->GetProjectDirectory() / configured;
		}

		std::error_code error;
		if (!std::filesystem::is_regular_file(file, error))
		{
			Console::Log(std::format("Failed to load script module: {}", file.string()));
			Unload();
			return;
		}

		HMODULE module = nullptr;
		std::filesystem::path copy;
		std::vector<LoadedScript> classes;
		if (!ReadModule(file, module, copy, classes))
		{
			if (s_Module != nullptr && s_LoadedSource == file)
				StartScripts(scene);
			else
				Unload();
			return;
		}

		Unload();
		s_Module = module;
		s_Copy = std::move(copy);
		s_Classes = std::move(classes);
		s_LoadedSource = file;
		Console::Log(std::format("Script module loaded: {}", configured.generic_string()));
		StartScripts(scene);
	}

	void ScriptRuntime::Unload()
	{
		s_Classes.clear();
		s_LoadedSource.clear();
		if (s_Module != nullptr)
		{
			FreeLibrary(s_Module);
			s_Module = nullptr;
		}

		DeleteCopy(std::move(s_Copy));
		s_Copy.clear();
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

	void ScriptRuntime::OnCollision(Scene& scene, uint32_t first, uint32_t second, bool begin)
	{
		if (s_Module == nullptr)
			return;

		CallCollision(scene, first, second, begin);
		CallCollision(scene, second, first, begin);
	}

}
