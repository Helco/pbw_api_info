#include "pbw_api_info.h"

void* minizip_alloc(void* d, size_t items, size_t size) {
	return malloc(items * size);
}

void minizip_free(void* d, void* block) {
	free(block);
}

void* minizip_realloc(void* d, void* block, size_t items, size_t size) {
	return realloc(block, items * size);
}

PblAppArchive::PblAppArchive() {
	memset(&archive, 0, sizeof(archive));
	archive.m_pAlloc = minizip_alloc;
	archive.m_pFree = minizip_free;
	archive.m_pRealloc = minizip_realloc;
}

PblAppArchive::~PblAppArchive() {
	mz_zip_reader_end(&archive);
}

bool PblAppArchive::load(const char* filename, bool verbose) {
	if (!mz_zip_reader_init_file(&archive, filename, 0)) {
		verbose && std::cerr << "Could not open pebble app archive: " << mz_zip_get_error_string(mz_zip_get_last_error(&archive)) << std::endl;
		return false;
	}

	// find all files named <platform>/pebble-app.bin
	uint32_t fileCount = mz_zip_reader_get_num_files(&archive);
	for (uint32_t i = 0; i < fileCount; i++) {
		BinaryInfo info;
		std::string name;
		name.resize(mz_zip_reader_get_filename(&archive, i, nullptr, 0));
		mz_zip_reader_get_filename(&archive, i, &name[0], name.length());

		size_t posApp = name.find("pebble-app.bin");
		if (posApp == std::string::npos)
			continue;
		size_t slashPos = name.rfind('/', posApp);
		if (slashPos == std::string::npos)
			info.platform = "aplite";
		else
			info.platform = name.substr(0, slashPos);
		info.fileIndex = i;

		verbose && std::cerr << "Found binary for " << info.platform << std::endl;
		binaries.push_back(info);
	}

	return binaries.size() > 0;
}

uint32_t PblAppArchive::getBinaryCount() const {
	return binaries.size();
}

const char* PblAppArchive::getBinaryPlatform(uint32_t index) const {
	if (index >= binaries.size())
		return nullptr;
	else
		return binaries[index].platform.c_str();
}

void* PblAppArchive::extractBinary(uint32_t index, uint32_t* size, bool verbose) {
	if (index >= binaries.size() || size == nullptr)
		return nullptr;
	size_t tmpSize;
	void* result = mz_zip_reader_extract_to_heap(&archive, binaries[index].fileIndex, &tmpSize, 0);
        *size = tmpSize;
	if (!result && verbose) {
		const char* errString = mz_zip_get_error_string(mz_zip_get_last_error(&archive));
		std::cerr << "Could not extract binary for " << binaries[index].platform << ": " << errString << std::endl;
	}
	return result;
}
