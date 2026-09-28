#include <Lite/Core/UUID.h>

#include <format>
#include <rpc.h>

#pragma comment(lib, "Rpcrt4.lib")

namespace Lite {

	UUID::UUID()
	{
		::GUID guid {};
		RPC_STATUS status = ::UuidCreate(&guid);
		if (status != RPC_S_OK && status != RPC_S_UUID_LOCAL_ONLY)
			return;

		m_Bytes[0] = static_cast<uint8_t>((guid.Data1 >> 24) & 0xff);
		m_Bytes[1] = static_cast<uint8_t>((guid.Data1 >> 16) & 0xff);
		m_Bytes[2] = static_cast<uint8_t>((guid.Data1 >> 8) & 0xff);
		m_Bytes[3] = static_cast<uint8_t>(guid.Data1 & 0xff);
		m_Bytes[4] = static_cast<uint8_t>((guid.Data2 >> 8) & 0xff);
		m_Bytes[5] = static_cast<uint8_t>(guid.Data2 & 0xff);
		m_Bytes[6] = static_cast<uint8_t>((guid.Data3 >> 8) & 0xff);
		m_Bytes[7] = static_cast<uint8_t>(guid.Data3 & 0xff);
		for (int index = 0; index < 8; ++index)
			m_Bytes[static_cast<size_t>(8 + index)] = guid.Data4[index];
	}

	UUID::UUID(std::array<uint8_t, 16> bytes)
		: m_Bytes(bytes)
	{
	}

	UUID UUID::Null()
	{
		return UUID(std::array<uint8_t, 16> {});
	}

	bool UUID::IsNull() const
	{
		for (uint8_t byte : m_Bytes)
		{
			if (byte != 0)
				return false;
		}

		return true;
	}

	std::string UUID::ToString() const
	{
		return std::format(
			"{:02x}{:02x}{:02x}{:02x}-{:02x}{:02x}-{:02x}{:02x}-{:02x}{:02x}-{:02x}{:02x}{:02x}{:02x}{:02x}{:02x}",
			m_Bytes[0], m_Bytes[1], m_Bytes[2], m_Bytes[3],
			m_Bytes[4], m_Bytes[5],
			m_Bytes[6], m_Bytes[7],
			m_Bytes[8], m_Bytes[9],
			m_Bytes[10], m_Bytes[11], m_Bytes[12], m_Bytes[13], m_Bytes[14], m_Bytes[15]);
	}

}
