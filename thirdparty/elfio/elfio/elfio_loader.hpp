#pragma once

namespace ELFIO {
	class Loader {
	public:
		virtual ~Loader() = default;
		virtual unsigned int read(void* buffer, unsigned int off, unsigned int size) = 0;
	};
}
