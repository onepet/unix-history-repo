/*
 * Copyright (c) 1991, 1993
 *	The Regents of the University of California.  All rights reserved.
 *
 * This code is derived from software contributed to Berkeley by
 * The Mach Operating System project at Carnegie-Mellon University.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *	This product includes software developed by the University of
 *	California, Berkeley and its contributors.
 * 4. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 *	from: @(#)vm_object.h	8.3 (Berkeley) 1/12/94
 *
 *
 * Copyright (c) 1987, 1990 Carnegie-Mellon University.
 * All rights reserved.
 *
 * Authors: Avadis Tevanian, Jr., Michael Wayne Young
 *
 * Permission to use, copy, modify and distribute this software and
 * its documentation is hereby granted, provided that both the copyright
 * notice and this permission notice appear in all copies of the
 * software, derivative works or modified versions, and any portions
 * thereof, and that both notices appear in supporting documentation.
 *
 * CARNEGIE MELLON ALLOWS FREE USE OF THIS SOFTWARE IN ITS "AS IS"
 * CONDITION.  CARNEGIE MELLON DISCLAIMS ANY LIABILITY OF ANY KIND
 * FOR ANY DAMAGES WHATSOEVER RESULTING FROM THE USE OF THIS SOFTWARE.
 *
 * Carnegie Mellon requests users of this software to return to
 *
 *  Software Distribution Coordinator  or  Software.Distribution@CS.CMU.EDU
 *  School of Computer Science
 *  Carnegie Mellon University
 *  Pittsburgh PA 15213-3890
 *
 * any improvements or extensions that they make and grant Carnegie the
 * rights to redistribute these changes.
 *
 * $Id: vm_object.h,v 1.10 1995/03/04 21:14:19 jkh Exp $
 */

/*
 *	Virtual memory object module definitions.
 */

#ifndef	_VM_OBJECT_
#define	_VM_OBJECT_

#include <vm/vm_page.h>
#include <vm/vm_pager.h>

/*
 *	Types defined:
 *
 *	vm_object_t		Virtual memory object.
 */

struct vm_object {
	struct pglist memq;		/* Resident memory */
	TAILQ_HEAD(rslist, vm_object) reverse_shadow_head; /* objects that this is a shadow for */
	TAILQ_ENTRY(vm_object) object_list; /* list of all objects */
	TAILQ_ENTRY(vm_object) reverse_shadow_list; /* chain of objects that are shadowed */
	TAILQ_ENTRY(vm_object) cached_list; /* for persistence */
	vm_size_t size;			/* Object size */
	int ref_count;			/* How many refs?? */
	u_short flags;			/* see below */
	u_short paging_in_progress;	/* Paging (in or out) so don't collapse or destroy */
	int resident_page_count;	/* number of resident pages */
	vm_pager_t pager;		/* Where to get data */
	vm_offset_t paging_offset;	/* Offset into paging space */
	struct vm_object *shadow;	/* My shadow */
	vm_offset_t shadow_offset;	/* Offset in shadow */
	struct vm_object *copy;		/* Object that holds copies of my changed pages */
	vm_offset_t last_read;		/* last read in object -- detect seq behavior */
};

/*
 * Flags
 */
#define OBJ_CANPERSIST	0x0001		/* allow to persist */
#define OBJ_INTERNAL	0x0002		/* internally created object */
#define OBJ_ACTIVE	0x0004		/* used to mark active objects */
#define OBJ_DEAD	0x0008		/* used to mark dead objects during rundown */
#define OBJ_ILOCKED	0x0010		/* lock from modification */
#define OBJ_ILOCKWT	0x0020		/* wait for lock from modification */
#define	OBJ_PIPWNT	0x0040		/* paging in progress wanted */

TAILQ_HEAD(vm_object_hash_head, vm_object_hash_entry);

struct vm_object_hash_entry {
	TAILQ_ENTRY(vm_object_hash_entry) hash_links;	/* hash chain links */
	vm_object_t object;		/* object represened */
};

typedef struct vm_object_hash_entry *vm_object_hash_entry_t;

#ifdef	KERNEL
TAILQ_HEAD(object_q, vm_object);

struct object_q vm_object_cached_list;	/* list of objects persisting */
int vm_object_cached;			/* size of cached list */
simple_lock_data_t vm_cache_lock;	/* lock for object cache */

struct object_q vm_object_list;		/* list of allocated objects */
long vm_object_count;			/* count of all objects */
simple_lock_data_t vm_object_list_lock;

 /* lock for object list and count */

vm_object_t kernel_object;		/* the single kernel object */
vm_object_t kmem_object;

#define	vm_object_cache_lock()		simple_lock(&vm_cache_lock)
#define	vm_object_cache_unlock()	simple_unlock(&vm_cache_lock)
#endif				/* KERNEL */

#if 1
#define	vm_object_lock_init(object)	simple_lock_init(&(object)->Lock)
#define	vm_object_lock(object)		simple_lock(&(object)->Lock)
#define	vm_object_unlock(object)	simple_unlock(&(object)->Lock)
#define	vm_object_lock_try(object)	simple_lock_try(&(object)->Lock)
#endif

__inline static void
vm_object_pip_wakeup( vm_object_t object) {
	object->paging_in_progress--;
	if ((object->flags & OBJ_PIPWNT) &&
		object->paging_in_progress == 0) {
		object->flags &= ~OBJ_PIPWNT;
		wakeup(object);
	}
}

#ifdef KERNEL
vm_object_t vm_object_allocate __P((vm_size_t));
void vm_object_cache_clear __P((void));
void vm_object_cache_trim __P((void));
boolean_t vm_object_coalesce __P((vm_object_t, vm_object_t, vm_offset_t, vm_offset_t, vm_offset_t, vm_size_t));
void vm_object_collapse __P((vm_object_t));
void vm_object_copy __P((vm_object_t, vm_offset_t, vm_size_t, vm_object_t *, vm_offset_t *, boolean_t *));
void vm_object_deactivate_pages __P((vm_object_t));
void vm_object_deallocate __P((vm_object_t));
void vm_object_enter __P((vm_object_t, vm_pager_t));
void vm_object_init __P((vm_size_t));
vm_object_t vm_object_lookup __P((vm_pager_t));
boolean_t vm_object_page_clean __P((vm_object_t, vm_offset_t, vm_offset_t, boolean_t, boolean_t));
void vm_object_page_remove __P((vm_object_t, vm_offset_t, vm_offset_t));
void vm_object_pmap_copy __P((vm_object_t, vm_offset_t, vm_offset_t));
void vm_object_pmap_remove __P((vm_object_t, vm_offset_t, vm_offset_t));
void vm_object_print __P((vm_object_t, boolean_t));
void vm_object_reference __P((vm_object_t));
void vm_object_remove __P((vm_pager_t));
void vm_object_shadow __P((vm_object_t *, vm_offset_t *, vm_size_t));
void vm_object_terminate __P((vm_object_t));

#endif
#endif				/* _VM_OBJECT_ */
