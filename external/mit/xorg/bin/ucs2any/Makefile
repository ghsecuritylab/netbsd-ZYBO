#	$NetBSD: Makefile,v 1.2 2010/11/21 01:25:32 mrg Exp $

.include <bsd.own.mk>

PROG=	ucs2any

X11EXTRAMANDEFS+= 	-e 's,__mapfilesdir__,${LIBDIR}/X11/fonts/util,'

.PATH:	${X11SRCDIR.font-util} ${X11SRCDIR.font-util}/man

.include <bsd.x11.mk>
.include <bsd.prog.mk>
