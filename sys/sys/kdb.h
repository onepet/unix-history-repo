/*
 * Copyright (c) 2004 Marcel Moolenaar
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
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
 * $FreeBSD$
 */

#ifndef _SYS_KDB_H_
#define	_SYS_KDB_H_

#include <machine/setjmp.h>

typedef int dbbe_init_f(void);
typedef void dbbe_trace_f(void);
typedef int dbbe_trap_f(int, int);

struct kdb_dbbe {
	const char	*dbbe_name;
	dbbe_init_f	*dbbe_init;
	dbbe_trace_f	*dbbe_trace;
	dbbe_trap_f	*dbbe_trap;
	int		dbbe_active;
};

#define	KDB_BACKEND(name, init, trace, trap)		\
	static struct kdb_dbbe name##_dbbe = {		\
		.dbbe_name = #name,			\
		.dbbe_init = init,			\
		.dbbe_trace = trace,			\
		.dbbe_trap = trap			\
	};						\
	DATA_SET(kdb_dbbe_set, name##_dbbe)

struct pcb;
struct thread;
struct trapframe;

extern int kdb_active;			/* Non-zero while in debugger. */
extern struct kdb_dbbe *kdb_dbbe;	/* Default debugger backend or NULL. */
extern struct trapframe *kdb_frame;	/* Frame to kdb_trap(). */
extern struct pcb *kdb_thrctx;		/* Current context. */
extern struct thread *kdb_thread;	/* Current thread. */

int	kdb_alt_break(int, int *);
void	kdb_backtrace(void);
int	kdb_dbbe_select(const char *);
void	kdb_enter(const char *);
void	kdb_init(void);
void *	kdb_jmpbuf(jmp_buf);
void	kdb_reenter(void);
struct pcb *kdb_thr_ctx(struct thread *);
struct thread *kdb_thr_first(void);
struct thread *kdb_thr_lookup(pid_t);
struct thread *kdb_thr_next(struct thread *);
int	kdb_thr_select(struct thread *);
int	kdb_trap(int, int, struct trapframe *);

#endif /* !_SYS_KDB_H_ */
