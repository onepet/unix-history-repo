/*-
 * Copyright (c) 1990 The Regents of the University of California.
 * All rights reserved.
 *
 * This code is derived from software contributed to Berkeley by
 * Mike Olson.
 *
 * %sccs.include.redist.c%
 */

#if defined(LIBC_SCCS) && !defined(lint)
static char sccsid[] = "@(#)bt_overflow.c	5.4 (Berkeley) %G%";
#endif /* LIBC_SCCS and not lint */

#include <sys/param.h>
#include <db.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "btree.h"

/*
 * Big key/data code.
 *
 * Big key and data entries are stored on linked lists of pages.  The initial
 * reference is byte string stored with the key or data and is the page number
 * and size.  The actual record is stored in a chain of pages linked by the
 * nextpg field of the PAGE header.
 *
 * The first page of the chain has a special property.  If the record is used
 * by an internal page, it cannot be deleted and the P_PRESERVE bit will be set
 * in the header.
 *
 * XXX
 * A single DBT is written to each chain, so a lot of space on the last page
 * is wasted.  This is a fairly major bug for some data sets.
 */

/*
 * __OVFL_GET -- Get an overflow key/data item.
 *
 * Parameters:
 *	t:	tree
 *	p:	pointer to { pgno_t, size_t }
 *	buf:	storage address
 *	bufsz:	storage size
 *
 * Returns:
 *	RET_ERROR, RET_SUCCESS
 */
int
__ovfl_get(t, p, ssz, buf, bufsz)
	BTREE *t;
	void *p;
	size_t *ssz;
	char **buf;
	size_t *bufsz;
{
	PAGE *h;
	pgno_t pg;
	size_t nb, plen, sz;

	pg = *(pgno_t *)p;
	*ssz = sz = *(size_t *)((char *)p + sizeof(pgno_t));

#ifdef DEBUG
	if (pg == P_INVALID || sz == 0)
		abort();
#endif
	/* Make the buffer bigger as necessary. */
	if (*bufsz < sz) {
		if ((*buf = realloc(*buf, sz)) == NULL)
			return (RET_ERROR);
		*bufsz = sz;
	}

	/*
	 * Step through the linked list of pages, copying the data on each one
	 * into the buffer.  Never copy more than the data's length.
	 */
	plen = t->bt_psize - BTDATAOFF;
	for (p = *buf;; p = (char *)p + nb, pg = h->nextpg) {
		if ((h = mpool_get(t->bt_mp, pg, 0)) == NULL)
			return (RET_ERROR);

		nb = MIN(sz, plen);
		bcopy((char *)h + BTDATAOFF, p, nb);
		mpool_put(t->bt_mp, h, 0);

		if ((sz -= nb) == 0)
			break;
	}
	return (RET_SUCCESS);
}

/*
 * __OVFL_PUT -- Store an overflow key/data item.
 *
 * Parameters:
 *	t:	tree
 *	data:	DBT to store
 *	pgno:	storage page number
 *
 * Returns:
 *	RET_ERROR, RET_SUCCESS
 */
int
__ovfl_put(t, dbt, pg)
	BTREE *t;
	const DBT *dbt;
	pgno_t *pg;
{
	PAGE *h, *last;
	void *p;
	pgno_t npg;
	size_t nb, plen, sz;

	/*
	 * Allocate pages and copy the key/data record into them.  Store the
	 * number of the first page in the chain.
	 */
	plen = t->bt_psize - BTDATAOFF;
	for (last = NULL, p = dbt->data, sz = dbt->size;;
	    p = (char *)p + plen, last = h) {
		if ((h = __bt_new(t, &npg)) == NULL)
			return (RET_ERROR);

		h->pgno = npg;
		h->nextpg = h->prevpg = P_INVALID;
		h->lower = h->upper = 0;

		nb = MIN(sz, plen);
		bcopy(p, (char *)h + BTDATAOFF, nb);

		if (last) {
			last->nextpg = h->pgno;
			last->flags |= P_OVERFLOW;
			mpool_put(t->bt_mp, last, MPOOL_DIRTY);
		} else
			*pg = h->pgno;

		if ((sz -= nb) == 0) {
			mpool_put(t->bt_mp, h, MPOOL_DIRTY);
			break;
		}
	}
	return (RET_SUCCESS);
}

/*
 * __OVFL_DELETE -- Delete an overflow chain.
 *
 * Parameters:
 *	t:	tree
 *	p:	pointer to { pgno_t, size_t }
 *
 * Returns:
 *	RET_ERROR, RET_SUCCESS
 */
int
__ovfl_delete(t, p)
	BTREE *t;
	void *p;
{
	PAGE *h;
	pgno_t pg;
	size_t plen, sz;

	pg = *(pgno_t *)p;
	sz = *(size_t *)((char *)p + sizeof(pgno_t));

#ifdef DEBUG
	if (pg == P_INVALID || sz == 0)
		abort();
#endif
	if ((h = mpool_get(t->bt_mp, pg, 0)) == NULL)
		return (RET_ERROR);

	/* Don't delete chains used by internal pages. */
	if (h->flags & P_PRESERVE) {
		mpool_put(t->bt_mp, h, 0);
		return (RET_SUCCESS);
	}

	/* Step through the chain, calling the free routine for each page. */
	for (plen = t->bt_psize - BTDATAOFF;; sz -= plen) {
		pg = h->nextpg;
		__bt_free(t, h);
		if (sz <= plen)
			break;
		if ((h = mpool_get(t->bt_mp, pg, 0)) == NULL)
			return (RET_ERROR);
	}
	return (RET_SUCCESS);
}
