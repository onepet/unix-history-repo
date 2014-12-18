/*
 * Copyright (C) 2014  Internet Systems Consortium, Inc. ("ISC")
 *
 * Permission to use, copy, modify, and/or distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND ISC DISCLAIMS ALL WARRANTIES WITH
 * REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY
 * AND FITNESS.  IN NO EVENT SHALL ISC BE LIABLE FOR ANY SPECIAL, DIRECT,
 * INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM
 * LOSS OF USE, DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE
 * OR OTHER TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR
 * PERFORMANCE OF THIS SOFTWARE.
 */

#ifndef GENERIC_CDNSKEY_60_H
#define GENERIC_CDNSKEY_60_H 1

/* CDNSKEY records have the same RDATA fields as DNSKEY records. */
typedef struct dns_rdata_cdnskey {
	dns_rdatacommon_t	common;
	isc_mem_t *		mctx;
	isc_uint16_t		flags;
	isc_uint8_t		protocol;
	isc_uint8_t		algorithm;
	isc_uint16_t		datalen;
	unsigned char *		data;
} dns_rdata_cdnskey_t;


#endif /* GENERIC_CDNSKEY_60_H */
