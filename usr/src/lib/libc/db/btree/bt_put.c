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
static char sccsid[] = "@(#)bt_put.c	5.4 (Berkeley) %G%";
#endif /* LIBC_SCCS and not lint */

#include <sys/types.h>
#include <errno.h>
#include <db.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "btree.h"

static EPG *bt_fast __P((BTREE *, const DBT *, const DBT *, int *));

/*
 * __BT_PUT -- Add a btree item to the tree.
 *
 * Parameters:
 *	dbp:	pointer to access method
 *	key:	key
 *	data:	data
 *	flag:	R_NOOVERWRITE
 *
 * Returns:
 *	RET_ERROR, RET_SUCCESS and RET_SPECIAL if the key is already in the
 *	tree and R_NOOVERWRITE specified.
 */
int
__bt_put(dbp, key, data, flags)
	const DB *dbp;
	const DBT *key, *data;
	u_int flags;
{
	BTREE *t;
	DBT tkey, tdata;
	EPG *e;
	PAGE *h;
	index_t index, nxtindex;
	pgno_t pg;
	size_t nbytes;
	int dflags, exact;
	char *dest, db[NOVFLSIZE], kb[NOVFLSIZE];

	if (flags && flags != R_NOOVERWRITE) {
		errno = EINVAL;
		return (RET_ERROR);
	}
	t = dbp->internal;
	if (ISSET(t, BTF_RDONLY)) {
		errno = EPERM;
		return (RET_ERROR);
	}
	
	/*
	 * If the key/data won't fit on a page, store it on indirect pages.
	 *
	 * XXX
	 * If the insert fails later on, these pages aren't recovered.
	 */
	dflags = 0;
	if (key->size >= t->bt_minkeypage) {
		if (__ovfl_put(t, key, &pg) == RET_ERROR)
			return (RET_ERROR);
		tkey.data = kb;
		tkey.size = NOVFLSIZE;
		*(pgno_t *)kb = pg;
		*(size_t *)(kb + sizeof(pgno_t)) = key->size;
		dflags |= P_BIGKEY;
		key = &tkey;
	}
	if (data->size >= t->bt_minkeypage) {
		if (__ovfl_put(t, data, &pg) == RET_ERROR)
			return (RET_ERROR);
		tdata.data = db;
		tdata.size = NOVFLSIZE;
		*(pgno_t *)db = pg;
		*(size_t *)(db + sizeof(pgno_t)) = data->size;
		dflags |= P_BIGDATA;
		data = &tdata;
	}

	/* bt_fast and __bt_search pin the returned page. */
	if (t->bt_order == NOT || (e = bt_fast(t, key, data, &exact)) == NULL)
		if ((e = __bt_search(t, key, &exact)) == NULL)
			return (RET_ERROR);

	h = e->page;
	index = e->index;

	/*
	 * Add the specified key/data pair to the tree.  If an identical key
	 * is already in the tree, and R_NOOVERWRITE is set, an error is
	 * returned.  If R_NOOVERWRITE is not set, the key is either added (if
	 * duplicates are permitted) or an error is returned.
	 *
	 * Pages are split as required.
	 */
	switch (flags) {
	case R_NOOVERWRITE:
		if (!exact)
			break;
		/*
		 * One special case is if the cursor references the record and
		 * it's been flagged for deletion.  If so, we delete it and
		 * pretend it was never there.  Since the cursor will move to
		 * the next record the inserted record won't be seen.
		 */
		if (ISSET(t, BTF_DELCRSR) && t->bt_bcursor.pgno == h->pgno &&
		    t->bt_bcursor.index == index) {
			UNSET(t, BTF_DELCRSR);
			goto delete;
		}
		BT_CLR(t);
		mpool_put(t->bt_mp, h, 0);
		return (RET_SPECIAL);
	default:
		if (!exact || NOTSET(t, BTF_NODUPS))
			break;
delete:		if (__bt_dleaf(t, h, index) == RET_ERROR) {
			BT_CLR(t);
			mpool_put(t->bt_mp, h, 0);
			return (RET_ERROR);
		}
		break;
	}

	/*
	 * If not enough room, or the user has put a ceiling on the number of
	 * keys permitted in the page, split the page.  The split code will
	 * insert the key and data and unpin the current page.  If inserting
	 * into the offset array, shift the pointers up.
	 */
	nbytes = NBLEAFDBT(key->size, data->size);
	if (h->upper - h->lower < nbytes + sizeof(index_t) ||
	    t->bt_maxkeypage && t->bt_maxkeypage < NEXTINDEX(h))
		return (__bt_split(t, h, key, data, dflags, nbytes, index));

	if (index < (nxtindex = NEXTINDEX(h)))
		bcopy(h->linp + index, h->linp + index + 1,
		    (nxtindex - index) * sizeof(index_t));
	h->lower += sizeof(index_t);

	h->linp[index] = h->upper -= nbytes;
	dest = (char *)h + h->upper;
	WR_BLEAF(dest, key, data, dflags);

	if (t->bt_order == NOT)
		if (h->nextpg == P_INVALID) {
			if (index == NEXTINDEX(h) - 1) {
				t->bt_order = FORWARD;
				t->bt_last.index = index;
				t->bt_last.pgno = h->pgno;
			}
		} else if (h->prevpg == P_INVALID) {
			if (index == 0) {
				t->bt_order = BACK;
				t->bt_last.index = 0;
				t->bt_last.pgno = h->pgno;
			}
		}

	BT_CLR(t);
	mpool_put(t->bt_mp, h, MPOOL_DIRTY);
	SET(t, BTF_MODIFIED);
	return (RET_SUCCESS);
}

#ifdef STATISTICS
u_long bt_cache_hit, bt_cache_miss;
#endif

/*
 * BT_FAST -- Do a quick check for sorted data.
 *
 * Parameters:
 *	t:	tree
 *	key:	key to insert
 *
 * Returns:
 * 	EPG for new record or NULL if not found.
 */
static EPG *
bt_fast(t, key, data, exactp)
	BTREE *t;
	const DBT *key, *data;
	int *exactp;
{
	EPG e;
	PAGE *h;
	size_t nbytes;
	int cmp;

	if ((h = mpool_get(t->bt_mp, t->bt_last.pgno, 0)) == NULL) {
		t->bt_order = NOT;
		return (NULL);
	}
	e.page = h;
	e.index = t->bt_last.index;

	/*
	 * If won't fit in this page or have too many keys in this page, have
	 * to search to get split stack.
	 */
	nbytes =
	    NBLEAFDBT(key->size >= t->bt_minkeypage ? NOVFLSIZE : key->size,
	    data->size >= t->bt_minkeypage ? NOVFLSIZE : data->size);
	if (h->upper - h->lower < nbytes + sizeof(index_t) ||
	    t->bt_maxkeypage && t->bt_maxkeypage < NEXTINDEX(h))
		goto miss;

	if (t->bt_order == FORWARD) {
		if (e.page->nextpg != P_INVALID)
			goto miss;
		if (e.index != NEXTINDEX(h) - 1)
			goto miss;
		if ((cmp = __bt_cmp(t, key, &e)) < 0)
			goto miss;
		t->bt_last.index = ++e.index;
	} else {
		if (e.page->prevpg != P_INVALID)
			goto miss;
		if (e.index != 0)
			goto miss;
		if ((cmp = __bt_cmp(t, key, &e)) > 0)
			goto miss;
		t->bt_last.index = 0;
	}
	*exactp = cmp == 0;
#ifdef STATISTICS
	++bt_cache_hit;
#endif
	return (&e);

miss:	
#ifdef STATISTICS
	++bt_cache_miss;
#endif
	t->bt_order = NOT;
	mpool_put(t->bt_mp, h, 0);
	return (NULL);
}
