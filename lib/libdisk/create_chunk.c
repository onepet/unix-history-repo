/*
 * ----------------------------------------------------------------------------
 * "THE BEER-WARE LICENSE" (Revision 42):
 * <phk@login.dknet.dk> wrote this file.  As long as you retain this notice you
 * can do whatever you want with this stuff. If we meet some day, and you think
 * this stuff is worth it, you can buy me a beer in return.   Poul-Henning Kamp
 * ----------------------------------------------------------------------------
 *
 * $Id: create_chunk.c,v 1.7 1995/05/03 06:30:50 phk Exp $
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/disklabel.h>
#include <sys/diskslice.h>
#include <sys/types.h>
#include <err.h>
#include "libdisk.h"

void
Fixup_FreeBSD_Names(struct disk *d, struct chunk *c)
{
	struct chunk *c1, *c3;
	int j;

	if (!strcmp(c->name, "X")) return;

	/* reset all names to "X" */
	for (c1 = c->part; c1 ; c1 = c1->next) {
		c1->oname = c1->name;
		c1->name = malloc(12);
		if(!c1->name) err(1,"Malloc failed");
		strcpy(c1->name,"X");
	}

	/* Allocate the first swap-partition we find */
	for (c1 = c->part; c1 ; c1 = c1->next) { 
		if (c1->type == unused) continue;
		if (c1->type == reserved) continue;
		if (c1->subtype != FS_SWAP) continue;
		sprintf(c1->name,"%s%c",c->name,SWAP_PART+'a');
		break;
	}

	/* Allocate the first root-partition we find */
	for (c1 = c->part; c1 ; c1 = c1->next) { 
		if (c1->type == unused) continue;
		if (c1->type == reserved) continue;
		if (!(c1->flags & CHUNK_IS_ROOT)) continue;
		sprintf(c1->name,"%s%c",c->name,0+'a');
		break;
	}

	/* Try to give them the same as they had before */
	for (c1 = c->part; c1 ; c1 = c1->next) {
		for(c3 = c->part; c3 ; c3 = c3->next) 
			if (c1 != c3 && !strcmp(c3->name, c1->oname)) {
				strcpy(c1->name,c1->oname);
				break;
			}
	}

	/* Allocate the rest sequentially */
	for (c1 = c->part; c1 ; c1 = c1->next) {
		const char order[] = "defghab";
		if (c1->type == unused) continue;
		if (c1->type == reserved) continue;
		if (strcmp("X",c1->name)) continue;

		for(j=0;j<strlen(order);j++) {
			sprintf(c1->name,"%s%c",c->name,order[j]);
			for(c3 = c->part; c3 ; c3 = c3->next) 
				if (c1 != c3 && !strcmp(c3->name, c1->name))
					goto match;
			break;
		match:
			strcpy(c1->name,"X");
			continue;
		}
	}
	for (c1 = c->part; c1 ; c1 = c1->next) {
		free(c1->oname);
		c1->oname = 0;
	}
}

void
Fixup_Extended_Names(struct disk *d, struct chunk *c)
{
	struct chunk *c1, *c3;
	int j;
	char *p=0;

	for (c1 = c->part; c1 ; c1 = c1->next) {
		if (c1->type == freebsd)
			Fixup_FreeBSD_Names(d,c1);
		if (c1->type == unused) continue;
		if (c1->type == reserved) continue;
		if (strcmp(c1->name, "X")) continue;
		for(j=5;j<=29;j++) {
			p = malloc(12);
			if(!p) err(1,"malloc failed");
			sprintf(p,"%ss%d",c->name,j);
			for(c3 = c->part; c3 ; c3 = c3->next) 
				if (c3 != c1 && !strcmp(c3->name, p))
					goto match;
			free(c1->name);
			c1->name = p;
			p = 0;
			break;
			match:
				continue;
		}
		if(p)
			free(p);
	}
}

void
Fixup_Names(struct disk *d)
{
	struct chunk *c1, *c2, *c3;
	int i,j;
	char *p=0;

	c1 = d->chunks;
	for(i=1,c2 = c1->part; c2 ; c2 = c2->next) {
		if (c2->type == freebsd)
			Fixup_FreeBSD_Names(d,c2);
		if (c2->type == extended)
			Fixup_Extended_Names(d,c2);
		if (c2->type == unused)
			continue;
		if (c2->type == reserved)
			continue;
		p = malloc(12);
		if(!p) err(1,"malloc failed");
		for(j=1;j<=NDOSPART;j++) {
			sprintf(p,"%ss%d",c1->name,j);
			for(c3 = c1->part; c3 ; c3 = c3->next) 
				if (c3 != c2 && !strcmp(c3->name, p))
					goto match;
			free(c2->name);
			c2->name = p;
			p = 0;
			break;
			match:
				continue;
		}
		if(p)
			free(p);
	}
}

int
Create_Chunk(struct disk *d, u_long offset, u_long size, chunk_e type, int subtype, u_long flags)
{
	int i;

	if (type == freebsd)
		subtype = 0xa5;
	i = Add_Chunk(d,offset,size,"X",type,subtype,flags);
	Fixup_Names(d);
	return i;
}
