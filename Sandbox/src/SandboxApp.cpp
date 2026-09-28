#include <Lite.h>

#include <imgui.h>

namespace {

	class SandboxLayer final : public Lite::Layer
	{
	public:
		SandboxLayer()
			: Lite::Layer("Sandbox")
		{
		}

		void OnAttach() override
		{
			LITE_CLIENT_INFO("Layer attached");
		}

		void OnDetach() override
		{
			LITE_CLIENT_INFO("Layer detached");
		}

		void OnEvent(Lite::Event& event) override
		{
			if (event.GetType() == Lite::EventType::KeyPressed)
				LITE_CLIENT_TRACE("{}", event.ToString());
		}

		void OnImGuiRender() override
		{
			ImGui::ShowDemoWindow();
		}
	};

	class Sandbox final : public Lite::Application
	{
	public:
		Sandbox()
			: Lite::Application(Lite::WindowProps("Sandbox"))
		{
			PushLayer(std::make_unique<SandboxLayer>());
			LITE_CLIENT_INFO("Created");
		}

		~Sandbox() override
		{
			LITE_CLIENT_INFO("Destroyed");
		}
	};

}

std::unique_ptr<Lite::Application> Lite::CreateApplication()
{
    return std::make_unique<Sandbox>();
}
