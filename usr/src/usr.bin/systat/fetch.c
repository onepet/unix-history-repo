#ifndef lint
static char sccsid[] = "@(#)fetch.c	1.1 (Lucasfilm) %G%";
#endif

#include "systat.h"

long
getw(loc)
        int loc;
{
        long word;

        lseek(kmem, loc, L_SET);
        if (read(kmem, &word, sizeof (word)) != sizeof (word))
                printf("Error reading kmem at %x\n", loc);
        return (word);
}

char *
getpname(pid, mproc)
        int pid;
        register struct proc *mproc;
{
        register struct procs *pp;
        register char *namp;
        register int j;
        char *getcmd();

        pp = procs;
        for (j = numprocs - 1; j >= 0; j--) {
                if (pp->pid == pid)
                        return (pp->cmd);
                pp++;
        }
        if (j < 0) {
                if (numprocs < 200) {
                        pp = &procs[numprocs];
                        namp = strncpy(pp->cmd, getcmd(pid, mproc), 15);
                        pp->cmd[15] = 0;
                        pp->pid = pid;
                        numprocs++;
                } else
                        namp = getcmd(pid, mproc);
        }
        return (namp);
}

union {
        struct  user user;
        char    upages[UPAGES][NBPG];
} user;
#define u       user.user

char *
getcmd(pid, mproc)
        int pid;
        register struct proc *mproc;
{
        static char cmd[30];

        if (mproc == NULL || mproc->p_stat == SZOMB ||
            mproc->p_flag&(SSYS|SWEXIT))
                return ("");
        getu(mproc);
        (void) strncpy(cmd, u.u_comm, sizeof (cmd));
        return (cmd);
}

getu(mproc)
        register struct proc *mproc;
{
        struct pte *pteaddr, apte;
        struct pte arguutl[UPAGES+CLSIZE];
        register int i;
        int ncl, size;

        size = sizeof (struct user);
        if ((mproc->p_flag & SLOAD) == 0) {
                if (swap < 0)
                        return (0);
                (void) lseek(swap, (long)dtob(mproc->p_swaddr), L_SET);
                if (read(swap, (char *)&user.user, size) != size) {
                        fprintf(stderr, "ps: cant read u for pid %d from %s\n",
                            mproc->p_pid, swapf);
                        return (0);
                }
                pcbpf = 0;
                argaddr = 0;
                return (1);
        }
        pteaddr = &Usrptma[btokmx(mproc->p_p0br) + mproc->p_szpt - 1];
        klseek(kmem, (long)pteaddr, L_SET);
        if (read(kmem, (char *)&apte, sizeof (apte)) != sizeof (apte)) {
                printf("ps: cant read indir pte to get u for pid %d from %s\n",
                    mproc->p_pid, swapf);
                return (0);
        }
        klseek(mem,
            (long)ctob(apte.pg_pfnum+1) - (UPAGES+CLSIZE) * sizeof (struct pte),
                L_SET);
        if (read(mem, (char *)arguutl, sizeof (arguutl)) != sizeof (arguutl)) {
                printf("ps: cant read page table for u of pid %d from %s\n",
                    mproc->p_pid, kmemf);
                return (0);
        }
        if (arguutl[0].pg_fod == 0 && arguutl[0].pg_pfnum)
                argaddr = ctob(arguutl[0].pg_pfnum);
        else
                argaddr = 0;
        pcbpf = arguutl[CLSIZE].pg_pfnum;
        ncl = (size + NBPG*CLSIZE - 1) / (NBPG*CLSIZE);
        while (--ncl >= 0) {
                i = ncl * CLSIZE;
                klseek(mem, (long)ctob(arguutl[CLSIZE+i].pg_pfnum), L_SET);
                if (read(mem, user.upages[i], CLSIZE*NBPG) != CLSIZE*NBPG) {
                        printf("ps: cant read page %d of u of pid %d from %s\n",
                            arguutl[CLSIZE+i].pg_pfnum, mproc->p_pid, memf);
                        return(0);
                }
        }
        return (1);
}

klseek(fd, loc, off)
        int fd;
        long loc;
        int off;
{

        (void) lseek(fd, (long)loc, off);
}
