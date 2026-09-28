#pragma once

#include "Events/Event.h"

#include <functional>
#include <memory>
#include <string>
#include <string_view>

namespace Lite {

	struct WindowProps
	{
		std::string Title;
		int Width;
		int Height;

		WindowProps(std::string_view title = "Lite", int width = 1280, int height = 720)
			: Title(title)
			, Width(width)
			, Height(height)
		{
		}
	};

	class Window
	{
	public:
		using EventCallback = std::function<void(Event&)>;

		virtual ~Window() = default;

		virtual void Update() = 0;
		virtual bool IsOpen() const = 0;

		virtual void SetEventCallback(EventCallback callback) = 0;
		virtual const WindowProps& GetProps() const = 0;

		static std::unique_ptr<Window> Create(const WindowProps& props);
	};

}
