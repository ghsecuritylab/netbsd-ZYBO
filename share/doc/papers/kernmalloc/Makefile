#	$NetBSD: Makefile,v 1.3 2002/06/02 17:40:45 pooka Exp $
#
#	@(#)Makefile	1.8 (Berkeley) 6/8/93

DIR=	papers/kernmalloc
SRCS=	kernmalloc.t appendix.t
MACROS=	-ms

paper.ps: ${SRCS} alloc.fig usage.tbl
	${TOOL_SOELIM} ${SRCS} | ${TOOL_TBL} | ${TOOL_PIC} | ${TOOL_EQN} | \
	    ${TOOL_VGRIND} | ${TOOL_ROFF_PS} ${MACROS} > ${.TARGET}

.include <bsd.doc.mk>
