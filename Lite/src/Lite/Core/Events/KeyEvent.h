#pragma once

#include <Lite/Core/Events/Event.h>

#include <format>
#include <string>

namespace Lite {

	class KeyEvent : public Event
	{
	public:
		int GetKeyCode() const { return m_KeyCode; }

		LITE_EVENT_CLASS_CATEGORY(EventCategory::Keyboard | EventCategory::Input)

	protected:
		explicit KeyEvent(int keyCode)
			: m_KeyCode(keyCode)
		{
		}

		int m_KeyCode;
	};

	class KeyPressedEvent : public KeyEvent
	{
	public:
		KeyPressedEvent(int keyCode, bool repeat = false)
			: KeyEvent(keyCode)
			, m_Repeat(repeat)
		{
		}

		bool IsRepeat() const { return m_Repeat; }

		std::string ToString() const override
		{
			return std::format("KeyPressed: {} (repeat = {})", m_KeyCode, m_Repeat);
		}

		LITE_EVENT_CLASS_TYPE(KeyPressed)

	private:
		bool m_Repeat;
	};

	class KeyReleasedEvent : public KeyEvent
	{
	public:
		explicit KeyReleasedEvent(int keyCode)
			: KeyEvent(keyCode)
		{
		}

		std::string ToString() const override
		{
			return std::format("KeyReleased: {}", m_KeyCode);
		}

		LITE_EVENT_CLASS_TYPE(KeyReleased)
	};

	class KeyTypedEvent : public KeyEvent
	{
	public:
		explicit KeyTypedEvent(int keyCode)
			: KeyEvent(keyCode)
		{
		}

		std::string ToString() const override
		{
			return std::format("KeyTyped: {}", m_KeyCode);
		}

		LITE_EVENT_CLASS_TYPE(KeyTyped)
	};

}
