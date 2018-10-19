# $NetBSD: Makefile.inc,v 1.4 2012/06/11 15:18:05 chs Exp $

CFLAGS+=	-mno-red-zone -mno-mmx -mno-sse -mno-avx
CFLAGS+=	-msoft-float

# this should really be !(RUMPKERNEL && PIC)
.if !defined(RUMPKERNEL)
CFLAGS+=	-mcmodel=kernel -fno-omit-frame-pointer
.endif

USE_SSP?=	yes