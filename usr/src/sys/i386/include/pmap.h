/* 
 * Copyright (c) 1987 Carnegie-Mellon University
 * Copyright (c) 1991 Regents of the University of California.
 * All rights reserved.
 *
 * This code is derived from software contributed to Berkeley by
 * The Mach Operating System project at Carnegie-Mellon University.
 *
 * The CMU software License Agreement specifies the terms and conditions
 * for use and redistribution.
 *
 * Derived from hp300 version by Mike Hibler, this version by William
 * Jolitz uses a recursive map [a pde points to the page directory] to
 * map the page tables using the pagetables themselves. This is done to
 * reduce the impact on kernel virtual memory for lots of sparse address
 * space, and to reduce the cost of memory to each process.
 *
 * from hp300:	@(#)pmap.h	7.2 (Berkeley) 12/16/90
 *
 *	@(#)pmap.h	1.2 (Berkeley) %G%
 */

#ifndef	_PMAP_MACHINE_
#define	_PMAP_MACHINE_	1

#include "sys/lock.h"
#include "machine/vmparam.h"
#include "vm/vm_statistics.h"

/*
 * 386 page table entry and page table directory
 * W.Jolitz, 8/89
 */

struct pde
{
unsigned int	
		pd_v:1,			/* valid bit */
		pd_prot:2,		/* access control */
		pd_mbz1:2,		/* reserved, must be zero */
		pd_u:1,			/* hardware maintained 'used' bit */
		:1,			/* not used */
		pd_mbz2:2,		/* reserved, must be zero */
		:3,			/* reserved for software */
		pd_pfnum:20;		/* physical page frame number of pte's*/
};

#define	PD_MASK		0xffc00000	/* page directory address bits */
#define	PT_MASK		0x003ff000	/* page table address bits */
#define	PD_SHIFT	22		/* page directory address shift */
#define	PG_SHIFT	12		/* page table address shift */

struct pte
{
unsigned int	
		pg_v:1,			/* valid bit */
		pg_prot:2,		/* access control */
		pg_mbz1:2,		/* reserved, must be zero */
		pg_u:1,			/* hardware maintained 'used' bit */
		pg_m:1,			/* hardware maintained modified bit */
		pg_mbz2:2,		/* reserved, must be zero */
		pg_w:1,			/* software, wired down page */
		:1,			/* software (unused) */
		pg_nc:1,		/* 'uncacheable page' bit */
		pg_pfnum:20;		/* physical page frame number */
};

#define	PG_V		0x00000001
#define	PG_RO		0x00000000
#define	PG_RW		0x00000002
#define	PG_u		0x00000004
#define	PG_PROT		0x00000006 /* all protection bits . */
#define	PG_W		0x00000200
#define PG_N		0x00000800 /* Non-cacheable */
#define	PG_M		0x00000040
#define PG_U		0x00000020
#define	PG_FRAME	0xfffff000

#define	PG_NOACC	0
#define	PG_KR		0x00000000
#define	PG_KW		0x00000002
#define	PG_URKR		0x00000004
#define	PG_URKW		0x00000004
#define	PG_UW		0x00000006

/* Garbage for current bastardized pager that assumes a hp300 */
#define	PG_NV	0
#define	PG_CI	0
/*
 * Page Protection Exception bits
 */

#define PGEX_P		0x01	/* Protection violation vs. not present */
#define PGEX_W		0x02	/* during a Write cycle */
#define PGEX_U		0x04	/* access from User mode (UPL) */

typedef struct pde	pd_entry_t;	/* page directory entry */
typedef struct pte	pt_entry_t;	/* Mach page table entry */

#define	PD_ENTRY_NULL	((pd_entry_t *) 0)
#define	PT_ENTRY_NULL	((pt_entry_t *) 0)

/*
 * One page directory, shared between
 * kernel and user modes.
 */
#define I386_PAGE_SIZE	NBPG
#define I386_PDR_SIZE	NBPDR

#define I386_KPDES	8 /* KPT page directory size */
#define I386_UPDES	NBPDR/sizeof(struct pde)-8 /* UPT page directory size */

#define	UPTDI		0x3f6		/* ptd entry for u./kernel&user stack */
#define	PTDPTDI		0x3f7		/* ptd entry that points to ptd! */
#define	KPTDI_FIRST	0x3f8		/* start of kernel virtual pde's */
#define	KPTDI_LAST	0x3fA		/* last of kernel virtual pde's */

#define I386_MAX_PTSIZE	NPTEPG*NBPG	/* max size of PT */
#ifdef old
#define I386_MAX_KPTSIZE 0x100000 /* max memory to allocate to KPT */

#define	I386_PTBASE	0xfe200000
#define	I386_PTMAXSIZE	0x01000000

/*
 * Kernel virtual address to page table entry and to physical address.
 */
#define	kvtopte(va) \
	(&Sysmap[((unsigned)(va) - VM_MIN_KERNEL_ADDRESS) >> PGSHIFT])
#define	ptetokv(pt) \
	((((pt_entry_t *)(pt) - Sysmap) << PGSHIFT) + VM_MIN_KERNEL_ADDRESS)
#define	kvtophys(va) \
	((kvtopte(va)->pg_pfnum << PGSHIFT) | ((int)(va) & PGOFSET))
extern	pt_entry_t	*Sysmap;
#else

/*
 * Address of current and alternate address space page table maps
 * and directories.
 */
extern struct pte PTmap[], APTmap[], Upte;
extern struct pde PTD[], APTD[], PTDpde, APTDpde, Upde;

extern int IdlePTD;

/*
 * virtual address to page table entry and
 * to physical address. Likewise for alternate address space.
 */
#define	vtopte(va)	(PTmap + i386_btop(va))
#define	kvtopte(va)	vtopte(va)
#define	ptetov(pt)	(i386_ptob(pt - PTmap)) 
#define	vtophys(va)  (i386_ptob(vtopte(va)->pg_pfnum) | ((int)(va) & PGOFSET))

#define	avtopte(va)	(APTmap + i386_btop(va))
#define	ptetoav(pt)	(i386_ptob(pt - APTmap)) 
#define	avtophys(va)  (i386_ptob(avtopte(va)->pg_pfnum) | ((int)(va) & PGOFSET))

/*
 * macros to generate page directory/table indicies
 */

#define	pdei(va)	(((va)&PD_MASK)>>PD_SHIFT)
#define	ptei(va)	(((va)&PT_MASK)>>PT_SHIFT)
#endif

/*
 * Pmap stuff
 */
#define PMAP_NULL	((pmap_t) 0)

struct pmap {
	pd_entry_t		*pm_pdir;	/* KVA of page directory */
	/* caddr_t			*pm_ptobj;	/* page table object */
	boolean_t		pm_pdchanged;	/* pdir changed */
	short			pm_dref;	/* page directory ref count */
	short			pm_count;	/* pmap reference count */
	simple_lock_data_t	pm_lock;	/* lock on pmap */
	struct pmap_statistics	pm_stats;	/* pmap statistics */
	long			pm_ptpages;	/* more stats: PT pages */
};

typedef struct pmap	*pmap_t;

extern pmap_t		kernel_pmap;

/*
 * Macros for speed
 */
#define PMAP_ACTIVATE(pmapp, pcbp) \
	if ((pmapp) != PMAP_NULL /*&& (pmapp)->pm_pdchanged */) {  \
		(pcbp)->pcb_cr3 = \
		    pmap_extract(kernel_pmap, (pmapp)->pm_pdir); \
		if ((pmapp) == u.u_procp->p_map->pmap) \
			load_cr3((pcbp)->pcb_cr3); \
		(pmapp)->pm_pdchanged = FALSE; \
	}

#define PMAP_DEACTIVATE(pmapp, pcbp)

/*
 * For each vm_page_t, there is a list of all currently valid virtual
 * mappings of that page.  An entry is a pv_entry_t, the list is pv_table.
 */
typedef struct pv_entry {
	struct pv_entry	*pv_next;	/* next pv_entry */
	pmap_t		pv_pmap;	/* pmap where mapping lies */
	vm_offset_t	pv_va;		/* virtual address for mapping */
	pd_entry_t	*pv_ptpde;	/* non-zero if VA maps a PT page */
	pmap_t		pv_ptpmap;	/* if pv_ptpde, pmap for PT page */
	int		pv_flags;	/* flags */
} *pv_entry_t;

#define	PV_ENTRY_NULL	((pv_entry_t) 0)

#define	PV_CI		0x01	/* all entries must be cache inhibited */
#define PV_PTPAGE	0x02	/* entry maps a page table page */

#ifdef	KERNEL

pv_entry_t	pv_table;		/* array of entries, one per page */

#define pa_index(pa)		atop(pa - vm_first_phys)
#define pa_to_pvh(pa)		(&pv_table[pa_index(pa)])

#define	pmap_resident_count(pmap)	((pmap)->pm_stats.resident_count)

#endif	KERNEL

#endif	_PMAP_MACHINE_
