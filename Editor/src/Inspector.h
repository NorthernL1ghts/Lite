#pragma once

#include <cstdint>
#include <string>

class Inspector
{
public:
	void Reset();
	void Draw(std::uint32_t selected);
	void ApplyTexture(std::uint32_t selected, const std::string& path);

private:
	char m_ObjectName[128] {};
	char m_ShaderText[128] {};
	char m_TextureText[1024] {};
	char m_ScriptText[128] {};
	std::uint32_t m_SyncedId = 0;
};
