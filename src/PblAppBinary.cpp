#include "pbw_api_info.h"

PblAppBinary::PblAppBinary(void* b, uint32_t s, PblLibrary* lib) :
	library(lib), buffer(b), size(s) {
}

PblAppBinary::~PblAppBinary() {
	if (buffer)
		free(buffer);
}

uint32_t PblAppBinary::scan() {
	uint8_t* code = reinterpret_cast<uint8_t*>(buffer);
	uint8_t* end = code + size;
	code += sizeof(PblAppHeader);

	// TODO: This is a really slow memory search
	for (; code != end; code++) {
		for (uint32_t i = 0; i < library->getFunctionCount(); i++) {
			const uint8_t* funcCode = reinterpret_cast<const uint8_t*>(library->getFunctionCode(i));
			uint32_t funcCodeSize = library->getFunctionCodeSize(i);
			if (code + funcCodeSize > end)
				continue;

			// compare code
			uint32_t relocOff = library->getFunctionRelocatedOffset(i);
			if (relocOff == UINT32_MAX) {
				// without relocation entry, mostly wrong
				if (memcmp(code, funcCode, funcCodeSize) == 0)
					usedFunctions.push_back(i);
			}
			else if (memcmp(code, funcCode, relocOff) == 0) {
				// check code after relocation entry (which is 4 bytes long)
				uint8_t* codeAfter = code + relocOff + 4;
				const uint8_t* funcCodeAfter = funcCode + relocOff + 4;
				uint32_t sizeAfter = funcCodeSize - relocOff - 4;
				if (memcmp(codeAfter, funcCodeAfter, sizeAfter) == 0)
					usedFunctions.push_back(i);
			}
		}
	}

	return usedFunctions.size();
}

const char* PblAppBinary::getPlatformName() const {
	return library->getPlatformName();
}

const PblAppHeader* PblAppBinary::getHeader() const {
	return (const PblAppHeader*)buffer;
}

uint32_t PblAppBinary::getUsedFunctionCount() const {
	return usedFunctions.size();
}

uint32_t PblAppBinary::getUsedFunctionIndex(uint32_t index) const {
	if (index >= usedFunctions.size())
		return UINT32_MAX;
	else
		return usedFunctions[index];
}

const char* PblAppBinary::getUsedFunctionName(uint32_t index) const {
	if (index >= usedFunctions.size())
		return nullptr;
	else
		return library->getFunctionName(usedFunctions[index]);
}
