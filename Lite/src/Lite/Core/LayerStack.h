#pragma once

#include <Lite/Core/Layer.h>

#include <vector>

namespace Lite {

	class LITE_API LayerStack
	{
	public:
		LayerStack() = default;
		~LayerStack();

		LayerStack(const LayerStack&) = delete;
		LayerStack& operator=(const LayerStack&) = delete;
		LayerStack(LayerStack&&) noexcept = default;
		LayerStack& operator=(LayerStack&&) noexcept = default;

		void PushLayer(Scope<Layer> layer);
		void PushOverlay(Scope<Layer> layer);
		void PopLayer(Layer* layer);
		void PopOverlay(Layer* layer);
		void Clear();

		template<typename Function>
		void ForEach(Function&& function)
		{
			for (Scope<Layer>& layer : m_Layers)
				function(*layer);
		}

		template<typename Function>
		void ForEachReverse(Function&& function)
		{
			for (auto it = m_Layers.rbegin(); it != m_Layers.rend(); ++it)
			{
				if (!function(**it))
					break;
			}
		}

	private:
		std::vector<Scope<Layer>> m_Layers;
		unsigned int m_LayerInsertIndex = 0;
	};

}
