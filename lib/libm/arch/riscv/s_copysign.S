/* $NetBSD: s_copysign.S,v 1.3 2014/11/15 21:07:01 joerg Exp $ */

/* The contents of this file are in the public domain. */

#include <machine/asm.h>

STRONG_ALIAS(_copysignl, copysign)
WEAK_ALIAS(copysignl, _copysignl)

ENTRY(copysign)
	fsgnj.d	fa0, fa0, fa1
	ret
END(copysign)
