#pragma once

#include "Layer.h"

#include <memory>
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

		void PushLayer(std::unique_ptr<Layer> layer);
		void PushOverlay(std::unique_ptr<Layer> layer);
		void PopLayer(Layer* layer);
		void PopOverlay(Layer* layer);
		void Clear();

		auto begin() { return m_Layers.begin(); }
		auto end() { return m_Layers.end(); }
		auto rbegin() { return m_Layers.rbegin(); }
		auto rend() { return m_Layers.rend(); }

	private:
		std::vector<std::unique_ptr<Layer>> m_Layers;
		unsigned int m_LayerInsertIndex = 0;
	};

}
