/* $NetBSD: eqtf2.c,v 1.1 2000/06/06 08:15:02 bjh21 Exp $ */

/*
 * Written by Matt Thomas, 2011.  This file is in the Public Domain.
 */

#include <sys/cdefs.h>
#if defined(LIBC_SCCS) && !defined(lint)
__RCSID("$NetBSD: eqtf2.c,v 1.1 2000/06/06 08:15:02 bjh21 Exp $");
#endif /* LIBC_SCCS and not lint */

#include "softfloat-for-gcc.h"
#include "milieu.h"
#include "softfloat.h"

#ifdef FLOAT128
flag __eqtf2(float128, float128);

flag
__eqtf2(float128 a, float128 b)
{

	/* libgcc1.c says !(a == b) */
	return !float128_eq(a, b);
}
#endif /* FLOAT128 */