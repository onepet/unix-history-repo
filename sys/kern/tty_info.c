/*-
 * Copyright (c) 1982, 1986, 1990, 1991, 1993
 *	The Regents of the University of California.  All rights reserved.
 * (c) UNIX System Laboratories, Inc.
 * All or some portions of this file are derived from material licensed
 * to the University of California by American Telephone and Telegraph
 * Co. or Unix System Laboratories, Inc. and are reproduced herein with
 * the permission of UNIX System Laboratories, Inc.
 *
 * Copyright (c) 2002 Networks Associates Technologies, Inc.
 * All rights reserved.
 *
 * Portions of this software were developed for the FreeBSD Project by
 * ThinkSec AS and NAI Labs, the Security Research Division of Network
 * Associates, Inc.  under DARPA/SPAWAR contract N66001-01-C-8035
 * ("CBOSS"), as part of the DARPA CHATS research program.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
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
 */

#include <sys/cdefs.h>
__FBSDID("$FreeBSD$");

#include <sys/param.h>
#include <sys/lock.h>
#include <sys/mutex.h>
#include <sys/proc.h>
#include <sys/resourcevar.h>
#include <sys/sched.h>
#include <sys/systm.h>
#include <sys/tty.h>

#include <vm/vm.h>
#include <vm/pmap.h>
#include <vm/vm_map.h>

/*
 * Returns 1 if p2 is "better" than p1
 *
 * The algorithm for picking the "interesting" process is thus:
 *
 *	1) Only foreground processes are eligible - implied.
 *	2) Runnable processes are favored over anything else.  The runner
 *	   with the highest cpu utilization is picked (p_estcpu).  Ties are
 *	   broken by picking the highest pid.
 *	3) The sleeper with the shortest sleep time is next.  With ties,
 *	   we pick out just "short-term" sleepers (P_SINTR == 0).
 *	4) Further ties are broken by picking the highest pid.
 */

#define TESTAB(a, b)    ((a)<<1 | (b))
#define ONLYA   2
#define ONLYB   1
#define BOTH    3

static int
proc_sum(struct proc *p, int *estcpup)
{
	struct thread *td;
	int estcpu;
	int val;

	val = 0;
	estcpu = 0;
	FOREACH_THREAD_IN_PROC(p, td) {
		thread_lock(td);
		if (TD_ON_RUNQ(td) ||
		    TD_IS_RUNNING(td))
			val = 1;
		estcpu += sched_pctcpu(td);
		thread_unlock(td);
	}
	*estcpup = estcpu;

	return (val);
}

static int
thread_compare(struct thread *td, struct thread *td2)
{
	int runa, runb;
	int slpa, slpb;
	fixpt_t esta, estb;

	if (td == NULL)
		return (1);

	/*
	 * Fetch running stats, pctcpu usage, and interruptable flag.
 	 */
	thread_lock(td);
	runa = TD_IS_RUNNING(td) | TD_ON_RUNQ(td);
	slpa = td->td_flags & TDF_SINTR;
	esta = sched_pctcpu(td);
	thread_unlock(td);
	thread_lock(td2);
	runb = TD_IS_RUNNING(td2) | TD_ON_RUNQ(td2);
	estb = sched_pctcpu(td2);
	slpb = td2->td_flags & TDF_SINTR;
	thread_unlock(td2);
	/*
	 * see if at least one of them is runnable
	 */
	switch (TESTAB(runa, runb)) {
	case ONLYA:
		return (0);
	case ONLYB:
		return (1);
	case BOTH:
		break;
	}
	/*
	 *  favor one with highest recent cpu utilization
	 */
	if (estb > esta)
		return (1);
	if (esta > estb)
		return (0);
	/*
	 * favor one sleeping in a non-interruptible sleep
	 */
	switch (TESTAB(slpa, slpb)) {
	case ONLYA:
		return (0);
	case ONLYB:
		return (1);
	case BOTH:
		break;
	}

	return (td < td2);
}

static int
proc_compare(struct proc *p1, struct proc *p2)
{

	int runa, runb;
	fixpt_t esta, estb;

	if (p1 == NULL)
		return (1);

	/*
	 * Fetch various stats about these processes.  After we drop the
	 * lock the information could be stale but the race is unimportant.
	 */
	PROC_LOCK(p1);
	runa = proc_sum(p1, &esta);
	PROC_UNLOCK(p1);
	PROC_LOCK(p2);
	runb = proc_sum(p2, &estb);
	PROC_UNLOCK(p2);
	
	/*
	 * see if at least one of them is runnable
	 */
	switch (TESTAB(runa, runb)) {
	case ONLYA:
		return (0);
	case ONLYB:
		return (1);
	case BOTH:
		break;
	}
	/*
	 *  favor one with highest recent cpu utilization
	 */
	if (estb > esta)
		return (1);
	if (esta > estb)
		return (0);
	/*
	 * weed out zombies
	 */
	switch (TESTAB(p1->p_state == PRS_ZOMBIE, p2->p_state == PRS_ZOMBIE)) {
	case ONLYA:
		return (1);
	case ONLYB:
		return (0);
	case BOTH:
		break;
	}

	return (p2->p_pid > p1->p_pid);		/* tie - return highest pid */
}

/*
 * Report on state of foreground process group.
 */
void
ttyinfo(struct tty *tp)
{
	struct timeval utime, stime;
	struct proc *p, *pick;
	struct thread *td, *picktd;
	const char *stateprefix, *state;
	long rss;
	int load, pctcpu;
	pid_t pid;
	char comm[MAXCOMLEN + 1];
	struct rusage ru;

	if (ttycheckoutq(tp,0) == 0)
		return;

	/* Print load average. */
	load = (averunnable.ldavg[0] * 100 + FSCALE / 2) >> FSHIFT;
	ttyprintf(tp, "load: %d.%02d ", load / 100, load % 100);

	/*
	 * On return following a ttyprintf(), we set tp->t_rocount to 0 so
	 * that pending input will be retyped on BS.
	 */
	if (tp->t_session == NULL) {
		ttyprintf(tp, "not a controlling terminal\n");
		tp->t_rocount = 0;
		return;
	}
	if (tp->t_pgrp == NULL) {
		ttyprintf(tp, "no foreground process group\n");
		tp->t_rocount = 0;
		return;
	}
	PGRP_LOCK(tp->t_pgrp);
	if (LIST_EMPTY(&tp->t_pgrp->pg_members)) {
		PGRP_UNLOCK(tp->t_pgrp);
		ttyprintf(tp, "empty foreground process group\n");
		tp->t_rocount = 0;
		return;
	}

	/*
	 * Pick the most interesting process and copy some of its
	 * state for printing later.  This operation could rely on stale
	 * data as we can't hold the proc slock or thread locks over the
	 * whole list. However, we're guaranteed not to reference an exited
	 * thread or proc since we hold the tty locked.
	 */
	pick = NULL;
	LIST_FOREACH(p, &tp->t_pgrp->pg_members, p_pglist)
		if (proc_compare(pick, p))
			pick = p;

	PROC_LOCK(pick);
	picktd = NULL;
	td = FIRST_THREAD_IN_PROC(pick);
	FOREACH_THREAD_IN_PROC(pick, td)
		if (thread_compare(picktd, td))
			picktd = td;
	td = picktd;
	stateprefix = "";
	thread_lock(td);
	if (TD_IS_RUNNING(td))
		state = "running";
	else if (TD_ON_RUNQ(td) || TD_CAN_RUN(td))
		state = "runnable";
	else if (TD_IS_SLEEPING(td)) {
		/* XXX: If we're sleeping, are we ever not in a queue? */
		if (TD_ON_SLEEPQ(td))
			state = td->td_wmesg;
		else
			state = "sleeping without queue";
	} else if (TD_ON_LOCK(td)) {
		state = td->td_lockname;
		stateprefix = "*";
	} else if (TD_IS_SUSPENDED(td))
		state = "suspended";
	else if (TD_AWAITING_INTR(td))
		state = "intrwait";
	else
		state = "unknown";
	pctcpu = (sched_pctcpu(td) * 10000 + FSCALE / 2) >> FSHIFT;
	thread_unlock(td);
	if (pick->p_state == PRS_NEW || pick->p_state == PRS_ZOMBIE)
		rss = 0;
	else
		rss = pgtok(vmspace_resident_count(pick->p_vmspace));
	PROC_UNLOCK(pick);
	PROC_LOCK(pick);
	PGRP_UNLOCK(tp->t_pgrp);
	rufetchcalc(pick, &ru, &utime, &stime);
	pid = pick->p_pid;
	bcopy(pick->p_comm, comm, sizeof(comm));
	PROC_UNLOCK(pick);

	/* Print command, pid, state, utime, stime, %cpu, and rss. */
	ttyprintf(tp,
	    " cmd: %s %d [%s%s] %ld.%02ldu %ld.%02lds %d%% %ldk\n",
	    comm, pid, stateprefix, state,
	    (long)utime.tv_sec, utime.tv_usec / 10000,
	    (long)stime.tv_sec, stime.tv_usec / 10000,
	    pctcpu / 100, rss);
	tp->t_rocount = 0;
}
