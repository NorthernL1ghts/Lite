#pragma once

#include "Lite/Core/Base.h"

#include <string>
#include <string_view>

namespace Lite {

	enum class EventType
	{
		None = 0,
		WindowClose, WindowResize,
		KeyPressed, KeyReleased, KeyTyped,
		MouseButtonPressed, MouseButtonReleased, MouseMoved, MouseScrolled
	};

	enum class EventCategory
	{
		None = 0,
		Application = LITE_BIT(0),
		Input = LITE_BIT(1),
		Keyboard = LITE_BIT(2),
		Mouse = LITE_BIT(3),
		MouseButton = LITE_BIT(4)
	};

	constexpr EventCategory operator|(EventCategory left, EventCategory right)
	{
		return static_cast<EventCategory>(static_cast<int>(left) | static_cast<int>(right));
	}

	constexpr int operator&(EventCategory left, EventCategory right)
	{
		return static_cast<int>(left) & static_cast<int>(right);
	}

	class Event
	{
	public:
		virtual ~Event() = default;

		virtual EventType GetType() const = 0;
		virtual std::string_view GetName() const = 0;
		virtual EventCategory GetCategoryFlags() const = 0;
		virtual std::string ToString() const { return std::string(GetName()); }

		bool IsInCategory(EventCategory category) const
		{
			return (GetCategoryFlags() & category) != 0;
		}

		bool Handled = false;
	};

	class EventDispatcher
	{
	public:
		explicit EventDispatcher(Event& event)
			: m_Event(event)
		{
		}

		template<typename T, typename Func>
		bool Dispatch(Func&& func)
		{
			if (m_Event.GetType() != T::GetStaticType())
				return false;

			m_Event.Handled = func(static_cast<T&>(m_Event));
			return true;
		}

	private:
		Event& m_Event;
	};

}

#define LITE_EVENT_CLASS_TYPE(type) \
	static EventType GetStaticType() { return EventType::type; } \
	EventType GetType() const override { return GetStaticType(); } \
	std::string_view GetName() const override { return #type; }

#define LITE_EVENT_CLASS_CATEGORY(category) \
	EventCategory GetCategoryFlags() const override { return category; }
