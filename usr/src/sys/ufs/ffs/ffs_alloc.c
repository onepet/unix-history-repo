/* Copyright (c) 1981 Regents of the University of California */

static char vers[] = "@(#)ffs_alloc.c 1.4 %G%";

/*	alloc.c	4.8	81/03/08	*/

#include "../h/param.h"
#include "../h/systm.h"
#include "../h/mount.h"
#include "../h/fs.h"
#include "../h/conf.h"
#include "../h/buf.h"
#include "../h/inode.h"
#include "../h/dir.h"
#include "../h/user.h"

long	hashalloc();
long	alloccg();
long	ialloccg();

struct buf *
alloc(dev, ip, bpref, size)
	dev_t dev;
	register struct inode *ip;
	daddr_t bpref;
	int size;
{
	daddr_t bno;
	register struct fs *fs;
	register struct buf *bp;
	int cg;
	
	if ((unsigned)size > BSIZE || size % FSIZE != 0)
		panic("alloc: bad size");
	fs = getfs(dev);
	if (fs->fs_nbfree == 0 && size == BSIZE)
		goto nospace;
	if (bpref == 0)
		cg = itog(ip->i_number, fs);
	else
		cg = dtog(bpref, fs);
	bno = hashalloc(dev, fs, cg, (long)bpref, size, alloccg);
	if (bno == 0)
		goto nospace;
	bp = getblk(dev, bno, size);
	clrbuf(bp);
	return (bp);
nospace:
	fserr(fs, "file system full");
	uprintf("\n%s: write failed, file system is full\n", fs->fs_fsmnt);
	u.u_error = ENOSPC;
	return (NULL);
}

struct buf *
realloccg(dev, ip, bprev, osize, nsize)
	dev_t dev;
	register struct inode *ip;
	daddr_t bprev;
	int osize, nsize;
{
	daddr_t bno;
	register struct fs *fs;
	register struct buf *bp, *obp;
	caddr_t cp;
	int cg;
	
	if ((unsigned)osize > BSIZE || osize % FSIZE != 0 ||
	    (unsigned)nsize > BSIZE || nsize % FSIZE != 0)
		panic("realloccg: bad size");
	fs = getfs(dev);
	if (bprev == 0)
		panic("realloccg: bad bprev");
	else
		cg = dtog(bprev, fs);
	bno = fragextend(dev, fs, cg, (long)bprev, osize, nsize);
	if (bno != 0) {
		bp = bread(dev, bno, osize);
		bp->b_bcount = nsize;
		blkclr(bp->b_un.b_addr + osize, nsize - osize);
		return (bp);
	}
	bno = hashalloc(dev, fs, cg, (long)bprev, nsize, alloccg);
	if (bno != 0) {
		/*
		 * make a new copy
		 */
		obp = bread(dev, bprev, osize);
		bp = getblk(dev, bno, nsize);
		cp = bp->b_un.b_addr;
		bp->b_un.b_addr = obp->b_un.b_addr;
		obp->b_un.b_addr = cp;
		obp->b_flags |= B_INVAL;
		brelse(obp);
		fre(dev, bprev, osize);
		blkclr(bp->b_un.b_addr + osize, nsize - osize);
		return(bp);
	}
	/*
	 * no space available
	 */
	fserr(fs, "file system full");
	uprintf("\n%s: write failed, file system is full\n", fs->fs_fsmnt);
	u.u_error = ENOSPC;
	return (NULL);
}

struct inode *
ialloc(dev, ipref, mode)
	dev_t dev;
	ino_t ipref;
	int mode;
{
	daddr_t ino;
	register struct fs *fs;
	register struct inode *ip;
	int cg;
	
	fs = getfs(dev);
	if (fs->fs_nifree == 0)
		goto noinodes;
	cg = itog(ipref, fs);
	ino = hashalloc(dev, fs, cg, (long)ipref, mode, ialloccg);
	if (ino == 0)
		goto noinodes;
	ip = iget(dev, ino);
	if (ip == NULL) {
		ifree(dev, ino);
		return (NULL);
	}
	if (ip->i_mode)
		panic("ialloc: dup alloc");
	return (ip);
noinodes:
	fserr(fs, "out of inodes");
	uprintf("\n%s: create failed, no inodes free\n", fs->fs_fsmnt);
	u.u_error = ENOSPC;
	return (NULL);
}

dipref(dev)
	dev_t dev;
{
	register struct fs *fs;
	int cg, minndir, mincg;

	fs = getfs(dev);
	minndir = fs->fs_cs[0].cs_ndir;
	mincg = 0;
	for (cg = 1; cg < fs->fs_ncg; cg++)
		if (fs->fs_cs[cg].cs_ndir < minndir) {
			mincg = cg;
			minndir = fs->fs_cs[cg].cs_ndir;
			if (minndir == 0)
				break;
		}
	return (fs->fs_ipg * mincg);
}

long
hashalloc(dev, fs, cg, pref, size, allocator)
	dev_t dev;
	register struct fs *fs;
	int cg;
	long pref;
	int size;	/* size for data blocks, mode for inodes */
	long (*allocator)();
{
	long result;
	int i, icg = cg;

	/*
	 * 1: preferred cylinder group
	 */
	result = (*allocator)(dev, fs, cg, pref, size);
	if (result)
		return (result);
	/*
	 * 2: quadratic rehash
	 */
	for (i = 1; i < fs->fs_ncg; i *= 2) {
		cg += i;
		if (cg >= fs->fs_ncg)
			cg -= fs->fs_ncg;
		result = (*allocator)(dev, fs, cg, 0, size);
		if (result)
			return (result);
	}
	/*
	 * 3: brute force search
	 */
	cg = icg;
	for (i = 0; i < fs->fs_ncg; i++) {
		result = (*allocator)(dev, fs, cg, 0, size);
		if (result)
			return (result);
		cg++;
		if (cg == fs->fs_ncg)
			cg = 0;
	}
	return (0);
}

daddr_t
fragextend(dev, fs, cg, bprev, osize, nsize)
	dev_t dev;
	register struct fs *fs;
	int cg;
	long bprev;
	int osize, nsize;
{
	register struct buf *bp;
	register struct cg *cgp;
	long bno;
	int frags, bbase;
	int i;

	frags = nsize / FSIZE;
	bbase = bprev % FRAG;
	if (bbase > (bprev + frags - 1) % FRAG) {
		/* cannot extend across a block boundry */
		return (0);
	}
	bp = bread(dev, cgtod(cg, fs), BSIZE);
	if (bp->b_flags & B_ERROR)
		return (0);
	cgp = bp->b_un.b_cg;
	bno = bprev % fs->fs_fpg;
	for (i = osize / FSIZE; i < frags; i++) {
		if (isclr(cgp->cg_free, bno + i))
			break;
	}
	if (i == frags) {
		/*
		 * the current fragment can be extended
		 * deduct the count on fragment being extended into
		 * increase the count on the remaining fragment (if any)
		 * allocate the extended piece
		 */
		for (i = frags; i < FRAG - bbase; i++)
			if (isclr(cgp->cg_free, bno + i))
				break;
		cgp->cg_frsum[i - osize / FSIZE]--;
		if (i != frags)
			cgp->cg_frsum[i - frags]++;
		for (i = osize / FSIZE; i < frags; i++) {
			clrbit(cgp->cg_free, bno + i);
			cgp->cg_nffree--;
			fs->fs_nffree--;
		}
		fs->fs_fmod++;
		bdwrite(bp);
		return (bprev);
	}
	brelse(bp);
	return (0);
}

daddr_t
alloccg(dev, fs, cg, bpref, size)
	dev_t dev;
	register struct fs *fs;
	int cg;
	daddr_t bpref;
	int size;
{
	register struct buf *bp;
	register struct cg *cgp;
	int bno, frags;
	int allocsiz;
	int start, len, loc;
	int blk, field, subfield, pos;
	register int i;

	bp = bread(dev, cgtod(cg, fs), BSIZE);
	if (bp->b_flags & B_ERROR)
		return (0);
	cgp = bp->b_un.b_cg;
	if (size == BSIZE) {
		if (cgp->cg_nbfree == 0) {
			brelse(bp);
			return (0);
		}
		bno = alloccgblk(dev, fs, cgp, bpref);
		bdwrite(bp);
		return (bno);
	}
	/*
	 * check to see if any fragments are already available
	 * allocsiz is the size which will be allocated, hacking
	 * it down to a smaller size if necessary
	 */
	frags = size / FSIZE;
	for (allocsiz = frags; allocsiz < FRAG; allocsiz++)
		if (cgp->cg_frsum[allocsiz] != 0)
			break;
	if (allocsiz == FRAG) {
		/*
		 * no fragments were available, so a block will be 
		 * allocated, and hacked up
		 */
		if (cgp->cg_nbfree == 0) {
			brelse(bp);
			return (0);
		}
		bno = alloccgblk(dev, fs, cgp, bpref);
		bpref = bno % fs->fs_fpg;
		for (i = frags; i < FRAG; i++)
			setbit(cgp->cg_free, bpref + i);
		i = FRAG - frags;
		cgp->cg_nffree += i;
		fs->fs_nffree += i;
		cgp->cg_frsum[i]++;
		bdwrite(bp);
		return (bno);
	}
	/*
	 * find the fragment by searching through the free block
	 * map for an appropriate bit pattern
	 */
	if (bpref)
		start = bpref % fs->fs_fpg / NBBY;
	else
		start = cgp->cg_frotor / NBBY;
	len = roundup(fs->fs_fpg - 1, NBBY) / NBBY - start;
	loc = scanc(len, &cgp->cg_free[start], fragtbl, 1 << (allocsiz - 1));
	if (loc == 0) {
		len = start - 1;
		start = (cgdmin(cg, fs) - cgbase(cg, fs)) / NBBY;
		loc = scanc(len, &cgp->cg_free[start], fragtbl,
			1 << (allocsiz - 1));
		if (loc == 0)
			panic("alloccg: can't find frag");
	}
	bno = (start + len - loc) * NBBY;
	cgp->cg_frotor = bno;
	/*
	 * found the byte in the map
	 * sift through the bits to find the selected frag
	 */
	for (i = 0; i < NBBY; i += FRAG) {
		blk = (cgp->cg_free[bno / NBBY] >> i) & (0xff >> NBBY - FRAG);
		blk <<= 1;
		field = around[allocsiz];
		subfield = inside[allocsiz];
		for (pos = 0; pos <= FRAG - allocsiz; pos++) {
			if ((blk & field) == subfield) {
				bno += i + pos;
				goto gotit;
			}
			field <<= 1;
			subfield <<= 1;
		}
	}
	panic("alloccg: frag not in block");
gotit:
	for (i = 0; i < frags; i++)
		clrbit(cgp->cg_free, bno + i);
	cgp->cg_nffree -= frags;
	fs->fs_nffree -= frags;
	cgp->cg_frsum[allocsiz]--;
	if (frags != allocsiz)
		cgp->cg_frsum[allocsiz - frags]++;
	bdwrite(bp);
	return (cg * fs->fs_fpg + bno);
}

daddr_t
alloccgblk(dev, fs, cgp, bpref)
	dev_t dev;
	struct fs *fs;
	register struct cg *cgp;
	daddr_t bpref;
{
	register int i;

	if (bpref) {
		bpref &= ~(FRAG - 1);
		bpref %= fs->fs_fpg;
		if (isblock(cgp->cg_free, bpref/FRAG))
			goto gotit;
	} else
		bpref = cgp->cg_rotor;
	for (i = 0; i < cgp->cg_ndblk; i += FRAG) {
		bpref += FRAG;
		if (bpref >= cgp->cg_ndblk)
			bpref = 0;
		if (isblock(cgp->cg_free, bpref/FRAG)) {
			cgp->cg_rotor = bpref;
			goto gotit;
		}
	}
	panic("alloccgblk: can't find a blk");
	return (0);
gotit:
	clrblock(cgp->cg_free, bpref/FRAG);
	cgp->cg_nbfree--;
	fs->fs_nbfree--;
	fs->fs_cs[cgp->cg_cgx].cs_nbfree--;
	i = bpref * NSPF;
	cgp->cg_b[i/fs->fs_spc][i%fs->fs_nsect*NRPOS/fs->fs_nsect]--;
	fs->fs_fmod++;
	return (cgp->cg_cgx * fs->fs_fpg + bpref);
}
	
long
ialloccg(dev, fs, cg, ipref, mode)
	dev_t dev;
	register struct fs *fs;
	int cg;
	daddr_t ipref;
	int mode;
{
	register struct buf *bp;
	register struct cg *cgp;
	int i;

	bp = bread(dev, cgtod(cg, fs), BSIZE);
	if (bp->b_flags & B_ERROR)
		return (0);
	cgp = bp->b_un.b_cg;
	if (cgp->cg_nifree == 0) {
		brelse(bp);
		return (0);
	}
	if (ipref) {
		ipref %= fs->fs_ipg;
		if (isclr(cgp->cg_iused, ipref))
			goto gotit;
	} else
		ipref = cgp->cg_irotor;
	for (i = 0; i < fs->fs_ipg; i++) {
		ipref++;
		if (ipref >= fs->fs_ipg)
			ipref = 0;
		if (isclr(cgp->cg_iused, ipref)) {
			cgp->cg_irotor = ipref;
			goto gotit;
		}
	}
	brelse(bp);
	return (0);
gotit:
	setbit(cgp->cg_iused, ipref);
	cgp->cg_nifree--;
	fs->fs_nifree--;
	fs->fs_cs[cg].cs_nifree--;
	fs->fs_fmod++;
	if ((mode & IFMT) == IFDIR) {
		cgp->cg_ndir++;
		fs->fs_cs[cg].cs_ndir++;
	}
	bdwrite(bp);
	return (cg * fs->fs_ipg + ipref);
}

fre(dev, bno, size)
	dev_t dev;
	daddr_t bno;
	int size;
{
	register struct fs *fs;
	register struct cg *cgp;
	register struct buf *bp;
	int cg, blk, frags, bbase;
	register int i;

	if ((unsigned)size > BSIZE || size % FSIZE != 0)
		panic("free: bad size");
	fs = getfs(dev);
	cg = dtog(bno, fs);
	if (badblock(fs, bno))
		return;
	bp = bread(dev, cgtod(cg, fs), BSIZE);
	if (bp->b_flags & B_ERROR)
		return;
	cgp = bp->b_un.b_cg;
	bno %= fs->fs_fpg;
	if (size == BSIZE) {
		if (isblock(cgp->cg_free, bno/FRAG))
			panic("free: freeing free block");
		setblock(cgp->cg_free, bno/FRAG);
		cgp->cg_nbfree++;
		fs->fs_nbfree++;
		fs->fs_cs[cg].cs_nbfree++;
		i = bno * NSPF;
		cgp->cg_b[i/fs->fs_spc][i%fs->fs_nsect*NRPOS/fs->fs_nsect]++;
	} else {
		bbase = bno - (bno % FRAG);
		/*
		 * decrement the counts associated with the old frags
		 */
		blk = ((cgp->cg_free[bbase / NBBY] >> (bbase % NBBY)) &
		       (0xff >> (NBBY - FRAG)));
		fragacct(blk, cgp->cg_frsum, -1);
		/*
		 * deallocate the fragment
		 */
		frags = size / FSIZE;
		for (i = 0; i < frags; i++) {
			if (isset(cgp->cg_free, bno + i))
				panic("free: freeing free frag");
			setbit(cgp->cg_free, bno + i);
			cgp->cg_nffree++;
			fs->fs_nffree++;
		}
		/*
		 * add back in counts associated with the new frags
		 */
		blk = ((cgp->cg_free[bbase / NBBY] >> (bbase % NBBY)) &
		       (0xff >> (NBBY - FRAG)));
		fragacct(blk, cgp->cg_frsum, 1);
		/*
		 * if a complete block has been reassembled, account for it
		 */
		if (isblock(cgp->cg_free, bbase / FRAG)) {
			cgp->cg_nffree -= FRAG;
			fs->fs_nffree -= FRAG;
			cgp->cg_nbfree++;
			fs->fs_nbfree++;
			fs->fs_cs[cg].cs_nbfree++;
			i = bbase * NSPF;
			cgp->cg_b[i / fs->fs_spc]
				 [i % fs->fs_nsect * NRPOS / fs->fs_nsect]++;
		}
	}
	fs->fs_fmod++;
	bdwrite(bp);
}

ifree(dev, ino, mode)
	dev_t dev;
	ino_t ino;
	int mode;
{
	register struct fs *fs;
	register struct cg *cgp;
	register struct buf *bp;
	int i;
	int cg;

	fs = getfs(dev);
	if ((unsigned)ino >= fs->fs_ipg*fs->fs_ncg)
		panic("ifree: range");
	cg = itog(ino, fs);
	bp = bread(dev, cgtod(cg, fs), BSIZE);
	if (bp->b_flags & B_ERROR)
		return;
	cgp = bp->b_un.b_cg;
	ino %= fs->fs_ipg;
	if (isclr(cgp->cg_iused, ino))
		panic("ifree: freeing free inode");
	clrbit(cgp->cg_iused, ino);
	cgp->cg_nifree++;
	fs->fs_nifree++;
	fs->fs_cs[cg].cs_nifree++;
	if ((mode & IFMT) == IFDIR) {
		cgp->cg_ndir--;
		fs->fs_cs[cg].cs_ndir--;
	}
	fs->fs_fmod++;
	bdwrite(bp);
}

/*
 * update the frsum fields to reflect addition or deletion 
 * of some frags
 */
fragacct(fragmap, fraglist, cnt)
	char fragmap;
	short fraglist[];
	int cnt;
{
	int inblk;
	register int field, subfield;
	register int siz, pos;

	inblk = (int)(fragtbl[fragmap] << 1);
	fragmap <<= 1;
	for (siz = 1; siz < FRAG; siz++) {
		if (((1 << siz) & inblk) == 0)
			continue;
		field = around[siz];
		subfield = inside[siz];
		for (pos = siz; pos <= FRAG; pos++) {
			if ((fragmap & field) == subfield) {
				fraglist[siz] += cnt;
				pos += siz;
				field <<= siz;
				subfield <<= siz;
			}
			field <<= 1;
			subfield <<= 1;
		}
	}
}

badblock(fs, bn)
	register struct fs *fs;
	daddr_t bn;
{

	if ((unsigned)bn >= fs->fs_size || bn < cgdmin(dtog(bn, fs), fs)) {
		fserr(fs, "bad block");
		return (1);
	}
	return (0);
}

/*
 * getfs maps a device number into
 * a pointer to the incore super
 * block.  The algorithm is a linear
 * search through the mount table.
 * A consistency check of the
 * in core free-block and i-node
 * counts is performed.
 *
 * panic: no fs -- the device is not mounted.
 *	this "cannot happen"
 */
struct fs *
getfs(dev)
	dev_t dev;
{
	register struct mount *mp;
	register struct fs *fs;

	for (mp = &mount[0]; mp < &mount[NMOUNT]; mp++)
		if (mp->m_bufp != NULL && mp->m_dev == dev) {
			fs = mp->m_bufp->b_un.b_fs;
			if (fs->fs_magic != FS_MAGIC)
				panic("getfs: bad magic");
			return (fs);
		}
	panic("getfs: no fs");
	return (NULL);
}

/*
 * Fserr prints the name of a file system
 * with an error diagnostic, in the form
 *	fs: error message
 */
fserr(fs, cp)
	struct fs *fs;
	char *cp;
{

	printf("%s: %s\n", fs->fs_fsmnt, cp);
}

/*
 * Getfsx returns the index in the file system
 * table of the specified device.  The swap device
 * is also assigned a pseudo-index.  The index may
 * be used as a compressed indication of the location
 * of a block, recording
 *	<getfsx(dev),blkno>
 * rather than
 *	<dev, blkno>
 * provided the information need remain valid only
 * as long as the file system is mounted.
 */
getfsx(dev)
	dev_t dev;
{
	register struct mount *mp;

	if (dev == swapdev)
		return (MSWAPX);
	for(mp = &mount[0]; mp < &mount[NMOUNT]; mp++)
		if (mp->m_dev == dev)
			return (mp - &mount[0]);
	return (-1);
}

/*
 * Update is the internal name of 'sync'.  It goes through the disk
 * queues to initiate sandbagged IO; goes through the inodes to write
 * modified nodes; and it goes through the mount table to initiate modified
 * super blocks.
 */
update()
{
	register struct inode *ip;
	register struct mount *mp;
	register struct buf *bp;
	struct fs *fs;
	time_t tim;
	int i;

	if (updlock)
		return;
	updlock++;
	/*
	 * Write back modified superblocks.
	 * Consistency check that the superblock
	 * of each file system is still in the buffer cache.
	 */
	for (mp = &mount[0]; mp < &mount[NMOUNT]; mp++)
		if (mp->m_bufp != NULL) {
			fs = mp->m_bufp->b_un.b_fs;
			if (fs->fs_fmod == 0)
				continue;
			if (fs->fs_ronly != 0)
				panic("update: rofs mod");
			bp = getblk(mp->m_dev, SBLOCK, BSIZE);
			fs->fs_fmod = 0;
			fs->fs_time = TIME;
			if (bp->b_un.b_fs != fs)
				panic("update: bad b_fs");
			bwrite(bp);
			for (i = 0; i < cssize(fs); i += BSIZE) {
				bp = getblk(mp->m_dev, csaddr(fs) + i / FSIZE,
					BSIZE);
				bcopy(fs->fs_cs + i, bp->b_un.b_addr, BSIZE);
				bwrite(bp);
			}
		}
	/*
	 * Write back each (modified) inode.
	 */
	for (ip = inode; ip < inodeNINODE; ip++)
		if((ip->i_flag&ILOCK)==0 && ip->i_count) {
			ip->i_flag |= ILOCK;
			ip->i_count++;
			tim = TIME;
			iupdat(ip, &tim, &tim, 0);
			iput(ip);
		}
	updlock = 0;
	/*
	 * Force stale buffer cache information to be flushed,
	 * for all devices.
	 */
	bflush(NODEV);
}
