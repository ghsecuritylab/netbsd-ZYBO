/*	$NetBSD$	*/
/*	$OpenBSD: bus.h,v 1.1 1997/10/13 10:53:42 pefo Exp $	*/

#ifndef _SANDPOINT_BUS_DEFS_H_
#define _SANDPOINT_BUS_DEFS_H_

/*
 * Values for the SandPoint bus space tag, not to be used directly by MI code.
 */
#define	SANDPOINT_BUS_SPACE_IO	0xFE000000	/* i/o space */
#define SANDPOINT_BUS_SPACE_MEM	0x80000000	/* mem space */
#define SANDPOINT_BUS_SPACE_EUMB	0xFC000000	/* EUMB space */
#define SANDPOINT_PCI_CONFIG_ADDR	0xFEC00CF8
#define SANDPOINT_PCI_CONFIG_DATA	0xFEE00CFC

/*
 * Address conversion as seen from a PCI master.
 */
#define PHYS_TO_BUS_MEM(t,x)	(x)
#define BUS_MEM_TO_PHYS(t,x)	(x)

#include <powerpc/bus_defs.h>

#endif /* _SANDPOINT_BUS_DEFS_H_ */
