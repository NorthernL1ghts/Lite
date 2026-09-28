#pragma once

#include <Lite/Core/Window.h>

struct GLFWwindow;

namespace Lite {

	class GLFWWindow : public Window
	{
	public:
		explicit GLFWWindow(const WindowProps& props);
		~GLFWWindow() override;

		void PollEvents() override;
		void Clear() override;
		void SwapBuffers() override;
		bool IsOpen() const override;

		void SetEventCallback(EventCallback callback) override;
		const WindowProps& GetProps() const override { return m_Data.Props; }
		void* GetNativeHandle() const override { return m_Window; }

	private:
		struct WindowData
		{
			WindowProps Props;
			EventCallback Callback;
		};

		void SetCallbacks();

		WindowData m_Data;
		GLFWwindow* m_Window = nullptr;
		bool m_GlfwInitialized = false;
	};

}
