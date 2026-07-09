// simple Windows Registry emulator

#ifndef __REGEMU_H__
#define __REGEMU_H__

#include <string>
#include <cstdint>

namespace regemu
{
	enum
	{
		REGEMU_NONE,
		REGEMU_SZ,
		REGEMU_EXPAND_SZ,
		REGEMU_BINARY,
		REGEMU_DWORD,
		REGEMU_DWORD_LITTLE_ENDIAN=4,
		REGEMU_DWORD_BIG_ENDIAN,
		REGEMU_MULTI_SZ=7,
		REGEMU_QWORD=11,
		REGEMU_QWORD_LITTLE_ENDIAN=11
	};

	void SetRegFile(const std::string& fileName);
	bool RegistryRead(const std::string& keyName, const std::string& valueName, uint32_t* type, uint8_t* value, uint32_t* length);
	bool RegistryWrite(const std::string& keyName, const std::string& valueName, uint32_t type, const uint8_t* value, uint32_t length);
	bool RegistryEraseKey(const std::string& keyName);
	bool RegistryEraseValue(const std::string& keyName, const std::string& valueName);
}

#endif
