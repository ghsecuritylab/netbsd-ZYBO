/*	$NetBSD: trap.h,v 1.7 1994/10/26 02:06:42 cgd Exp $	*/

#ifndef _MACHINE_TRAP_H_
#define _MACHINE_TRAP_H_

#include <m68k/trap.h>

/*
 * make sure we don't have this one defined, it's no longer done
 * with the REI emulation.
 */
#undef T_SSIR

#endif