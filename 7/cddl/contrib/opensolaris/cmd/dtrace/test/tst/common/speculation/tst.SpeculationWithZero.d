/*
 * CDDL HEADER START
 *
 * The contents of this file are subject to the terms of the
 * Common Development and Distribution License (the "License").
 * You may not use this file except in compliance with the License.
 *
 * You can obtain a copy of the license at usr/src/OPENSOLARIS.LICENSE
 * or http://www.opensolaris.org/os/licensing.
 * See the License for the specific language governing permissions
 * and limitations under the License.
 *
 * When distributing Covered Code, include this CDDL HEADER in each
 * file and include the License file at usr/src/OPENSOLARIS.LICENSE.
 * If applicable, add the following below this CDDL HEADER, with the
 * fields enclosed by brackets "[]" replaced with your own identifying
 * information: Portions Copyright [yyyy] [name of copyright owner]
 *
 * CDDL HEADER END
 */

/*
 * Copyright 2006 Sun Microsystems, Inc.  All rights reserved.
 * Use is subject to license terms.
 */

#pragma ident	"%Z%%M%	%I%	%E% SMI"

/*
 * ASSERTION:
 * Calling speculate() with zero does not have any ill effects.
 * Statement after speculate does not execute.
 *
 * SECTION: Speculative Tracing/Creating a Speculation
 */

#pragma D option quiet

BEGIN
{
	self->speculateFlag = 0;
	self->spec = speculation();
	self->spec = speculation();
	printf("Speculative buffer ID: %d\n", self->spec);
}

BEGIN
{
	speculate(self->spec);
	self->speculateFlag++;
}

BEGIN
/1 == self->speculateFlag/
{
	printf("Statement was executed\n");
	exit(1);
}

BEGIN
/1 != self->speculateFlag/
{
	printf("Statement wasn't executed\n");
	exit(0);
}
