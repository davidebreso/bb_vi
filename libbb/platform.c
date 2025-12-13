/*
 * Replacements for common but usually nonstandard functions that aren't
 * supplied by all platforms.
 *
 * Copyright (C) 2009 by Dan Fandrich <dan@coneharvesters.com>, et. al.
 *
 * Licensed under GPLv2, see file LICENSE in this source tree.
 */
#include "libbb.h"

#ifndef HAVE_STRCHRNUL
char* FAST_FUNC strchrnul(const char *s, int c)
{
	while (*s != '\0' && *s != c)
		s++;
	return (char*)s;
}
#endif

#ifndef HAVE_MEMRCHR
/* Copyright (C) 2005 Free Software Foundation, Inc.
 * memrchr() is a GNU function that might not be available everywhere.
 * It's basically the inverse of memchr() - search backwards in a
 * memory block for a particular character.
 */
void* FAST_FUNC memrchr(const void *s, int c, size_t n)
{
	const char *start = s, *end = s;

	end += n - 1;

	while (end >= start) {
		if (*end == (char)c)
			return (void *) end;
		end--;
	}

	return NULL;
}
#endif

