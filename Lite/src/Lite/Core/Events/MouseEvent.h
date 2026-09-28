#pragma once

#include <Lite/Core/Events/Event.h>

#include <format>
#include <string>

namespace Lite {

	class MouseMovedEvent : public Event
	{
	public:
		MouseMovedEvent(float x, float y)
			: m_X(x)
			, m_Y(y)
		{
		}

		float GetX() const { return m_X; }
		float GetY() const { return m_Y; }

		std::string ToString() const override
		{
			return std::format("MouseMoved: {}, {}", m_X, m_Y);
		}

		LITE_EVENT_CLASS_TYPE(MouseMoved)
		LITE_EVENT_CLASS_CATEGORY(EventCategory::Mouse | EventCategory::Input)

	private:
		float m_X;
		float m_Y;
	};

	class MouseScrolledEvent : public Event
	{
	public:
		MouseScrolledEvent(float xOffset, float yOffset)
			: m_XOffset(xOffset)
			, m_YOffset(yOffset)
		{
		}

		float GetXOffset() const { return m_XOffset; }
		float GetYOffset() const { return m_YOffset; }

		std::string ToString() const override
		{
			return std::format("MouseScrolled: {}, {}", m_XOffset, m_YOffset);
		}

		LITE_EVENT_CLASS_TYPE(MouseScrolled)
		LITE_EVENT_CLASS_CATEGORY(EventCategory::Mouse | EventCategory::Input)

	private:
		float m_XOffset;
		float m_YOffset;
	};

	class MouseButtonEvent : public Event
	{
	public:
		int GetButton() const { return m_Button; }

		LITE_EVENT_CLASS_CATEGORY(EventCategory::Mouse | EventCategory::MouseButton | EventCategory::Input)

	protected:
		explicit MouseButtonEvent(int button)
			: m_Button(button)
		{
		}

		int m_Button;
	};

	class MouseButtonPressedEvent : public MouseButtonEvent
	{
	public:
		explicit MouseButtonPressedEvent(int button)
			: MouseButtonEvent(button)
		{
		}

		std::string ToString() const override
		{
			return std::format("MouseButtonPressed: {}", m_Button);
		}

		LITE_EVENT_CLASS_TYPE(MouseButtonPressed)
	};

	class MouseButtonReleasedEvent : public MouseButtonEvent
	{
	public:
		explicit MouseButtonReleasedEvent(int button)
			: MouseButtonEvent(button)
		{
		}

		std::string ToString() const override
		{
			return std::format("MouseButtonReleased: {}", m_Button);
		}

		LITE_EVENT_CLASS_TYPE(MouseButtonReleased)
	};

}
