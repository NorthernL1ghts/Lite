#pragma once

#include "Lite/Core/Layer.h"

struct GLFWwindow;

namespace Lite {

	class LITE_API ImGuiLayer : public Layer
	{
	public:
		explicit ImGuiLayer(GLFWwindow* window);
		~ImGuiLayer() override;

		void OnAttach() override;
		void OnDetach() override;
		void OnEvent(Event& event) override;

		void Begin();
		void End();

	private:
		GLFWwindow* m_Window = nullptr;
	};

}
