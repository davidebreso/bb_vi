/* vi: set sw=4 ts=4: */
/*
 * Utility routines.
 *
 * Copyright (C) 1999-2004 by Erik Andersen <andersen@codepoet.org>
 *
 * Licensed under GPLv2 or later, see file LICENSE in this source tree.
 */
#include "libbb.h"

// Read (potentially big) files in one go. File size is estimated
// by stat. Extra '\0' byte is appended.
void* FAST_FUNC xmalloc_read_with_initial_buf(int fd, size_t *maxsz_p, char *buf, size_t total)
{
	size_t size, rd_size;
	size_t to_read;
	struct stat st;

	to_read = maxsz_p ? *maxsz_p : (INT_MAX - 4095); /* max to read */

	/* Estimate file size */
	st.st_size = 0; /* in case fstat fails, assume 0 */
	fstat(fd, &st);
	/* /proc/N/stat files report st_size 0 */
	/* In order to make such files readable, we add small const */
	size = (st.st_size | 0x3ff) + 1;

	while (1) {
		if (to_read < size)
			size = to_read;
		buf = xrealloc(buf, total + size + 1);
		rd_size = full_read(fd, buf + total, size);
		if ((ssize_t)rd_size == (ssize_t)(-1)) { /* error */
			free(buf);
			return NULL;
		}
		total += rd_size;
		if (rd_size < size) /* EOF */
			break;
		if (to_read <= rd_size)
			break;
		to_read -= rd_size;
		/* grow by 1/8, but in [1k..64k] bounds */
		size = ((total / 8) | 0x3ff) + 1;
		if (size > 64*1024)
			size = 64*1024;
	}
	buf = xrealloc(buf, total + 1);
	buf[total] = '\0';

	if (maxsz_p)
		*maxsz_p = total;
	return buf;
}

void* FAST_FUNC xmalloc_read(int fd, size_t *maxsz_p)
{
	return xmalloc_read_with_initial_buf(fd, maxsz_p, NULL, 0);
}

#ifdef USING_LSEEK_TO_GET_SIZE
/* Alternatively, file size can be obtained by lseek to the end.
 * The code is slightly bigger. Retained in case fstat approach
 * will not work for some weird cases (/proc, block devices, etc).
 * (NB: lseek also can fail to work for some weird files) */

// Read (potentially big) files in one go. File size is estimated by
// lseek to end.
void* FAST_FUNC xmalloc_open_read_close(const char *filename, size_t *maxsz_p)
{
	char *buf;
	size_t size;
	int fd;
	off_t len;

	fd = open(filename, O_RDONLY);
	if (fd < 0)
		return NULL;

	/* /proc/N/stat files report len 0 here */
	/* In order to make such files readable, we add small const */
	size = 0x3ff; /* read only 1k on unseekable files */
	len = lseek(fd, 0, SEEK_END) | 0x3ff; /* + up to 1k */
	if (len != (off_t)-1) {
		xlseek(fd, 0, SEEK_SET);
		size = maxsz_p ? *maxsz_p : (INT_MAX - 4095);
		if (len < size)
			size = len;
	}

	buf = xmalloc(size + 1);
	size = read_close(fd, buf, size);
	if ((ssize_t)size < 0) {
		free(buf);
		return NULL;
	}
	buf = xrealloc(buf, size + 1);
	buf[size] = '\0';

	if (maxsz_p)
		*maxsz_p = size;
	return buf;
}
#endif

// Read (potentially big) files in one go. File size is estimated
// by stat.
void* FAST_FUNC xmalloc_open_read_close(const char *filename, size_t *maxsz_p)
{
	char *buf;
	int fd;

	fd = open(filename, O_RDONLY);
	if (fd < 0)
		return NULL;

	buf = xmalloc_read(fd, maxsz_p);
	close(fd);
	return buf;
}

