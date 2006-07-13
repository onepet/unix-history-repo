/*
 * Copyright (c) 1995 John Birrell <jb@cimlogic.com.au>.
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
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *	This product includes software developed by John Birrell.
 * 4. Neither the name of the author nor the names of any co-contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY JOHN BIRRELL AND CONTRIBUTORS ``AS IS'' AND
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
 *
 * $FreeBSD$
 */

#include "namespace.h"
#include <pthread.h>
#include "un-namespace.h"

#include "thr_private.h"

__weak_reference(_pthread_setprio, pthread_setprio);

int
_pthread_setprio(pthread_t pthread, int prio)
{
	struct pthread	*curthread = _get_curthread();
	struct sched_param	param;
	int	ret;

	param.sched_priority = prio;
	if (pthread == curthread) {
		THR_LOCK(curthread);
		if (curthread->attr.sched_policy == SCHED_OTHER ||
		    curthread->attr.prio == prio) {
			curthread->attr.prio = prio;
			ret = 0;
		} else {
			ret = thr_setschedparam(curthread->tid,
				 &param, sizeof(param));
			if (ret == -1)
				ret = errno;
			else 
				curthread->attr.prio = prio;
		}
		THR_UNLOCK(curthread);
	} else if ((ret = _thr_ref_add(curthread, pthread, /*include dead*/0))
		== 0) {
		THR_THREAD_LOCK(curthread, pthread);
		if (pthread->attr.sched_policy == SCHED_OTHER ||
		    pthread->attr.prio == prio) {
			pthread->attr.prio = prio;
			ret = 0;
		} else {
			ret = thr_setschedparam(pthread->tid, &param,
				sizeof(param));
			if (ret == -1)
				ret = errno;
			else
				pthread->attr.prio = prio;
		}
		THR_THREAD_UNLOCK(curthread, pthread);
		_thr_ref_delete(curthread, pthread);
	}
	return (ret);
}
