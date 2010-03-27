/*	$NetBSD: data.c,v 1.8 2000/04/02 11:10:53 augustss Exp $	*/

/*
 * Copyright (c) 1999 Lennart Augustsson <augustss@netbsd.org>
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#include <sys/cdefs.h>
__FBSDID("$FreeBSD$");

#include <sys/param.h>
#include <assert.h>
#include <stdlib.h>
#include "usbhid.h"

int
hid_get_data(const void *p, const hid_item_t *h)
{
	const uint8_t *buf;
	uint32_t hpos;
	uint32_t hsize;
	uint32_t data;
	int i, end, offs;

	buf = p;

	/* Skip report ID byte. */
	if (h->report_ID > 0)
		buf++;

	hpos = h->pos;			/* bit position of data */
	hsize = h->report_size;		/* bit length of data */

	/* Range check and limit */
	if (hsize == 0)
		return (0);
	if (hsize > 32)
		hsize = 32;

	offs = hpos / 8;
	end = (hpos + hsize) / 8 - offs;
	data = 0;
	for (i = 0; i <= end; i++)
		data |= buf[offs + i] << (i*8);
	data >>= hpos % 8;
	data &= (1 << hsize) - 1;
	if (h->logical_minimum < 0) {
		/* Need to sign extend */
		hsize = sizeof data * 8 - hsize;
		data = (data << hsize) >> hsize;
	}
	return (data);
}

void
hid_set_data(void *p, const hid_item_t *h, int data)
{
	uint8_t *buf;
	uint32_t hpos;
	uint32_t hsize;
	int i, end, offs, mask;

	buf = p;

	/* Set report ID byte. */
	if (h->report_ID > 0)
		*buf++ = h->report_ID & 0xff;

	hpos = h->pos;			/* bit position of data */
	hsize = h->report_size;		/* bit length of data */

	if (hsize != 32) {
		mask = (1 << hsize) - 1;
		data &= mask;
	} else
		mask = ~0;

	data <<= (hpos % 8);
	mask <<= (hpos % 8);
	mask = ~mask;

	offs = hpos / 8;
	end = (hpos + hsize) / 8 - offs;

	for (i = 0; i <= end; i++)
		buf[offs + i] = (buf[offs + i] & (mask >> (i*8))) |
		    ((data >> (i*8)) & 0xff);
}
