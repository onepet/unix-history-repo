/*
 * Copyright (c) 1994 Jan-Simon Pendry
 * Copyright (c) 1994
 *	The Regents of the University of California.  All rights reserved.
 *
 * This code is derived from software contributed to Berkeley by
 * Jan-Simon Pendry.
 *
 * %sccs.include.redist.c%
 *
 *	@(#)union_subr.c	2.1 (Berkeley) %G%
 */

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/time.h>
#include <sys/kernel.h>
#include <sys/vnode.h>
#include <sys/namei.h>
#include <sys/malloc.h>
#include <sys/file.h>
#include <sys/filedesc.h>
#include "union.h" /*<miscfs/union/union.h>*/

#ifdef DIAGNOSTIC
#include <sys/proc.h>
#endif

static struct union_node *unhead;
static int unvplock;

int
union_init()
{

	unhead = 0;
	unvplock = 0;
}

static void
union_remlist(un)
	struct union_node *un;
{
	struct union_node **unpp;

	for (unpp = &unhead; *unpp != 0; unpp = &(*unpp)->un_next) {
		if (*unpp == un) {
			*unpp = un->un_next;
			break;
		}
	}
}

/*
 * allocate a union_node/vnode pair.  the vnode is
 * referenced and locked.  the new vnode is returned
 * via (vpp).  (mp) is the mountpoint of the union filesystem,
 * (dvp) is the parent directory where the upper layer object
 * should exist (but doesn't) and (cnp) is the componentname
 * information which is partially copied to allow the upper
 * layer object to be created at a later time.  (uppervp)
 * and (lowervp) reference the upper and lower layer objects
 * being mapped.  either, but not both, can be nil.
 * if supplied, (uppervp) is locked.
 * the reference is either maintained in the new union_node
 * object which is allocated, or they are vrele'd.
 *
 * all union_nodes are maintained on a singly-linked
 * list.  new nodes are only allocated when they cannot
 * be found on this list.  entries on the list are
 * removed when the vfs reclaim entry is called.
 *
 * a single lock is kept for the entire list.  this is
 * needed because the getnewvnode() function can block
 * waiting for a vnode to become free, in which case there
 * may be more than one process trying to get the same
 * vnode.  this lock is only taken if we are going to
 * call getnewvnode, since the kernel itself is single-threaded.
 *
 * if an entry is found on the list, then call vget() to
 * take a reference.  this is done because there may be
 * zero references to it and so it needs to removed from
 * the vnode free list.
 */
int
union_allocvp(vpp, mp, undvp, dvp, cnp, uppervp, lowervp)
	struct vnode **vpp;
	struct mount *mp;
	struct vnode *undvp;
	struct vnode *dvp;		/* may be null */
	struct componentname *cnp;	/* may be null */
	struct vnode *uppervp;		/* may be null */
	struct vnode *lowervp;		/* may be null */
{
	int error;
	struct union_node *un;
	struct union_node **pp;
	struct vnode *xlowervp = 0;

	if (uppervp == 0 && lowervp == 0)
		panic("union: unidentifiable allocation");

	if (uppervp && lowervp && (uppervp->v_type != lowervp->v_type)) {
		xlowervp = lowervp;
		lowervp = 0;
	}

loop:
	for (un = unhead; un != 0; un = un->un_next) {
		if ((un->un_lowervp == lowervp ||
		     un->un_lowervp == 0) &&
		    (un->un_uppervp == uppervp ||
		     un->un_uppervp == 0) &&
		    (UNIONTOV(un)->v_mount == mp)) {
			if (vget(UNIONTOV(un), 0))
				goto loop;
			break;
		}
	}

	if (un) {
		/*
		 * Obtain a lock on the union_node.
		 * uppervp is locked, though un->un_uppervp
		 * may not be.  this doesn't break the locking
		 * hierarchy since in the case that un->un_uppervp
		 * is not yet locked it will be vrele'd and replaced
		 * with uppervp.
		 */

		if ((dvp != NULLVP) && (uppervp == dvp)) {
			/*
			 * Access ``.'', so (un) will already
			 * be locked.  Since this process has
			 * the lock on (uppervp) no other
			 * process can hold the lock on (un).
			 */
#ifdef DIAGNOSTIC
			if ((un->un_flags & UN_LOCKED) == 0)
				panic("union: . not locked");
			else if (curproc && un->un_pid != curproc->p_pid &&
				    un->un_pid > -1 && curproc->p_pid > -1)
				panic("union: allocvp not lock owner");
#endif
		} else {
			if (un->un_flags & UN_LOCKED) {
				vrele(UNIONTOV(un));
				un->un_flags |= UN_WANT;
				sleep((caddr_t) &un->un_flags, PINOD);
				goto loop;
			}
			un->un_flags |= UN_LOCKED;

#ifdef DIAGNOSTIC
			if (curproc)
				un->un_pid = curproc->p_pid;
			else
				un->un_pid = -1;
#endif
		}

		/*
		 * At this point, the union_node is locked,
		 * un->un_uppervp may not be locked, and uppervp
		 * is locked or nil.
		 */

		/*
		 * Save information about the upper layer.
		 */
		if (uppervp != un->un_uppervp) {
			if (un->un_uppervp)
				vrele(un->un_uppervp);
			un->un_uppervp = uppervp;
		} else if (uppervp) {
			vrele(uppervp);
		}

		if (un->un_uppervp) {
			un->un_flags |= UN_ULOCK;
			un->un_flags &= ~UN_KLOCK;
		}

		/*
		 * Save information about the lower layer.
		 * This needs to keep track of pathname
		 * and directory information which union_vn_create
		 * might need.
		 */
		if (lowervp != un->un_lowervp) {
			if (un->un_lowervp) {
				vrele(un->un_lowervp);
				free(un->un_path, M_TEMP);
				vrele(un->un_dirvp);
			}
			un->un_lowervp = lowervp;
			if (cnp && (lowervp != NULLVP) &&
			    (lowervp->v_type == VREG)) {
				un->un_hash = cnp->cn_hash;
				un->un_path = malloc(cnp->cn_namelen+1,
						M_TEMP, M_WAITOK);
				bcopy(cnp->cn_nameptr, un->un_path,
						cnp->cn_namelen);
				un->un_path[cnp->cn_namelen] = '\0';
				VREF(dvp);
				un->un_dirvp = dvp;
			}
		} else if (lowervp) {
			vrele(lowervp);
		}
		*vpp = UNIONTOV(un);
		return (0);
	}

	/*
	 * otherwise lock the vp list while we call getnewvnode
	 * since that can block.
	 */ 
	if (unvplock & UN_LOCKED) {
		unvplock |= UN_WANT;
		sleep((caddr_t) &unvplock, PINOD);
		goto loop;
	}
	unvplock |= UN_LOCKED;

	error = getnewvnode(VT_UNION, mp, union_vnodeop_p, vpp);
	if (error) {
		if (uppervp) {
			if (dvp == uppervp)
				vrele(uppervp);
			else
				vput(uppervp);
		}
		if (lowervp)
			vrele(lowervp);

		goto out;
	}

	MALLOC((*vpp)->v_data, void *, sizeof(struct union_node),
		M_TEMP, M_WAITOK);

	if (uppervp)
		(*vpp)->v_type = uppervp->v_type;
	else
		(*vpp)->v_type = lowervp->v_type;
	un = VTOUNION(*vpp);
	un->un_vnode = *vpp;
	un->un_next = 0;
	un->un_uppervp = uppervp;
	un->un_lowervp = lowervp;
	un->un_openl = 0;
	un->un_flags = UN_LOCKED;
	if (un->un_uppervp)
		un->un_flags |= UN_ULOCK;
#ifdef DIAGNOSTIC
	if (curproc)
		un->un_pid = curproc->p_pid;
	else
		un->un_pid = -1;
#endif
	if (cnp && (lowervp != NULLVP) && (lowervp->v_type == VREG)) {
		un->un_hash = cnp->cn_hash;
		un->un_path = malloc(cnp->cn_namelen+1, M_TEMP, M_WAITOK);
		bcopy(cnp->cn_nameptr, un->un_path, cnp->cn_namelen);
		un->un_path[cnp->cn_namelen] = '\0';
		VREF(dvp);
		un->un_dirvp = dvp;
	} else {
		un->un_hash = 0;
		un->un_path = 0;
		un->un_dirvp = 0;
	}

	/* add to union vnode list */
	for (pp = &unhead; *pp; pp = &(*pp)->un_next)
		continue;
	*pp = un;

	if (xlowervp)
		vrele(xlowervp);

out:
	unvplock &= ~UN_LOCKED;

	if (unvplock & UN_WANT) {
		unvplock &= ~UN_WANT;
		wakeup((caddr_t) &unvplock);
	}

	return (error);
}

int
union_freevp(vp)
	struct vnode *vp;
{
	struct union_node *un = VTOUNION(vp);

	union_remlist(un);

	FREE(vp->v_data, M_TEMP);
	vp->v_data = 0;
	return (0);
}

/*
 * copyfile.  copy the vnode (fvp) to the vnode (tvp)
 * using a sequence of reads and writes.  both (fvp)
 * and (tvp) are locked on entry and exit.
 */
int
union_copyfile(p, cred, fvp, tvp)
	struct proc *p;
	struct ucred *cred;
	struct vnode *fvp;
	struct vnode *tvp;
{
	char *buf;
	struct uio uio;
	struct iovec iov;
	int error = 0;

	/*
	 * strategy:
	 * allocate a buffer of size MAXBSIZE.
	 * loop doing reads and writes, keeping track
	 * of the current uio offset.
	 * give up at the first sign of trouble.
	 */

	uio.uio_procp = p;
	uio.uio_segflg = UIO_SYSSPACE;
	uio.uio_offset = 0;

	VOP_UNLOCK(fvp);				/* XXX */
	LEASE_CHECK(fvp, p, cred, LEASE_READ);
	VOP_LOCK(fvp);					/* XXX */
	VOP_UNLOCK(tvp);				/* XXX */
	LEASE_CHECK(tvp, p, cred, LEASE_WRITE);
	VOP_LOCK(tvp);					/* XXX */

	buf = malloc(MAXBSIZE, M_TEMP, M_WAITOK);

	/* ugly loop follows... */
	do {
		off_t offset = uio.uio_offset;

		uio.uio_iov = &iov;
		uio.uio_iovcnt = 1;
		iov.iov_base = buf;
		iov.iov_len = MAXBSIZE;
		uio.uio_resid = iov.iov_len;
		uio.uio_rw = UIO_READ;
		error = VOP_READ(fvp, &uio, 0, cred);

		if (error == 0) {
			uio.uio_iov = &iov;
			uio.uio_iovcnt = 1;
			iov.iov_base = buf;
			iov.iov_len = MAXBSIZE - uio.uio_resid;
			uio.uio_offset = offset;
			uio.uio_rw = UIO_WRITE;
			uio.uio_resid = iov.iov_len;

			if (uio.uio_resid == 0)
				break;

			do {
				error = VOP_WRITE(tvp, &uio, 0, cred);
			} while ((uio.uio_resid > 0) && (error == 0));
		}

	} while (error == 0);

	free(buf, M_TEMP);
	return (error);
}

/*
 * Create a shadow directory in the upper layer.
 * The new vnode is returned locked.
 *
 * (um) points to the union mount structure for access to the
 * the mounting process's credentials.
 * (dvp) is the directory in which to create the shadow directory.
 * it is unlocked on entry and exit.
 * (cnp) is the componentname to be created.
 * (vpp) is the returned newly created shadow directory, which
 * is returned locked.
 */
int
union_mkshadow(um, dvp, cnp, vpp)
	struct union_mount *um;
	struct vnode *dvp;
	struct componentname *cnp;
	struct vnode **vpp;
{
	int error;
	struct vattr va;
	struct proc *p = cnp->cn_proc;
	struct componentname cn;

	/*
	 * policy: when creating the shadow directory in the
	 * upper layer, create it owned by the user who did
	 * the mount, group from parent directory, and mode
	 * 777 modified by umask (ie mostly identical to the
	 * mkdir syscall).  (jsp, kb)
	 */

	/*
	 * A new componentname structure must be faked up because
	 * there is no way to know where the upper level cnp came
	 * from or what it is being used for.  This must duplicate
	 * some of the work done by NDINIT, some of the work done
	 * by namei, some of the work done by lookup and some of
	 * the work done by VOP_LOOKUP when given a CREATE flag.
	 * Conclusion: Horrible.
	 *
	 * The pathname buffer will be FREEed by VOP_MKDIR.
	 */
	cn.cn_pnbuf = malloc(cnp->cn_namelen+1, M_NAMEI, M_WAITOK);
	bcopy(cnp->cn_nameptr, cn.cn_pnbuf, cnp->cn_namelen);
	cn.cn_pnbuf[cnp->cn_namelen] = '\0';

	cn.cn_nameiop = CREATE;
	cn.cn_flags = (LOCKPARENT|HASBUF|SAVENAME|ISLASTCN);
	cn.cn_proc = cnp->cn_proc;
	cn.cn_cred = um->um_cred;
	cn.cn_nameptr = cn.cn_pnbuf;
	cn.cn_namelen = cnp->cn_namelen;
	cn.cn_hash = cnp->cn_hash;
	cn.cn_consume = cnp->cn_consume;

	VREF(dvp);
	if (error = relookup(dvp, vpp, &cn))
		return (error);
	vrele(dvp);

	if (*vpp) {
		VOP_ABORTOP(dvp, &cn);
		VOP_UNLOCK(dvp);
		vrele(*vpp);
		*vpp = NULLVP;
		return (EEXIST);
	}

	VATTR_NULL(&va);
	va.va_type = VDIR;
	va.va_mode = um->um_cmode;

	/* LEASE_CHECK: dvp is locked */
	LEASE_CHECK(dvp, p, p->p_ucred, LEASE_WRITE);

	VREF(dvp);
	error = VOP_MKDIR(dvp, vpp, &cn, &va);
	return (error);
}

/*
 * union_vn_create: creates and opens a new shadow file
 * on the upper union layer.  this function is similar
 * in spirit to calling vn_open but it avoids calling namei().
 * the problem with calling namei is that a) it locks too many
 * things, and b) it doesn't start at the "right" directory,
 * whereas relookup is told where to start.
 */
int
union_vn_create(vpp, un, p)
	struct vnode **vpp;
	struct union_node *un;
	struct proc *p;
{
	struct vnode *vp;
	struct ucred *cred = p->p_ucred;
	struct vattr vat;
	struct vattr *vap = &vat;
	int fmode = FFLAGS(O_WRONLY|O_CREAT|O_TRUNC|O_EXCL);
	int error;
	int cmode = UN_FILEMODE & ~p->p_fd->fd_cmask;
	char *cp;
	struct componentname cn;

	*vpp = NULLVP;

	/*
	 * Build a new componentname structure (for the same
	 * reasons outlines in union_mkshadow).
	 * The difference here is that the file is owned by
	 * the current user, rather than by the person who
	 * did the mount, since the current user needs to be
	 * able to write the file (that's why it is being
	 * copied in the first place).
	 */
	cn.cn_namelen = strlen(un->un_path);
	cn.cn_pnbuf = (caddr_t) malloc(cn.cn_namelen, M_NAMEI, M_WAITOK);
	bcopy(un->un_path, cn.cn_pnbuf, cn.cn_namelen+1);
	cn.cn_nameiop = CREATE;
	cn.cn_flags = (LOCKLEAF|LOCKPARENT|HASBUF|SAVENAME|ISLASTCN);
	cn.cn_proc = p;
	cn.cn_cred = p->p_ucred;
	cn.cn_nameptr = cn.cn_pnbuf;
	cn.cn_hash = un->un_hash;
	cn.cn_consume = 0;

	VREF(un->un_dirvp);
	if (error = relookup(un->un_dirvp, &vp, &cn))
		return (error);
	vrele(un->un_dirvp);

	if (vp == NULLVP) {
		/*
		 * Good - there was no race to create the file
		 * so go ahead and create it.  The permissions
		 * on the file will be 0666 modified by the
		 * current user's umask.  Access to the file, while
		 * it is unioned, will require access to the top *and*
		 * bottom files.  Access when not unioned will simply
		 * require access to the top-level file.
		 * TODO: confirm choice of access permissions.
		 */
		VATTR_NULL(vap);
		vap->va_type = VREG;
		vap->va_mode = cmode;
		LEASE_CHECK(un->un_dirvp, p, cred, LEASE_WRITE);
		if (error = VOP_CREATE(un->un_dirvp, &vp,
		    &cn, vap))
			return (error);
	} else {
		VOP_ABORTOP(un->un_dirvp, &cn);
		if (un->un_dirvp == vp)
			vrele(un->un_dirvp);
		else
			vput(vp);
		error = EEXIST;
		goto bad;
	}

	if (vp->v_type != VREG) {
		error = EOPNOTSUPP;
		goto bad;
	}

	VOP_UNLOCK(vp);				/* XXX */
	LEASE_CHECK(vp, p, cred, LEASE_WRITE);
	VOP_LOCK(vp);				/* XXX */
	VATTR_NULL(vap);
	vap->va_size = 0;
	if (error = VOP_SETATTR(vp, vap, cred, p))
		goto bad;

	if (error = VOP_OPEN(vp, fmode, cred, p))
		goto bad;

	vp->v_writecount++;
	*vpp = vp;
	return (0);
bad:
	vput(vp);
	return (error);
}

int
union_vn_close(vp, fmode, cred, p)
	struct vnode *vp;
	int fmode;
	struct ucred *cred;
	struct proc *p;
{
	if (fmode & FWRITE)
		--vp->v_writecount;
	return (VOP_CLOSE(vp, fmode));
}

void
union_removed_upper(un)
	struct union_node *un;
{
	if (un->un_flags & UN_ULOCK) {
		un->un_flags &= ~UN_ULOCK;
		vput(un->un_uppervp);
	} else {
		vrele(un->un_uppervp);
	}
	un->un_uppervp = NULLVP;
}

struct vnode *
union_lowervp(vp)
	struct vnode *vp;
{
	struct union_node *un = VTOUNION(vp);

	if (un->un_lowervp && (vp->v_type == un->un_lowervp->v_type)) {
		if (vget(un->un_lowervp, 0))
			return (NULLVP);
	}

	return (un->un_lowervp);
}
