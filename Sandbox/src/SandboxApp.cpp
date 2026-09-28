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
			auto [mouseX, mouseY] = Lite::Input::GetMousePosition();
			bool leftDown = Lite::Input::IsMouseButtonPressed(Lite::Mouse::ButtonLeft);
			bool spaceDown = Lite::Input::IsKeyPressed(Lite::Key::Space);

			ImGui::Begin("Input");
			ImGui::Text("Mouse: %.0f, %.0f", mouseX, mouseY);
			ImGui::Text("Left button: %s", leftDown ? "down" : "up");
			ImGui::Text("Space: %s", spaceDown ? "down" : "up");
			ImGui::End();

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
