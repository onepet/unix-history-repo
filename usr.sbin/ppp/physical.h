/*
 * Written by Eivind Eklund <eivind@yes.no>
 *    for Yes Interactive
 *
 * Copyright (C) 1998, Yes Interactive.  All rights reserved.
 *
 * Redistribution and use in any form is permitted.  Redistribution in
 * source form should include the above copyright and this set of
 * conditions, because large sections american law seems to have been
 * created by a bunch of jerks on drugs that are now illegal, forcing
 * me to include this copyright-stuff instead of placing this in the
 * public domain.  The name of of 'Yes Interactive' or 'Eivind Eklund'
 * may not be used to endorse or promote products derived from this
 * software without specific prior written permission.
 * THIS SOFTWARE IS PROVIDED ``AS IS'' AND WITHOUT ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, WITHOUT LIMITATION, THE IMPLIED
 * WARRANTIES OF MERCHANTIBILITY AND FITNESS FOR A PARTICULAR PURPOSE.
 *
 *  $Id: physical.h,v 1.8 1999/04/27 00:23:57 brian Exp $
 *
 */

struct datalink;
struct bundle;
struct iovec;
struct physical;
struct bundle;
struct ccp;
struct cmdargs;

#define TTY_DEVICE	1
#define TCP_DEVICE	2
#define EXEC_DEVICE	3

struct device {
  int type;
  const char *name;
  int (*open)(struct physical *);
  int (*raw)(struct physical *);
  void (*offline)(struct physical *);
  void (*cooked)(struct physical *);
  void (*postclose)(struct physical *);
  void (*restored)(struct physical *);
  int (*speed)(struct physical *);
  const char *(*openinfo)(struct physical *);
};

struct physical {
  struct link link;
  struct descriptor desc;
  int type;                    /* What sort of PHYS_* link are we ? */
  struct async async;          /* Our async state */
  struct hdlc hdlc;            /* Our hdlc state */
  int fd;                      /* File descriptor for this device */
  int mbits;                   /* Current DCD status */
  struct mbuf *out;            /* mbuf that suffered a short write */
  int connect_count;
  struct datalink *dl;         /* my owner */

  struct {
    u_char buf[MAX_MRU];       /* Our input data buffer */
    size_t sz;
  } input;

  struct {
    char full[40];             /* Our current device name */
    char *base;
  } name;

  unsigned Utmp : 1;           /* Are we in utmp ? */
  pid_t session_owner;         /* HUP this when closing the link */

  struct {
    unsigned rts_cts : 1;      /* Is rts/cts enabled ? */
    unsigned parity;           /* What parity is enabled? (tty flags) */
    unsigned speed;            /* tty speed */

    char devlist[LINE_LEN];    /* NUL separated list of devices */
    int ndev;                  /* number of devices in list */
    struct {
      unsigned required : 1;   /* Is cd *REQUIRED* on this device */
      int delay;               /* Wait this many seconds after login script */
    } cd;
  } cfg;

  struct termios ios;          /* To be able to reset from raw mode */

  struct pppTimer Timer;       /* CD checks */

  const struct device *handler; /* device specific handlers */
};

#define field2phys(fp, name) \
  ((struct physical *)((char *)fp - (int)(&((struct physical *)0)->name)))

#define link2physical(l) \
  ((l)->type == PHYSICAL_LINK ? field2phys(l, link) : NULL)

#define descriptor2physical(d) \
  ((d)->type == PHYSICAL_DESCRIPTOR ? field2phys(d, desc) : NULL)

extern struct physical *physical_Create(struct datalink *, int);
extern int physical_Open(struct physical *, struct bundle *);
extern int physical_Raw(struct physical *);
extern int physical_GetSpeed(struct physical *);
extern int physical_SetSpeed(struct physical *, int);
extern int physical_SetParity(struct physical *, const char *);
extern int physical_SetRtsCts(struct physical *, int);
extern void physical_SetSync(struct physical *);
extern int physical_ShowStatus(struct cmdargs const *);
extern void physical_Offline(struct physical *);
extern void physical_Close(struct physical *);
extern void physical_Destroy(struct physical *);
extern struct physical *iov2physical(struct datalink *, struct iovec *, int *,
                                     int, int);
extern int physical2iov(struct physical *, struct iovec *, int *, int, pid_t);
extern void physical_ChangedPid(struct physical *, pid_t);

extern int physical_IsSync(struct physical *);
extern const char *physical_GetDevice(struct physical *);
extern void physical_SetDeviceList(struct physical *, int, const char *const *);
extern void physical_SetDevice(struct physical *, const char *);

extern ssize_t physical_Read(struct physical *, void *, size_t);
extern ssize_t physical_Write(struct physical *, const void *, size_t);
extern int physical_doUpdateSet(struct descriptor *, fd_set *, fd_set *,
                                fd_set *, int *, int);
extern int physical_IsSet(struct descriptor *, const fd_set *);
extern void physical_Login(struct physical *, const char *);
extern int physical_RemoveFromSet(struct physical *, fd_set *, fd_set *,
                                  fd_set *);
extern int physical_SetMode(struct physical *, int);
extern void physical_DeleteQueue(struct physical *);
extern void physical_SetupStack(struct physical *, int);
