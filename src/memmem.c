#include <dfk/memmem.h>

#if DFK_HAVE_MEMMEM

#define _GNU_SOURCE
#include <string.h>

void* dfk_memmem(const void *haystack, size_t haystack_len,
    const void *needle, size_t needle_len)
{
  return memmem(haystack, haystack_len, needle, needle_len);
}

#else

void* dfk_memmem(const void *haystack, size_t haystack_len,
    const void *needle, size_t needle_len)
{
	/* we need something to compare */
	if (haystack_len == 0 || needle_len == 0) {
		return NULL;
  }

	/* "needle" must be smaller or equal to "haystack" */
	if (haystack_len < needle_len) {
		return NULL;
  }

	/* the last position where its possible to find "needle" in "haystack" */
	char* last = (char*) haystack + haystack_len - needle_len;

	for (char* cur = (char*) haystack; cur <= last; cur++) {
		if (memcmp(cur, needle, needle_len) == 0) {
			return cur;
    }
  }

	return NULL;
}

#endif /* DFK_HAVE_MEMMEM */

