#include "pbw_api_info.h"

#include <stdio.h>

static constexpr char* ArMagic = "!<arch>\n";
static constexpr uint32_t ArMagicLen = 8;
static constexpr uint8_t ArFMagic[2] = { '`', '\n' };

ArArchive::ArArchive() {
}

ArArchive::~ArArchive() {
	auto itEntry = files.begin();
	for (; itEntry != files.end(); ++itEntry)
		free(itEntry->buffer);
}

bool ArArchive::load(const char* filename) {
	FILE* fp = fopen(filename, "rb");
	if (fp == nullptr)
		return false;
	fseek(fp, 0, SEEK_END);
	uint32_t fileSize = ftell(fp);
	fseek(fp, 0, SEEK_SET);

	// Check magic
	char magic[ArMagicLen];
	if (fread(magic, 1, ArMagicLen, fp) != ArMagicLen)
		return false;
	if (memcmp(ArMagic, magic, ArMagicLen) != 0)
		return false;

	// Read files
	while (!feof(fp) && static_cast<uint32_t>(ftell(fp)) < fileSize) {
		// each file is placed at an even byte offset
		if (ftell(fp) % 2 > 0) {
			if (!fgetc(fp))
				return false;
		}

		// Parse the header
		FileEntry entry;
		FileHeader header;
		if (fread(&header, 1, sizeof(FileHeader), fp) != sizeof(FileHeader))
			return false;
		if (memcmp(ArFMagic, header.ar_fmag, sizeof(ArFMagic)) != 0)
			return false;
		if (!parseFileName(header.ar_name, entry.name))
			return false;
		entry.size = static_cast<uint32_t>(strtoul(header.ar_size, nullptr, 10));
		entry.offset = ftell(fp);
		if (entry.size == 0)
			return false;

		// Read the file
		entry.buffer = malloc(entry.size);
		if (entry.buffer == nullptr)
			return false;
		if (fread(entry.buffer, 1, entry.size, fp) != entry.size)
			return false;

		files.push_back(entry);
	}

	fclose(fp);
	return true;
}

// cannot find the null-character after maxLen
const char* ar_strnrchr(const char* in, uint32_t maxLen, char chr) {
	if (maxLen == 0)
		return nullptr;
	in += maxLen - 1;
	while (maxLen != 0 && *in != chr) {
		maxLen--;
		in--;
	}
	if (maxLen == UINT32_MAX) // overflow
		return nullptr;
	else
		return in;
}

// returns UINT32_MAX on failure
uint32_t ar_atou(const char* str, uint32_t maxLen) {
	// skip leading space
	while (isspace(*str) && maxLen != 0) {
		str++;
		maxLen--;
	}
	if (maxLen == UINT32_MAX || !isdigit(*str))
		return UINT32_MAX;

	// parse integer
	uint32_t result = 0;
	while (isdigit(*str) && maxLen != 0) {
		result = result * 10 + (*str - '0');
		str++;
		maxLen--;
	}

	return result;
}

bool ArArchive::parseFileName(const char* inName, std::string& outName) {
	const char* slashPtr = ar_strnrchr(inName, 16, '/');
	if (slashPtr == nullptr)
		return false;

	// is symbol table?
	if (strncmp(inName, "/               ", 16) == 0) {
		if (files.size() != 0) // the symbol table has to be the first entry
			return false;
		outName = "/";
		return true;
	}
	// is string table?
	else if (strncmp(inName, "//              ", 16) == 0) {
		if (files.size() > 1) // the string table has to be the first or second entry
			return false;
		outName = "//";
		return true;
	}
	// is short enough to fit?
	else if (slashPtr != inName) {
		outName.assign(inName, slashPtr - inName);
		return true;
	}
	// name is stored in string table
	else {
		uint32_t strTableIdx = getFileIndex("//");
		if (strTableIdx == UINT32_MAX)
			return false;
		uint32_t strTableSize = getFileSize(strTableIdx);
		const char* strTableBuffer = reinterpret_cast<char*>(getFileBuffer(strTableIdx));

		uint32_t offset = ar_atou(inName + 1, 15); // no need for validation
		uint32_t endOffset = offset;
		while (endOffset < strTableSize && strTableBuffer[endOffset] != '/')
			endOffset++;
		if (endOffset >= strTableSize)
			return false;

		outName.assign(strTableBuffer + offset, endOffset - offset);
		return true;
	}
}

uint32_t ArArchive::getFileCount() const {
	return files.size();
}

uint32_t ArArchive::getFileIndex(const char* name) const {
	for (uint32_t i = 0; i < files.size(); i++) {
		if (files[i].name == name)
			return i;
	}
	return UINT32_MAX;
}

const char* ArArchive::getFileName(uint32_t index) const {
	if (index >= files.size())
		return nullptr;
	else
		return files[index].name.c_str();
}

uint32_t ArArchive::getFileSize(uint32_t index) const {
	if (index >= files.size())
		return 0;
	else
		return files[index].size;
}

uint32_t ArArchive::getFileOffset(uint32_t index) const {
	if (index >= files.size())
		return 0;
	else
		return files[index].offset;
}

void* ArArchive::getFileBuffer(uint32_t index) {
	if (index >= files.size())
		return 0;
	else
		return files[index].buffer;
}
