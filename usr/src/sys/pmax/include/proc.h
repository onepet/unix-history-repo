/*
 * Copyright (c) 1992 Regents of the University of California.
 * All rights reserved.
 *
 * This code is derived from software contributed to Berkeley by
 * Ralph Campbell.
 *
 * %sccs.include.redist.c%
 *
 *	@(#)proc.h	7.1 (Berkeley) %G%
 */

/*
 * Machine-dependent part of the proc structure for DEC Station.
 */
struct mdproc {
	int	md_flags;		/* machine-dependent flags */
	int	md_upte[UPAGES];	/* ptes for mapping u page */
};

/* md_flags */
#define	MDP_FPUSED	0x0001	/* floating point coprocessor used */
#define	MDP_ULTRIX	0x0002	/* ULTRIX process (ULTRIXCOMPAT) */
