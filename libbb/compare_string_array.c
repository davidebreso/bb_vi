/* vi: set sw=4 ts=4: */
/*
 * Licensed under GPLv2 or later, see file LICENSE in this source tree.
 */
#include "libbb.h"

int FAST_FUNC index_in_strings(const char *strings, const char *key)
{
	int j, idx = 0;

	while (*strings) {
		/* Do we see "key\0" at current position in strings? */
		for (j = 0; *strings == key[j]; ++j) {
			if (*strings++ == '\0') {
				//bb_error_msg("found:'%s' i:%u", key, idx);
				return idx; /* yes */
			}
		}
		/* No.  Move to the start of the next string. */
		while (*strings++ != '\0')
			continue;
		idx++;
	}
	return -1;
}

