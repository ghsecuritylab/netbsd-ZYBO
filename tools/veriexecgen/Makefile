#       $NetBSD: Makefile,v 1.1 2006/12/19 21:23:18 agc Exp $

HOSTPROGNAME=   ${_TOOL_PREFIX}veriexecgen
HOST_SRCDIR=    usr.sbin/veriexecgen

HOST_CPPFLAGS+= -I${TOOLDIR}/include/nbinclude
HOST_CPPFLAGS+= -I${.CURDIR}/../compat
HOST_CPPFLAGS+= -I${.CURDIR}/../compat/sys

.include "${.CURDIR}/../Makefile.host"
