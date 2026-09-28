#pragma once

#include "Event.h"

#include <format>
#include <string>

namespace Lite {

	class WindowCloseEvent : public Event
	{
	public:
		LITE_EVENT_CLASS_TYPE(WindowClose)
		LITE_EVENT_CLASS_CATEGORY(EventCategory::Application)
	};

	class WindowResizeEvent : public Event
	{
	public:
		WindowResizeEvent(int width, int height)
			: m_Width(width)
			, m_Height(height)
		{
		}

		int GetWidth() const { return m_Width; }
		int GetHeight() const { return m_Height; }

		std::string ToString() const override
		{
			return std::format("WindowResize: {}x{}", m_Width, m_Height);
		}

		LITE_EVENT_CLASS_TYPE(WindowResize)
		LITE_EVENT_CLASS_CATEGORY(EventCategory::Application)

	private:
		int m_Width;
		int m_Height;
	};

}
