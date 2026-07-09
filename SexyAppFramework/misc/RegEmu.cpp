// simple Windows Registry emulator

#include "RegEmu.h"

#include <map>
#include <cstdio>
#include <cstring>

#define REGEMU_VERSION 1

struct RegValue
{
	uint32_t mType;
	uint32_t mLength;
	uint8_t* mValue;
};
typedef std::map<std::string, std::map<std::string, RegValue> > RegContents;
static RegContents registry;
static std::string currFile;

static void SaveToFile()
{
	if (currFile.empty())
	{
		printf("RegEmu: Filename not specified, can't save\n");
		return;
	}

	FILE* f = fopen(currFile.c_str(), "wb");
	if (!f)
	{
		printf("RegEmu: Couldn't open '%s' for writing\n", currFile.c_str());
		return;
	}

	fwrite("REGEMU", 6, 1, f);

	uint16_t aVersion = REGEMU_VERSION;
	fwrite(&aVersion, sizeof(uint16_t), 1, f);

	uint32_t aNumKeys = registry.size();
	fwrite(&aNumKeys, sizeof(uint32_t), 1, f);

	for (auto& keyPair : registry)
	{
		uint32_t aKeyNameLen = keyPair.first.size()+1;
		fwrite(&aKeyNameLen, sizeof(uint32_t), 1, f);
		fwrite(keyPair.first.c_str(), aKeyNameLen, 1, f);

		uint32_t aNumValues = keyPair.second.size();
		fwrite(&aNumValues, sizeof(uint32_t), 1, f);

		for (auto& valuePair : registry[keyPair.first])
		{
			uint32_t aValueNameLen = valuePair.first.size()+1;
			fwrite(&aValueNameLen, sizeof(uint32_t), 1, f);
			fwrite(valuePair.first.c_str(), aValueNameLen, 1, f);

			RegValue& value = valuePair.second;
			fwrite(&value.mType, sizeof(uint32_t), 1, f);
			fwrite(&value.mLength, sizeof(uint32_t), 1, f);
			fwrite(value.mValue, value.mLength, 1, f);
		}
	}

	fclose(f);
}

void regemu::SetRegFile(const std::string& fileName)
{
	currFile = fileName;
	registry.clear();

	FILE* f = fopen(currFile.c_str(), "rb");
	if (!f)
	{
		printf("RegEmu: Can't read '%s': File does not exist\n", currFile.c_str());
		return;
	}

	char aHeader[6];
	fread(aHeader, 6, 1, f);
	if (strncmp(aHeader, "REGEMU", 6))
	{
		printf("RegEmu: Can't read '%s': Invalid header\n", currFile.c_str());
		fclose(f);
		return;
	}

	uint16_t aVersion;
	fread(&aVersion, sizeof(uint16_t), 1, f);

	uint32_t aNumKeys;
	fread(&aNumKeys, sizeof(uint32_t), 1, f);

	for (uint32_t i=0; i<aNumKeys; i++)
	{
		uint32_t aKeyNameLen;
		char* aKeyName;

		fread(&aKeyNameLen, sizeof(uint32_t), 1, f);
		aKeyName = new char[aKeyNameLen];
		fread(aKeyName, aKeyNameLen, 1, f);

		registry[aKeyName] = {};

		uint32_t aNumValues;
		fread(&aNumValues, sizeof(uint32_t), 1, f);

		for (uint32_t j=0; j<aNumValues; j++)
		{
			RegValue value;
			uint32_t aValueNameLen;
			char* aValueName;

			fread(&aValueNameLen, sizeof(uint32_t), 1, f);
			aValueName = new char[aValueNameLen];
			fread(aValueName, aValueNameLen, 1, f);

			fread(&value.mType, sizeof(uint32_t), 1, f);
			fread(&value.mLength, sizeof(uint32_t), 1, f);
			value.mValue = new uint8_t[value.mLength];
			fread(value.mValue, value.mLength, 1, f);

			registry[aKeyName][aValueName] = value;

			delete[] aValueName;
		}

		delete[] aKeyName;
	}

	fclose(f);
	printf("RegEmu: Loaded from '%s': %ld total key(s)\n", currFile.c_str(), registry.size());
}

bool regemu::RegistryRead(const std::string& keyName, const std::string& valueName, uint32_t* type, uint8_t* value, uint32_t* length)
{
	if (!registry.count(keyName))
	{
		printf("RegEmu: Key '%s' does not exist\n", keyName.c_str());
		return false;
	}
	if (!registry[keyName].count(valueName))
	{
		printf("RegEmu: Value '%s' does not exist\n", valueName.c_str());
		return false;
	}

	*type = registry[keyName][valueName].mType;
	*length = registry[keyName][valueName].mLength;
	memcpy(value, registry[keyName][valueName].mValue, registry[keyName][valueName].mLength);
	return true;
}

bool regemu::RegistryWrite(const std::string& keyName, const std::string& valueName, uint32_t type, const uint8_t* value, uint32_t length)
{
	if (!registry.count(keyName))
		registry[keyName] = {}; // create
	else
		delete[] registry[keyName][valueName].mValue;

	RegValue regvalue;
	regvalue.mType = type;
	regvalue.mLength = length;
	regvalue.mValue = new uint8_t[length];
	memcpy(regvalue.mValue, value, length);

	registry[keyName][valueName] = regvalue;

	SaveToFile();

	return true;
}

bool regemu::RegistryEraseKey(const std::string& keyName)
{
	if (!registry.count(keyName))
		return false;

	for (auto& valuePair : registry[keyName])
		delete[] valuePair.second.mValue;

	registry.erase(keyName);
	printf("RegEmu: Erased key '%s'\n", keyName.c_str());

	SaveToFile();

	return true;
}

bool regemu::RegistryEraseValue(const std::string& keyName, const std::string& valueName)
{
	if (!registry.count(keyName) || !registry[keyName].count(valueName))
		return false;

	delete[] registry[keyName][valueName].mValue;
	registry[keyName].erase(valueName);
	printf("RegEmu: Erased value '%s' from key '%s'\n", valueName.c_str(), keyName.c_str());

	SaveToFile();

	return true;
}
