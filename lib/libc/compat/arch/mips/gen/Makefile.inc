#	$NetBSD: Makefile.inc,v 1.2 2005/10/07 17:16:39 tsutsui Exp $

# objects built from assembler sources (need lint stubs)
SRCS+=	compat_setjmp.S compat_sigsetjmp.S

# objects built from C sources in compat/gen
SRCS+=	compat_frexp_ieee754.c
#SRCS+=	compat_ldexp_ieee754.c compat_modf_ieee754.c
