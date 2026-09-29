#pragma once

// Public engine header. Include from exactly one client translation unit
// so the entry point is defined once.
#include <Lite/Core/Core.h>
#include <Lite/Core/Layer.h>
#include <Lite/Core/LayerStack.h>
#include <Lite/Input/Input.h>
#include <Lite/Math/Math.h>
#include <Lite/Assets/Asset.h>
#include <Lite/Assets/AssetRegistry.h>
#include <Lite/Assets/Shader.h>
#include <Lite/Assets/Texture.h>
#include <Lite/Renderer/OrthographicCamera.h>
#include <Lite/Renderer/Resources/Buffer.h>
#include <Lite/Renderer/Resources/VertexArray.h>
#include <Lite/Renderer/Resources/Framebuffer.h>
#include <Lite/Renderer/Resources/UniformBuffer.h>
#include <Lite/Renderer/Shader/ShaderLibrary.h>
#include <Lite/Renderer/Shader/ShaderProgram.h>
#include <Lite/Renderer/Material.h>
#include <Lite/Renderer/RendererAPI.h>
#include <Lite/Renderer/Renderer.h>
#include <Lite/Renderer/Renderer2D.h>
#include <Lite/Project/Project.h>
#include <Lite/Scene/Console.h>
#include <Lite/Scene/Scene.h>
#include <Lite/Core/Events/Event.h>
#include <Lite/Core/Events/KeyEvent.h>
#include <Lite/Core/Events/MouseEvent.h>
#include <Lite/Core/Events/WindowEvent.h>
#include <Lite/Core/Application.h>
#include <EntryPoint.h>
