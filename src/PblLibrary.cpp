#include "pbw_api_info.h"

#include <iostream>

static bool is_little_endian() {
	union {
		uint8_t bytes[4];
		uint32_t dword;
	} u;
	u.dword = 1;
	return u.bytes[0] > 0;
}

static uint32_t swap_to_le(uint32_t val) {
	union {
		uint8_t bytes[4];
		uint32_t dword;
	} u;
	if (is_little_endian())
		return val;
	u.dword = val;
	uint8_t tmp = u.bytes[0];
	u.bytes[0] = u.bytes[3];
	u.bytes[3] = tmp;
	tmp = u.bytes[1];
	u.bytes[1] = u.bytes[2];
	u.bytes[2] = tmp;
	return u.dword;
}


class ElfMemoryLoader : public ELFIO::Loader {
	const uint8_t* data;
	uint32_t dataSize;
public:
	uint32_t moff;

	ElfMemoryLoader(const void* d, uint32_t s) : data(reinterpret_cast<const uint8_t*>(d)), dataSize(s) {
	}

	virtual ~ElfMemoryLoader() {
	}

	virtual unsigned int read(void* buffer, unsigned int off, unsigned int size) {
		if (off >= dataSize)
			return 0;
		unsigned int readSize = dataSize - off;
		if (size < readSize)
			readSize = size;

		memcpy(buffer, data + off, readSize);
		return readSize;
	}
};

PblLibrary::PblLibrary(const char* cstrPlatformName) : platformName(cstrPlatformName) {
}

PblLibrary::~PblLibrary() {
}

bool PblLibrary::loadFromArArchive(ArArchive& archive, bool verbose) {
	// check file entry
	if (archive.getFileCount() != 3 ||
		strcmp(archive.getFileName(0), "/") != 0 ||
		strcmp(archive.getFileName(1), "//") != 0) {
		verbose && std::cerr << "Invalid pebble library content for " << platformName << std::endl;
		return false;
	}
	uint32_t fileIdx = 2;
	const char* fileName = archive.getFileName(fileIdx);
	const char* fileExt = fileName + strlen(fileName) - 2;
	if (fileExt < fileName || strcmp(fileExt, ".o") != 0) {
		verbose && std::cerr << "Invalid pebble library content for " << platformName << std::endl;
		return false;
	}

	// read as elf file
	ElfMemoryLoader loader(archive.getFileBuffer(fileIdx), archive.getFileSize(fileIdx));
	return loadFromELF(&loader, verbose);
}

bool PblLibrary::loadFromELF(ELFIO::Loader* loader, bool verbose) {
	// the pebble libraries have section for every export function
	// these sections are named .text.<function_name>
	if (!elf.load(loader)) {
		verbose && std::cerr << "Could not load ELF file for " << platformName << std::endl;
		return false;
	}

	// Find functions
	auto itSection = elf.sections.begin();
	for (; itSection != elf.sections.end(); ++itSection) {
		if ((*itSection)->get_name().find(".text.") == 0) {
			Function function;
			function.section = *itSection;
			function.name = (*itSection)->get_name().substr(6);
			function.relocatedOffset = UINT32_MAX;
			function.symbolTableOffset = UINT32_MAX;

			// find relocation entry
			ELFIO::section* relocSection = elf.sections[".rel.text." + function.name];
			if (relocSection != nullptr) {
				ELFIO::relocation_section_accessor reloc(elf, relocSection);
				if (reloc.get_entries_num() > 1) {
					verbose && std::cerr << "Ignored function \"" << function.name << "\" for " << platformName <<
						" because of too many relocation entries" << std::endl;
					continue;
				}
				else if (reloc.get_entries_num() == 1) {
					ELFIO::Elf64_Addr offset;
					ELFIO::Elf_Word symbol, type;
					ELFIO::Elf_Sxword addend;
					if (!reloc.get_entry(0, offset, symbol, type, addend) || type != 30) {
						verbose && std::cerr << "Ignored function \"" << function.name << "\" for " << platformName <<
							" because invalid relocation entry" << std::endl;
						continue;
					}

					function.relocatedOffset = static_cast<uint32_t>(offset);
				}
			}

			// read out symbol table offset
			if ((*itSection)->get_size() == 12)
				function.symbolTableOffset = swap_to_le(*(uint32_t*)((*itSection)->get_data() + 8)) / 4;
			else if (verbose)
				std::cerr << "Unknown function format \"" << function.name << "\" is " << (*itSection)->get_size() << "B long" << std::endl;

			functions.push_back(function);
		}
	}

	verbose && std::cerr << "Found " << functions.size() << " functions for " << platformName << std::endl;

	return functions.size() > 0;
}

const char* PblLibrary::getPlatformName() const {
	return platformName.c_str();
}

uint32_t PblLibrary::getFunctionCount() const {
	return functions.size();
}

const char* PblLibrary::getFunctionName(uint32_t index) const {
	if (index >= functions.size())
		return nullptr;
	else
		return functions[index].name.c_str();
}

const void* PblLibrary::getFunctionCode(uint32_t index) const {
	if (index >= functions.size())
		return nullptr;
	else
		return reinterpret_cast<const void*>(functions[index].section->get_data());
}

uint32_t PblLibrary::getFunctionCodeSize(uint32_t index) const {
	if (index >= functions.size())
		return 0;
	else
		return static_cast<uint32_t>(functions[index].section->get_size());
}

uint32_t PblLibrary::getFunctionRelocatedOffset(uint32_t index) const {
	if (index >= functions.size())
		return UINT32_MAX;
	else
		return functions[index].relocatedOffset;
}

uint32_t PblLibrary::getFunctionSymbolTableOffset(uint32_t index) const {
	if (index >= functions.size())
		return UINT32_MAX;
	else
		return functions[index].symbolTableOffset;
}
