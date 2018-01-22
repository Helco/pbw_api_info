#ifndef PBW_API_INFO_H
#define PBW_API_INFO_H

#include <string>
#include <vector>

#include "../thirdparty/elfio/elfio/elfio.hpp"
#include "../thirdparty/miniz/miniz_zip.h"

/**
 * .a archive reader, following the SRV4/GNU variant
 */
class ArArchive {
	struct FileHeader {
		char ar_name[16];
		char ar_date[12];
		char ar_uid[6];
		char ar_gid[6];
		char ar_mode[8];
		char ar_size[10];
		uint8_t ar_fmag[2];
	};

	struct FileEntry {
		std::string name;
		uint32_t size, offset;
		void* buffer;
	};

	std::vector<FileEntry> files;

	bool parseFileName(const char* inName, std::string& outName);
public:
	ArArchive();
	~ArArchive();

	bool load(const char* filename);

	uint32_t getFileCount() const;
	uint32_t getFileIndex(const char* name) const; // returns UINT32_MAX on failure
	const char* getFileName(uint32_t index) const;
	uint32_t getFileSize(uint32_t index) const; // returns 0 on failure
	uint32_t getFileOffset(uint32_t index) const;
	void* getFileBuffer(uint32_t index); // has to be mutable for streambuf to work
};

/**
 * A pebble import library
 */
class PblLibrary {
	struct Function {
		ELFIO::section* section;
		std::string name;
		uint32_t relocatedOffset; // a 4 byte long relocation entry, which has to be ignored
		uint32_t symbolTableOffset; // the index in the symbol table
	};

	ELFIO::elfio elf;
	std::vector<Function> functions;
	std::string platformName;
public:
	PblLibrary(const char* platformName);
	~PblLibrary();

	bool loadFromArArchive(ArArchive& archive, bool verbose);
	bool loadFromELF(ELFIO::Loader* loader, bool verbose);

	const char* getPlatformName() const;
	uint32_t getFunctionCount() const;
	const char* getFunctionName(uint32_t index) const;
	const void* getFunctionCode(uint32_t index) const;
	uint32_t getFunctionCodeSize(uint32_t index) const;
	uint32_t getFunctionRelocatedOffset(uint32_t index) const;
	uint32_t getFunctionSymbolTableOffset(uint32_t index) const;
};

/**
 * A pebble app archive
 */
class PblAppArchive {
	struct BinaryInfo {
		uint32_t fileIndex;
		std::string platform;
	};

	mz_zip_archive archive;
	std::vector<BinaryInfo> binaries;
public:
	PblAppArchive();
	~PblAppArchive();

	bool load(const char* filename, bool verbose);

	uint32_t getBinaryCount() const;
	const char* getBinaryPlatform(uint32_t index) const;
	void* extractBinary(uint32_t index, uint32_t* size, bool verbose);
};

/**
 * A pebble app binary header
 */
#define APP_NAME_SIZE 32

#pragma pack(push, 1)
struct PblAppHeader {
	char magic[8];
	uint8_t struct_version_major, struct_version_minor;
	uint8_t sdk_version_major, sdk_version_minor;
	uint8_t process_version_major, process_version_minor;
	uint16_t loadSize;
	uint32_t offset;
	uint32_t crc;
	char name[APP_NAME_SIZE];
	char company[APP_NAME_SIZE];
	uint32_t icon_resource_id;
	uint32_t sym_table_addr;
	uint32_t flags;
	uint32_t num_reloc_entries;
	uint8_t uuid[16];
	uint32_t resource_crc;
	uint32_t resource_timestamp;
	uint16_t virtualSize;
};
#pragma pack(pop)

/**
 * A pebble app binary
 */
class PblAppBinary {
	PblLibrary* library;
	void* buffer;
	uint32_t size;
	std::vector<uint32_t> usedFunctions;
public:
	PblAppBinary(void* buffer, uint32_t size, PblLibrary* library);
	~PblAppBinary();

	uint32_t scan();

	const char* getPlatformName() const;
	const PblAppHeader* getHeader() const;
	uint32_t getUsedFunctionCount() const;
	uint32_t getUsedFunctionIndex(uint32_t index) const;
	const char* getUsedFunctionName(uint32_t index) const;
};

#endif // PBW_API_INFO_H
