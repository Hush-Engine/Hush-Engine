#if 0
#pragma once
#include <cstddef>
#include <string.h>

namespace utils {

	bool safe_strn_copy(char *dest, size_t destSize, const char *src) {
		//Code from: https://stackoverflow.com/a/63331927
		// Optional pointer test
		if (dest == NULL) return true;

		// Size test
		if (destSize == 0) return true;

		// Optional pointer test
		if (src == NULL) {
			dest[0] = '\0'; 
			return true;
		}

		// Look for null character, but not beyond destsz
		const char *end = (char*)memchr(src, '\0', destSize);
		if (end == NULL) {
		  memmove(dest, src, destSize - 1);
		  dest[destSize - 1] = '\0';
		  return true;
		}
		memmove(dest, src, end - src);
		return false;
	}

}
#endif
