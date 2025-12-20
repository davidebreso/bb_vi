/* vi: set sw=4 ts=4: */
/*
 * String utility routines.
 *
 * Copyright (C) 2008 Rob Landley <rob@landley.net>
 * Copyright (C) 2008 Denys Vlasenko <vda.linux@googlemail.com>
 *
 * Licensed under GPLv2, see file LICENSE in this source tree.
 */
#include "libbb.h"

/* Find out if the last character of a string matches the one given */
char *FAST_FUNC last_char_is(const char *s, int c) {
    if (!s[0])
        return NULL;
    while (s[1])
        s++;
    return (*s == (char)c) ? (char *)s : NULL;
}

char *FAST_FUNC skip_whitespace(const char *s) {
    /* In POSIX/C locale (the only locale we care about: do we REALLY want
	 * to allow Unicode whitespace in, say, .conf files? nuts!)
	 * isspace is only these chars: "\t\n\v\f\r" and space.
	 * "\t\n\v\f\r" happen to have ASCII codes 9,10,11,12,13.
	 * Use that.
	 */
    while (*s == ' ' || (unsigned char)(*s - 9) <= (13 - 9))
        s++;

    return (char *)s;
}

char *FAST_FUNC skip_non_whitespace(const char *s) {
    while (*s != '\0' && *s != ' ' && (unsigned char)(*s - 9) > (13 - 9))
        s++;

    return (char *)s;
}
