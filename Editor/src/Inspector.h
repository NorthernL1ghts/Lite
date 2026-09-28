#pragma once

#include <cstdint>

class Inspector
{
public:
	void Reset();
	void Draw(std::uint32_t selected);

private:
	char m_ObjectName[128] {};
	char m_ShaderText[128] {};
	char m_TextureText[260] {};
	std::uint32_t m_SyncedId = 0;
};
