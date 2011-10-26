/*
 * Copyright (C) 2004 Joakim Tjernlund
 * Copyright (C) 2000-2005 Erik Andersen <andersen@uclibc.org>
 *
 * Licensed under the LGPL v2.1, see the file COPYING.LIB in this tarball.
 */

/* These are carefully optimized mem*() functions for PPC written in C.
 * Don't muck around with these function without checking the generated
 * assmbler code.
 * It is possible to optimize these significantly more by using specific
 * data cache instructions(mainly dcbz). However that requires knownledge
 * about the CPU's cache line size.
 *
 * BUG ALERT!
 * The cache instructions on MPC8xx CPU's are buggy(they don't update
 * the DAR register when causing a DTLB Miss/Error) and cannot be
 * used on 8xx CPU's without a kernel patch to work around this
 * problem.
 */

#include <string.h>

libc_hidden_proto(memcpy)
void *memcpy(void *to, const void *from, size_t n)
/* PPC can do pre increment and load/store, but not post increment and load/store.
   Therefore use *++ptr instead of *ptr++. */
{
	unsigned long rem, chunks, tmp1, tmp2;
	unsigned char *tmp_to;
	unsigned char *tmp_from = (unsigned char *)from;

	chunks = n / 8;
	tmp_from -= 4;
	tmp_to = to - 4;
	if (!chunks)
		goto lessthan8;
	rem = (unsigned long )tmp_to % 4;
	if (rem)
		goto align;
 copy_chunks:
	do {
		/* make gcc to load all data, then store it */
		tmp1 = *(unsigned long *)(tmp_from+4);
		tmp_from += 8;
		tmp2 = *(unsigned long *)tmp_from;
		*(unsigned long *)(tmp_to+4) = tmp1;
		tmp_to += 8;
		*(unsigned long *)tmp_to = tmp2;
	} while (--chunks);
 lessthan8:
	n = n % 8;
	if (n >= 4) {
		*(unsigned long *)(tmp_to+4) = *(unsigned long *)(tmp_from+4);
		tmp_from += 4;
		tmp_to += 4;
		n = n-4;
	}
	if (!n ) return to;
	tmp_from += 3;
	tmp_to += 3;
	do {
		*++tmp_to = *++tmp_from;
	} while (--n);
	
	return to;
 align:
	rem = 4 - rem;
	n = n - rem;
	do {
		*(tmp_to+4) = *(tmp_from+4);
		++tmp_from;
		++tmp_to;
	} while (--rem);
	chunks = n / 8;
	if (chunks)
		goto copy_chunks;
	goto lessthan8;
}
libc_hidden_def(memcpy)