/*-
 * Copyright (c) 1996, 1997, 1998, 2001 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Jason R. Thorpe of the Numerical Aerospace Simulation Facility,
 * NASA Ames Research Center.
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
 *	This product includes software developed by the NetBSD
 *	Foundation, Inc. and its contributors.
 * 4. Neither the name of The NetBSD Foundation nor the names of its
 *    contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE NETBSD FOUNDATION, INC. AND CONTRIBUTORS
 * ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE FOUNDATION OR CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */
/*
 * Copyright (c) 1997-1999 Eduardo E. Horvath. All rights reserved.
 * Copyright (c) 1996 Charles M. Hannum.  All rights reserved.
 * Copyright (c) 1996 Christopher G. Demetriou.  All rights reserved.
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
 *      This product includes software developed by Christopher G. Demetriou
 *	for the NetBSD Project.
 * 4. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 * 	from: NetBSD: bus.h,v 1.28 2001/07/19 15:32:19 thorpej Exp
 *	and
 *	from: FreeBSD: src/sys/alpha/include/bus.h,v 1.9 2001/01/09
 *
 * $FreeBSD$
 */

#ifndef	_MACHINE_BUS_H_
#define	_MACHINE_BUS_H_

#ifdef BUS_SPACE_DEBUG
#include <sys/ktr.h>
#endif

#include <machine/cpufunc.h>
#include <machine/upa.h>

/*
 * UPA and SBUS spaces are non-cached and big endian
 * (except for RAM and PROM)
 *
 * PCI spaces are non-cached and little endian
 */
#define	UPA_BUS_SPACE		0
#define	SBUS_BUS_SPACE		1
#define	PCI_CONFIG_BUS_SPACE	2
#define	PCI_IO_BUS_SPACE	3
#define	PCI_MEMORY_BUS_SPACE	4
#define	LAST_BUS_SPACE		5

extern int bus_type_asi[];
extern int bus_stream_asi[];

#define __BUS_SPACE_HAS_STREAM_METHODS	1

/*
 * Bus address and size types
 */
typedef	u_long		bus_space_handle_t;
typedef int		bus_type_t;
typedef u_long		bus_addr_t;
typedef u_long		bus_size_t;

#define BUS_SPACE_MAXSIZE_24BIT	0xFFFFFF
#define BUS_SPACE_MAXSIZE_32BIT 0xFFFFFFFF
#define BUS_SPACE_MAXSIZE	(128 * 1024) /* Maximum supported size */
#define BUS_SPACE_MAXADDR_24BIT	0xFFFFFF
#define BUS_SPACE_MAXADDR_32BIT 0xFFFFFFFF
#define BUS_SPACE_MAXADDR	0xFFFFFFFF

#define BUS_SPACE_UNRESTRICTED	(~0UL)

/*
 * Access methods for bus resources and address space.
 */
typedef struct bus_space_tag	*bus_space_tag_t;

struct bus_space_tag {
	void		*bst_cookie;
	bus_space_tag_t	bst_parent;
	int		bst_type;

	void		(*bst_bus_barrier)(bus_space_tag_t, bus_space_handle_t,
	    bus_size_t, bus_size_t, int);
};

/*
 * Helpers
 */
int sparc64_bus_mem_map(bus_space_tag_t, bus_space_handle_t, bus_size_t,
    int, vm_offset_t, void **);
int sparc64_bus_mem_unmap(void *, bus_size_t);
bus_space_handle_t sparc64_fake_bustag(int, bus_addr_t,
    struct bus_space_tag *);

/*
 * Bus space function prototypes.
 */
static void bus_space_barrier(bus_space_tag_t, bus_space_handle_t, bus_size_t,
    bus_size_t, int);
static int bus_space_subregion(bus_space_tag_t, bus_space_handle_t,
    bus_size_t, bus_size_t, bus_space_handle_t *);
/*
 * Unmap a region of device bus space.
 */
static __inline void bus_space_unmap(bus_space_tag_t t, bus_space_handle_t bsh,
				     bus_size_t size);

static __inline void
bus_space_unmap(bus_space_tag_t t __unused, bus_space_handle_t bsh __unused,
		bus_size_t size __unused)
{
}

/* This macro finds the first "upstream" implementation of method `f' */
#define _BS_CALL(t,f)							\
	while (t->f == NULL)						\
		t = t->bst_parent;						\
	return (*(t)->f)

static __inline void
bus_space_barrier(bus_space_tag_t t, bus_space_handle_t h, bus_size_t o,
    bus_size_t s, int f)
{
	_BS_CALL(t, bst_bus_barrier)(t, h, o, s, f);
}

static __inline int
bus_space_subregion(bus_space_tag_t t, bus_space_handle_t h, bus_size_t o,
    bus_size_t s, bus_space_handle_t *hp)
{
	*hp = h + o;
	return (0);
}

/* flags for bus space map functions */
#define BUS_SPACE_MAP_CACHEABLE		0x0001
#define BUS_SPACE_MAP_LINEAR		0x0002
#define BUS_SPACE_MAP_READONLY		0x0004
#define BUS_SPACE_MAP_PREFETCHABLE	0x0008
/* placeholders for bus functions... */
#define BUS_SPACE_MAP_BUS1		0x0100
#define BUS_SPACE_MAP_BUS2		0x0200
#define BUS_SPACE_MAP_BUS3		0x0400
#define BUS_SPACE_MAP_BUS4		0x0800

/* flags for bus_space_barrier() */
#define	BUS_SPACE_BARRIER_READ		0x01	/* force read barrier */
#define	BUS_SPACE_BARRIER_WRITE		0x02	/* force write barrier */

#ifdef BUS_SPACE_DEBUG
#define	KTR_BUS				KTR_CT2
#define	BUS_HANDLE_MIN			UPA_MEMSTART
#define	__BUS_DEBUG_ACCESS(h, o, desc, sz) do {				\
	CTR4(KTR_BUS, "bus space: %s %d: handle %#lx, offset %#lx",	\
	    (desc), (sz), (h), (o));					\
	if ((h) + (o) < BUS_HANDLE_MIN)					\
		panic("bus space access at %#lx out of range",		\
		    (h) + (o));						\
} while (0)
#else
#define	__BUS_DEBUG_ACCESS(h, o, desc, sz)
#endif

static __inline uint8_t
bus_space_read_1(bus_space_tag_t t, bus_space_handle_t h, bus_size_t o)
{

	__BUS_DEBUG_ACCESS(h, o, "read", 1);
	return (lduba_nc((caddr_t)(h + o), bus_type_asi[t->bst_type]));
}

static __inline uint16_t
bus_space_read_2(bus_space_tag_t t, bus_space_handle_t h, bus_size_t o)
{

	__BUS_DEBUG_ACCESS(h, o, "read", 2);
	return (lduha_nc((caddr_t)(h + o), bus_type_asi[t->bst_type]));
}

static __inline uint32_t
bus_space_read_4(bus_space_tag_t t, bus_space_handle_t h, bus_size_t o)
{

	__BUS_DEBUG_ACCESS(h, o, "read", 4);
	return (lduwa_nc((caddr_t)(h + o), bus_type_asi[t->bst_type]));
}

static __inline uint64_t
bus_space_read_8(bus_space_tag_t t, bus_space_handle_t h, bus_size_t o)
{

	__BUS_DEBUG_ACCESS(h, o, "read", 8);
	return (ldxa_nc((caddr_t)(h + o), bus_type_asi[t->bst_type]));
}

static __inline void
bus_space_read_multi_1(bus_space_tag_t t, bus_space_handle_t h, bus_size_t o,
    uint8_t *a, size_t c)
{

	while (c-- > 0)
		*a++ = bus_space_read_1(t, h, o);
}

static __inline void
bus_space_read_multi_2(bus_space_tag_t t, bus_space_handle_t h, bus_size_t o,
    uint16_t *a, size_t c)
{

	while (c-- > 0)
		*a++ = bus_space_read_2(t, h, o);
}

static __inline void
bus_space_read_multi_4(bus_space_tag_t t, bus_space_handle_t h, bus_size_t o,
    uint32_t *a, size_t c)
{

	while (c-- > 0)
		*a++ = bus_space_read_4(t, h, o);
}

static __inline void
bus_space_read_multi_8(bus_space_tag_t t, bus_space_handle_t h, bus_size_t o,
    uint64_t *a, size_t c)
{

	while (c-- > 0)
		*a++ = bus_space_read_8(t, h, o);
}

static __inline void
bus_space_write_1(bus_space_tag_t t, bus_space_handle_t h, bus_size_t o,
    uint8_t v)
{

	__BUS_DEBUG_ACCESS(h, o, "write", 1);
	stba_nc((caddr_t)(h + o), bus_type_asi[t->bst_type], v);
}

static __inline void
bus_space_write_2(bus_space_tag_t t, bus_space_handle_t h, bus_size_t o,
    uint16_t v)
{

	__BUS_DEBUG_ACCESS(h, o, "write", 2);
	stha_nc((caddr_t)(h + o), bus_type_asi[t->bst_type], v);
}

static __inline void
bus_space_write_4(bus_space_tag_t t, bus_space_handle_t h, bus_size_t o,
    uint32_t v)
{

	__BUS_DEBUG_ACCESS(h, o, "write", 4);
	stwa_nc((caddr_t)(h + o), bus_type_asi[t->bst_type], v);
}

static __inline void
bus_space_write_8(bus_space_tag_t t, bus_space_handle_t h, bus_size_t o,
    uint64_t v)
{

	__BUS_DEBUG_ACCESS(h, o, "write", 8);
	stxa_nc((caddr_t)(h + o), bus_type_asi[t->bst_type], v);
}

static __inline void
bus_space_write_multi_1(bus_space_tag_t t, bus_space_handle_t h, bus_size_t o,
    uint8_t *a, size_t c)
{

	while (c-- > 0)
		bus_space_write_1(t, h, o, *a++);
}

static __inline void
bus_space_write_multi_2(bus_space_tag_t t, bus_space_handle_t h, bus_size_t o,
    uint16_t *a, size_t c)
{

	while (c-- > 0)
		bus_space_write_2(t, h, o, *a++);
}

static __inline void
bus_space_write_multi_4(bus_space_tag_t t, bus_space_handle_t h, bus_size_t o,
    uint32_t *a, size_t c)
{

	while (c-- > 0)
		bus_space_write_4(t, h, o, *a++);
}

static __inline void
bus_space_write_multi_8(bus_space_tag_t t, bus_space_handle_t h, bus_size_t o,
    uint64_t *a, size_t c)
{

	while (c-- > 0)
		bus_space_write_8(t, h, o, *a++);
}

static __inline void
bus_space_set_multi_1(bus_space_tag_t t, bus_space_handle_t h, bus_size_t o,
    uint8_t v, size_t c)
{

	while (c-- > 0)
		bus_space_write_1(t, h, o, v);
}

static __inline void
bus_space_set_multi_2(bus_space_tag_t t, bus_space_handle_t h, bus_size_t o,
    uint16_t v, size_t c)
{

	while (c-- > 0)
		bus_space_write_2(t, h, o, v);
}

static __inline void
bus_space_set_multi_4(bus_space_tag_t t, bus_space_handle_t h, bus_size_t o,
    uint32_t v, size_t c)
{

	while (c-- > 0)
		bus_space_write_4(t, h, o, v);
}

static __inline void
bus_space_set_multi_8(bus_space_tag_t t, bus_space_handle_t h, bus_size_t o,
    uint64_t v, size_t c)
{

	while (c-- > 0)
		bus_space_write_8(t, h, o, v);
}

static __inline void
bus_space_read_region_1(bus_space_tag_t t, bus_space_handle_t h, bus_size_t o,
    u_int8_t *a, bus_size_t c)
{
	for (; c; a++, c--, o++)
		*a = bus_space_read_1(t, h, o);
}

static __inline void
bus_space_read_region_2(bus_space_tag_t t, bus_space_handle_t h, bus_size_t o,
    u_int16_t *a, bus_size_t c)
{
	for (; c; a++, c--, o+=2)
		*a = bus_space_read_2(t, h, o);
}

static __inline void
bus_space_read_region_4(bus_space_tag_t t, bus_space_handle_t h, bus_size_t o,
    u_int32_t *a, bus_size_t c)
{
	for (; c; a++, c--, o+=4)
		*a = bus_space_read_4(t, h, o);
}

static __inline void
bus_space_read_region_8(bus_space_tag_t t, bus_space_handle_t h, bus_size_t o,
    u_int64_t *a, bus_size_t c)
{
	for (; c; a++, c--, o+=8)
		*a = bus_space_read_8(t, h, o);
}

static __inline void
bus_space_write_region_1(bus_space_tag_t t, bus_space_handle_t h, bus_size_t o,
    const u_int8_t *a, bus_size_t c)
{
	for (; c; a++, c--, o++)
		bus_space_write_1(t, h, o, *a);
}

static __inline void
bus_space_write_region_2(bus_space_tag_t t, bus_space_handle_t h, bus_size_t o,
    const u_int16_t *a, bus_size_t c)
{
	for (; c; a++, c--, o+=2)
		bus_space_write_2(t, h, o, *a);
}

static __inline void
bus_space_write_region_4(bus_space_tag_t t, bus_space_handle_t h, bus_size_t o,
    const u_int32_t *a, bus_size_t c)
{
	for (; c; a++, c--, o+=4)
		bus_space_write_4(t, h, o, *a);
}

static __inline void
bus_space_write_region_8(bus_space_tag_t t, bus_space_handle_t h, bus_size_t o,
    const u_int64_t *a, bus_size_t c)
{
	for (; c; a++, c--, o+=8)
		bus_space_write_8(t, h, o, *a);
}

static __inline void
bus_space_set_region_1(bus_space_tag_t t, bus_space_handle_t h, bus_size_t o,
    const u_int8_t v, bus_size_t c)
{
	for (; c; c--, o++)
		bus_space_write_1(t, h, o, v);
}

static __inline void
bus_space_set_region_2(bus_space_tag_t t, bus_space_handle_t h, bus_size_t o,
    const u_int16_t v, bus_size_t c)
{
	for (; c; c--, o+=2)
		bus_space_write_2(t, h, o, v);
}

static __inline void
bus_space_set_region_4(bus_space_tag_t t, bus_space_handle_t h, bus_size_t o,
    const u_int32_t v, bus_size_t c)
{
	for (; c; c--, o+=4)
		bus_space_write_4(t, h, o, v);
}

static __inline void
bus_space_set_region_8(bus_space_tag_t t, bus_space_handle_t h, bus_size_t o,
    const u_int64_t v, bus_size_t c)
{
	for (; c; c--, o+=8)
		bus_space_write_8(t, h, o, v);
}

static __inline void
bus_space_copy_region_1(bus_space_tag_t t, bus_space_handle_t h1,
    bus_size_t o1, bus_space_handle_t h2, bus_size_t o2, bus_size_t c)
{
	for (; c; c--, o1++, o2++)
	    bus_space_write_1(t, h1, o1, bus_space_read_1(t, h2, o2));
}

static __inline void
bus_space_copy_region_2(bus_space_tag_t t, bus_space_handle_t h1,
    bus_size_t o1, bus_space_handle_t h2, bus_size_t o2, bus_size_t c)
{
	for (; c; c--, o1+=2, o2+=2)
	    bus_space_write_2(t, h1, o1, bus_space_read_2(t, h2, o2));
}

static __inline void
bus_space_copy_region_4(bus_space_tag_t t, bus_space_handle_t h1,
    bus_size_t o1, bus_space_handle_t h2, bus_size_t o2, bus_size_t c)
{
	for (; c; c--, o1+=4, o2+=4)
	    bus_space_write_4(t, h1, o1, bus_space_read_4(t, h2, o2));
}

static __inline void
bus_space_copy_region_8(bus_space_tag_t t, bus_space_handle_t h1,
    bus_size_t o1, bus_space_handle_t h2, bus_size_t o2, bus_size_t c)
{
	for (; c; c--, o1+=8, o2+=8)
	    bus_space_write_8(t, h1, o1, bus_space_read_8(t, h2, o2));
}

static __inline uint8_t
bus_space_read_stream_1(bus_space_tag_t t, bus_space_handle_t h, bus_size_t o)
{

	__BUS_DEBUG_ACCESS(h, o, "read stream", 1);
	return (lduba_nc((caddr_t)(h + o), bus_stream_asi[t->bst_type]));
}

static __inline uint16_t
bus_space_read_stream_2(bus_space_tag_t t, bus_space_handle_t h, bus_size_t o)
{

	__BUS_DEBUG_ACCESS(h, o, "read stream", 2);
	return (lduha_nc((caddr_t)(h + o), bus_stream_asi[t->bst_type]));
}

static __inline uint32_t
bus_space_read_stream_4(bus_space_tag_t t, bus_space_handle_t h, bus_size_t o)
{

	__BUS_DEBUG_ACCESS(h, o, "read stream", 4);
	return (lduwa_nc((caddr_t)(h + o), bus_stream_asi[t->bst_type]));
}

static __inline uint64_t
bus_space_read_stream_8(bus_space_tag_t t, bus_space_handle_t h, bus_size_t o)
{

	__BUS_DEBUG_ACCESS(h, o, "read stream", 8);
	return (ldxa_nc((caddr_t)(h + o), bus_stream_asi[t->bst_type]));
}

static __inline void
bus_space_read_multi_stream_1(bus_space_tag_t t, bus_space_handle_t h,
    bus_size_t o, uint8_t *a, size_t c)
{

	while (c-- > 0)
		*a++ = bus_space_read_stream_1(t, h, o);
}

static __inline void
bus_space_read_multi_stream_2(bus_space_tag_t t, bus_space_handle_t h,
    bus_size_t o, uint16_t *a, size_t c)
{

	while (c-- > 0)
		*a++ = bus_space_read_stream_2(t, h, o);
}

static __inline void
bus_space_read_multi_stream_4(bus_space_tag_t t, bus_space_handle_t h,
    bus_size_t o, uint32_t *a, size_t c)
{

	while (c-- > 0)
		*a++ = bus_space_read_stream_4(t, h, o);
}

static __inline void
bus_space_read_multi_stream_8(bus_space_tag_t t, bus_space_handle_t h,
    bus_size_t o, uint64_t *a, size_t c)
{

	while (c-- > 0)
		*a++ = bus_space_read_stream_8(t, h, o);
}

static __inline void
bus_space_write_stream_1(bus_space_tag_t t, bus_space_handle_t h, bus_size_t o,
    uint8_t v)
{

	__BUS_DEBUG_ACCESS(h, o, "write stream", 1);
	stba_nc((caddr_t)(h + o), bus_stream_asi[t->bst_type], v);
}

static __inline void
bus_space_write_stream_2(bus_space_tag_t t, bus_space_handle_t h, bus_size_t o,
    uint16_t v)
{

	__BUS_DEBUG_ACCESS(h, o, "write stream", 2);
	stha_nc((caddr_t)(h + o), bus_stream_asi[t->bst_type], v);
}

static __inline void
bus_space_write_stream_4(bus_space_tag_t t, bus_space_handle_t h, bus_size_t o,
    uint32_t v)
{

	__BUS_DEBUG_ACCESS(h, o, "write stream", 4);
	stwa_nc((caddr_t)(h + o), bus_stream_asi[t->bst_type], v);
}

static __inline void
bus_space_write_stream_8(bus_space_tag_t t, bus_space_handle_t h, bus_size_t o,
    uint64_t v)
{

	__BUS_DEBUG_ACCESS(h, o, "write stream", 8);
	stxa_nc((caddr_t)(h + o), bus_stream_asi[t->bst_type], v);
}

static __inline void
bus_space_write_multi_stream_1(bus_space_tag_t t, bus_space_handle_t h,
    bus_size_t o, uint8_t *a, size_t c)
{

	while (c-- > 0)
		bus_space_write_stream_1(t, h, o, *a++);
}

static __inline void
bus_space_write_multi_stream_2(bus_space_tag_t t, bus_space_handle_t h,
    bus_size_t o, uint16_t *a, size_t c)
{

	while (c-- > 0)
		bus_space_write_stream_2(t, h, o, *a++);
}

static __inline void
bus_space_write_multi_stream_4(bus_space_tag_t t, bus_space_handle_t h,
    bus_size_t o, uint32_t *a, size_t c)
{

	while (c-- > 0)
		bus_space_write_stream_4(t, h, o, *a++);
}

static __inline void
bus_space_write_multi_stream_8(bus_space_tag_t t, bus_space_handle_t h,
    bus_size_t o, uint64_t *a, size_t c)
{

	while (c-- > 0)
		bus_space_write_stream_8(t, h, o, *a++);
}

static __inline void
bus_space_set_multi_stream_1(bus_space_tag_t t, bus_space_handle_t h,
    bus_size_t o, uint8_t v, size_t c)
{

	while (c-- > 0)
		bus_space_write_stream_1(t, h, o, v);
}

static __inline void
bus_space_set_multi_stream_2(bus_space_tag_t t, bus_space_handle_t h,
    bus_size_t o, uint16_t v, size_t c)
{

	while (c-- > 0)
		bus_space_write_stream_2(t, h, o, v);
}

static __inline void
bus_space_set_multi_stream_4(bus_space_tag_t t, bus_space_handle_t h,
    bus_size_t o, uint32_t v, size_t c)
{

	while (c-- > 0)
		bus_space_write_stream_4(t, h, o, v);
}

static __inline void
bus_space_set_multi_stream_8(bus_space_tag_t t, bus_space_handle_t h,
    bus_size_t o, uint64_t v, size_t c)
{

	while (c-- > 0)
		bus_space_write_stream_8(t, h, o, v);
}

static __inline void
bus_space_read_region_stream_1(bus_space_tag_t t, bus_space_handle_t h,
    bus_size_t o, u_int8_t *a, bus_size_t c)
{
	for (; c; a++, c--, o++)
		*a = bus_space_read_stream_1(t, h, o);
}

static __inline void
bus_space_read_region_stream_2(bus_space_tag_t t, bus_space_handle_t h,
    bus_size_t o, u_int16_t *a, bus_size_t c)
{
	for (; c; a++, c--, o+=2)
		*a = bus_space_read_stream_2(t, h, o);
}

static __inline void
bus_space_read_region_stream_4(bus_space_tag_t t, bus_space_handle_t h,
    bus_size_t o, u_int32_t *a, bus_size_t c)
{
	for (; c; a++, c--, o+=4)
		*a = bus_space_read_stream_4(t, h, o);
}

static __inline void
bus_space_read_region_stream_8(bus_space_tag_t t, bus_space_handle_t h,
    bus_size_t o, u_int64_t *a, bus_size_t c)
{
	for (; c; a++, c--, o+=8)
		*a = bus_space_read_stream_8(t, h, o);
}

static __inline void
bus_space_write_region_stream_1(bus_space_tag_t t, bus_space_handle_t h,
    bus_size_t o, const u_int8_t *a, bus_size_t c)
{
	for (; c; a++, c--, o++)
		bus_space_write_stream_1(t, h, o, *a);
}

static __inline void
bus_space_write_region_stream_2(bus_space_tag_t t, bus_space_handle_t h,
    bus_size_t o, const u_int16_t *a, bus_size_t c)
{
	for (; c; a++, c--, o+=2)
		bus_space_write_stream_2(t, h, o, *a);
}

static __inline void
bus_space_write_region_stream_4(bus_space_tag_t t, bus_space_handle_t h,
    bus_size_t o, const u_int32_t *a, bus_size_t c)
{
	for (; c; a++, c--, o+=4)
		bus_space_write_stream_4(t, h, o, *a);
}

static __inline void
bus_space_write_region_stream_8(bus_space_tag_t t, bus_space_handle_t h,
    bus_size_t o, const u_int64_t *a, bus_size_t c)
{
	for (; c; a++, c--, o+=8)
		bus_space_write_stream_8(t, h, o, *a);
}

static __inline void
bus_space_set_region_stream_1(bus_space_tag_t t, bus_space_handle_t h,
    bus_size_t o, const u_int8_t v, bus_size_t c)
{
	for (; c; c--, o++)
		bus_space_write_stream_1(t, h, o, v);
}

static __inline void
bus_space_set_region_stream_2(bus_space_tag_t t, bus_space_handle_t h,
    bus_size_t o, const u_int16_t v, bus_size_t c)
{
	for (; c; c--, o+=2)
		bus_space_write_stream_2(t, h, o, v);
}

static __inline void
bus_space_set_region_stream_4(bus_space_tag_t t, bus_space_handle_t h,
    bus_size_t o, const u_int32_t v, bus_size_t c)
{
	for (; c; c--, o+=4)
		bus_space_write_stream_4(t, h, o, v);
}

static __inline void
bus_space_set_region_stream_8(bus_space_tag_t t, bus_space_handle_t h,
    bus_size_t o, const u_int64_t v, bus_size_t c)
{
	for (; c; c--, o+=8)
		bus_space_write_stream_8(t, h, o, v);
}

static __inline void
bus_space_copy_region_stream_1(bus_space_tag_t t, bus_space_handle_t h1,
    bus_size_t o1, bus_space_handle_t h2, bus_size_t o2, bus_size_t c)
{
	for (; c; c--, o1++, o2++)
	    bus_space_write_stream_1(t, h1, o1, bus_space_read_stream_1(t, h2,
		o2));
}

static __inline void
bus_space_copy_region_stream_2(bus_space_tag_t t, bus_space_handle_t h1,
    bus_size_t o1, bus_space_handle_t h2, bus_size_t o2, bus_size_t c)
{
	for (; c; c--, o1+=2, o2+=2)
	    bus_space_write_stream_2(t, h1, o1, bus_space_read_stream_2(t, h2,
		o2));
}

static __inline void
bus_space_copy_region_stream_4(bus_space_tag_t t, bus_space_handle_t h1,
    bus_size_t o1, bus_space_handle_t h2, bus_size_t o2, bus_size_t c)
{
	for (; c; c--, o1+=4, o2+=4)
	    bus_space_write_stream_4(t, h1, o1, bus_space_read_stream_4(t, h2,
		o2));
}

static __inline void
bus_space_copy_region_stream_8(bus_space_tag_t t, bus_space_handle_t h1,
    bus_size_t o1, bus_space_handle_t h2, bus_size_t o2, bus_size_t c)
{
	for (; c; c--, o1+=8, o2+=8)
	    bus_space_write_stream_8(t, h1, o1, bus_space_read_8(t, h2, o2));
}

/* Back-compat functions for old ISA drivers */
extern bus_space_tag_t isa_io_bt;
extern bus_space_handle_t isa_io_hdl;
extern bus_space_tag_t isa_mem_bt;
extern bus_space_handle_t isa_mem_hdl;

#define inb(o)		bus_space_read_1(isa_io_bt, isa_io_hdl, o)
#define inw(o)		bus_space_read_2(isa_io_bt, isa_io_hdl, o)
#define inl(o)		bus_space_read_4(isa_io_bt, isa_io_hdl, o)
#define outb(o, v)	bus_space_write_1(isa_io_bt, isa_io_hdl, o, v)
#define outw(o, v)	bus_space_write_2(isa_io_bt, isa_io_hdl, o, v)
#define outl(o, v)	bus_space_write_4(isa_io_bt, isa_io_hdl, o, v)

#define readb(o)	bus_space_read_1(isa_mem_bt, isa_mem_hdl, o)
#define readw(o)	bus_space_read_2(isa_mem_bt, isa_mem_hdl, o)
#define readl(o)	bus_space_read_4(isa_mem_bt, isa_mem_hdl, o)
#define writeb(o, v)	bus_space_write_1(isa_mem_bt, isa_mem_hdl, o, v)
#define writew(o, v)	bus_space_write_2(isa_mem_bt, isa_mem_hdl, o, v)
#define writel(o, v)	bus_space_write_4(isa_mem_bt, isa_mem_hdl, o, v)

#define insb(o, a, c) \
	bus_space_read_multi_1(isa_io_bt, isa_io_hdl, o, (void*)a, c)
#define insw(o, a, c) \
	bus_space_read_multi_2(isa_io_bt, isa_io_hdl, o, (void*)a, c)
#define insl(o, a, c) \
	bus_space_read_multi_4(isa_io_bt, isa_io_hdl, o, (void*)a, c)
#define outsb(o, a, c) \
	bus_space_write_multi_1(isa_io_bt, isa_io_hdl, o, (void*)a, c)
#define outsw(o, a, c) \
	bus_space_write_multi_2(isa_io_bt, isa_io_hdl, o, (void*)a, c)
#define outsl(o, a, c) \
	bus_space_write_multi_4(isa_io_bt, isa_io_hdl, o, (void*)a, c)

#define memcpy_fromio(d, s, c) \
	bus_space_read_region_1(isa_mem_bt, isa_mem_hdl, s, d, c)
#define memcpy_toio(d, s, c) \
	bus_space_write_region_1(isa_mem_bt, isa_mem_hdl, d, s, c)
#define memcpy_io(d, s, c) \
	bus_space_copy_region_1(isa_mem_bt, isa_mem_hdl, s, isa_mem_hdl, d, c)
#define memset_io(d, v, c) \
	bus_space_set_region_1(isa_mem_bt, isa_mem_hdl, d, v, c)
#define memsetw_io(d, v, c) \
	bus_space_set_region_2(isa_mem_bt, isa_mem_hdl, d, v, c)

static __inline void
memsetw(void *d, int val, size_t size)
{
    u_int16_t *sp = d;

    while (size--)
	*sp++ = val;
}

/* DMA support */

/*
 * Flags used in various bus DMA methods.
 */
#define	BUS_DMA_WAITOK		0x000	/* safe to sleep (pseudo-flag) */
#define	BUS_DMA_NOWAIT		0x001	/* not safe to sleep */
#define	BUS_DMA_ALLOCNOW	0x002	/* perform resource allocation now */
#define	BUS_DMAMEM_NOSYNC	0x004	/* map memory to not require sync */
#define	BUS_DMA_NOWRITE		0x008
#define	BUS_DMA_BUS1		0x010
#define	BUS_DMA_BUS2		0x020
#define	BUS_DMA_BUS3		0x040
#define	BUS_DMA_BUS4		0x080
/*
 * The following flags are from NetBSD, but are not implemented for all
 * architetures, and should therefore not be used in MI code.
 * Some have different values than under NetBSD.
 */
#define	BUS_DMA_STREAMING	0x100	/* hint: sequential, unidirectional */
#define	BUS_DMA_READ		0x200	/* mapping is device -> memory only */
#define	BUS_DMA_WRITE		0x400	/* mapping is memory -> device only */
#define	BUS_DMA_COHERENT	0x800	/* hint: map memory DMA coherent */

#define	BUS_DMA_NOCACHE		BUS_DMA_BUS1
/* Don't bother with alignment */
#define	BUS_DMA_DVMA		BUS_DMA_BUS2

/* Forwards needed by prototypes below. */
struct mbuf;
struct uio;

typedef enum {
	BUS_DMASYNC_PREREAD,
	BUS_DMASYNC_POSTREAD,
	BUS_DMASYNC_PREWRITE,
	BUS_DMASYNC_POSTWRITE,
} bus_dmasync_op_t;

/*
 * A function that returns 1 if the address cannot be accessed by
 * a device and 0 if it can be.
 */
typedef int bus_dma_filter_t(void *, bus_addr_t);

typedef struct bus_dma_tag	*bus_dma_tag_t;
typedef struct bus_dmamap	*bus_dmamap_t;

struct bus_dma_segment {
	bus_addr_t	ds_addr;	/* DVMA address */
	bus_size_t	ds_len;		/* length of transfer */
};
typedef struct bus_dma_segment	bus_dma_segment_t;

/*
 * A function that processes a successfully loaded dma map or an error
 * from a delayed load map.
 */
typedef void bus_dmamap_callback_t(void *, bus_dma_segment_t *, int, int);

/*
 * Like bus_dmamap_callback but includes map size in bytes.  This is
 * defined as a separate interface to maintain compatiiblity for users
 * of bus_dmamap_callback_t--at some point these interfaces should be merged.
 */
typedef void bus_dmamap_callback2_t(void *, bus_dma_segment_t *, int, bus_size_t, int);

/*
 *	bus_dma_tag_t
 *
 *	A machine-dependent opaque type describing the implementation of
 *	DMA for a given bus.
 */
struct bus_dma_tag {
	void		*dt_cookie;		/* cookie used in the guts */
	bus_dma_tag_t	dt_parent;
	bus_size_t	dt_alignment;
	bus_size_t	dt_boundary;
	bus_addr_t	dt_lowaddr;
	bus_addr_t	dt_highaddr;
	bus_dma_filter_t	*dt_filter;
	void		*dt_filterarg;
	bus_size_t	dt_maxsize;
	int		dt_nsegments;
	bus_size_t	dt_maxsegsz;
	int		dt_flags;
	int		dt_ref_count;
	int		dt_map_count;

	/*
	 * DMA mapping methods.
	 */
	int	(*dt_dmamap_create)(bus_dma_tag_t, bus_dma_tag_t, int,
	    bus_dmamap_t *);
	int	(*dt_dmamap_destroy)(bus_dma_tag_t, bus_dma_tag_t,
	    bus_dmamap_t);
	int	(*dt_dmamap_load)(bus_dma_tag_t, bus_dma_tag_t, bus_dmamap_t,
	    void *, bus_size_t, bus_dmamap_callback_t *, void *, int);
	int	(*dt_dmamap_load_mbuf)(bus_dma_tag_t, bus_dma_tag_t,
	    bus_dmamap_t, struct mbuf *, bus_dmamap_callback2_t *, void *, int);
	int	(*dt_dmamap_load_uio)(bus_dma_tag_t, bus_dma_tag_t,
	    bus_dmamap_t, struct uio *, bus_dmamap_callback2_t *, void *, int);
	void	(*dt_dmamap_unload)(bus_dma_tag_t, bus_dma_tag_t, bus_dmamap_t);
	void	(*dt_dmamap_sync)(bus_dma_tag_t, bus_dma_tag_t, bus_dmamap_t,
	    bus_dmasync_op_t);

	/*
	 * DMA memory utility functions.
	 */
	int	(*dt_dmamem_alloc_size)(bus_dma_tag_t, bus_dma_tag_t, void **,
	    int, bus_dmamap_t *, bus_size_t size);
	int	(*dt_dmamem_alloc)(bus_dma_tag_t, bus_dma_tag_t, void **, int,
	    bus_dmamap_t *);
	void	(*dt_dmamem_free_size)(bus_dma_tag_t, bus_dma_tag_t, void *,
	    bus_dmamap_t, bus_size_t size);
	void	(*dt_dmamem_free)(bus_dma_tag_t, bus_dma_tag_t, void *,
	    bus_dmamap_t);
};

/*
 * XXX: This is a kluge. It would be better to handle dma tags in a hierarchical
 * way, and have a BUS_GET_DMA_TAG(); however, since this is not currently the
 * case, save a root tag in the relevant bus attach function and use that.
 * Keep the hierarchical structure, it might become needed in the future.
 */
extern bus_dma_tag_t sparc64_root_dma_tag;

int bus_dma_tag_create(bus_dma_tag_t, bus_size_t, bus_size_t, bus_addr_t,
    bus_addr_t, bus_dma_filter_t *, void *, bus_size_t, int, bus_size_t,
    int, bus_dma_tag_t *);

int bus_dma_tag_destroy(bus_dma_tag_t);

int sparc64_dmamem_alloc_map(bus_dma_tag_t dmat, bus_dmamap_t *mapp);
void sparc64_dmamem_free_map(bus_dma_tag_t dmat, bus_dmamap_t map);

static __inline int
sparc64_dmamap_create(bus_dma_tag_t pt, bus_dma_tag_t dt, int f,
    bus_dmamap_t *p)
{
	bus_dma_tag_t lt;

	for (lt = pt; lt->dt_dmamap_create == NULL; lt = lt->dt_parent)
		;
	return ((*lt->dt_dmamap_create)(lt, dt, f, p));
}
#define	bus_dmamap_create(t, f, p)					\
	sparc64_dmamap_create((t), (t), (f), (p))

static __inline int
sparc64_dmamap_destroy(bus_dma_tag_t pt, bus_dma_tag_t dt, bus_dmamap_t p)
{
	bus_dma_tag_t lt;

	for (lt = pt; lt->dt_dmamap_destroy == NULL; lt = lt->dt_parent)
		;
	return ((*lt->dt_dmamap_destroy)(lt, dt, p));
}
#define	bus_dmamap_destroy(t, p)					\
	sparc64_dmamap_destroy((t), (t), (p))

static __inline int
sparc64_dmamap_load(bus_dma_tag_t pt, bus_dma_tag_t dt, bus_dmamap_t m,
    void *p, bus_size_t s, bus_dmamap_callback_t *cb, void *cba, int f)
{
	bus_dma_tag_t lt;

	for (lt = pt; lt->dt_dmamap_load == NULL; lt = lt->dt_parent)
		;
	return ((*lt->dt_dmamap_load)(lt, dt, m, p, s, cb, cba, f));
}
#define	bus_dmamap_load(t, m, p, s, cb, cba, f)				\
	sparc64_dmamap_load((t), (t), (m), (p), (s), (cb), (cba), (f))

static __inline int
sparc64_dmamap_load_mbuf(bus_dma_tag_t pt, bus_dma_tag_t dt, bus_dmamap_t m,
    struct mbuf *mb, bus_dmamap_callback2_t *cb, void *cba, int f)
{
	bus_dma_tag_t lt;

	for (lt = pt; lt->dt_dmamap_load_mbuf == NULL; lt = lt->dt_parent)
		;
	return ((*lt->dt_dmamap_load_mbuf)(lt, dt, m, mb, cb, cba, f));
}
#define	bus_dmamap_load_mbuf(t, m, mb, cb, cba, f)				\
	sparc64_dmamap_load_mbuf((t), (t), (m), (mb), (cb), (cba), (f))

static __inline int
sparc64_dmamap_load_uio(bus_dma_tag_t pt, bus_dma_tag_t dt, bus_dmamap_t m,
    struct uio *ui, bus_dmamap_callback2_t *cb, void *cba, int f)
{
	bus_dma_tag_t lt;

	for (lt = pt; lt->dt_dmamap_load_uio == NULL; lt = lt->dt_parent)
		;
	return ((*lt->dt_dmamap_load_uio)(lt, dt, m, ui, cb, cba, f));
}
#define	bus_dmamap_load_uio(t, m, ui, cb, cba, f)				\
	sparc64_dmamap_load_uio((t), (t), (m), (ui), (cb), (cba), (f))

static __inline void
sparc64_dmamap_unload(bus_dma_tag_t pt, bus_dma_tag_t dt, bus_dmamap_t p)
{
	bus_dma_tag_t lt;

	for (lt = pt; lt->dt_dmamap_unload == NULL; lt = lt->dt_parent)
		;
	(*lt->dt_dmamap_unload)(lt, dt, p);
}
#define	bus_dmamap_unload(t, p)						\
	sparc64_dmamap_unload((t), (t), (p))

static __inline void
sparc64_dmamap_sync(bus_dma_tag_t pt, bus_dma_tag_t dt, bus_dmamap_t m,
    bus_dmasync_op_t op)
{
	bus_dma_tag_t lt;

	for (lt = pt; lt->dt_dmamap_sync == NULL; lt = lt->dt_parent)
		;
	(*lt->dt_dmamap_sync)(lt, dt, m, op);
}
#define	bus_dmamap_sync(t, m, op)					\
	sparc64_dmamap_sync((t), (t), (m), (op))

static __inline int
sparc64_dmamem_alloc_size(bus_dma_tag_t pt, bus_dma_tag_t dt, void **v, int f,
    bus_dmamap_t *m, bus_size_t s)
{
	bus_dma_tag_t lt;

	for (lt = pt; lt->dt_dmamem_alloc_size == NULL; lt = lt->dt_parent)
		;
	return ((*lt->dt_dmamem_alloc_size)(lt, dt, v, f, m, s));
}
#define	bus_dmamem_alloc_size(t, v, f, m, s)				\
	sparc64_dmamem_alloc_size((t), (t), (v), (f), (m), (s))

static __inline int
sparc64_dmamem_alloc(bus_dma_tag_t pt, bus_dma_tag_t dt, void **v, int f,
    bus_dmamap_t *m)
{
	bus_dma_tag_t lt;

	for (lt = pt; lt->dt_dmamem_alloc == NULL; lt = lt->dt_parent)
		;
	return ((*lt->dt_dmamem_alloc)(lt, dt, v, f, m));
}
#define	bus_dmamem_alloc(t, v, f, m)					\
	sparc64_dmamem_alloc((t), (t), (v), (f), (m))

static __inline void
sparc64_dmamem_free_size(bus_dma_tag_t pt, bus_dma_tag_t dt, void *v,
    bus_dmamap_t m, bus_size_t s)
{
	bus_dma_tag_t lt;

	for (lt = pt; lt->dt_dmamem_free_size == NULL; lt = lt->dt_parent)
		;
	(*lt->dt_dmamem_free_size)(lt, dt, v, m, s);
}
#define	bus_dmamem_free_size(t, v, m, s)				\
	sparc64_dmamem_free_size((t), (t), (v), (m), (s))

static __inline void
sparc64_dmamem_free(bus_dma_tag_t pt, bus_dma_tag_t dt, void *v,
    bus_dmamap_t m)
{
	bus_dma_tag_t lt;

	for (lt = pt; lt->dt_dmamem_free == NULL; lt = lt->dt_parent)
		;
	(*lt->dt_dmamem_free)(lt, dt, v, m);
}
#define	bus_dmamem_free(t, v, m)					\
	sparc64_dmamem_free((t), (t), (v), (m))

#endif /* !_MACHINE_BUS_H_ */
