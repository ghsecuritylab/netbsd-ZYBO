/* $NetBSD: tegra_mcreg.h,v 1.1 2015/03/29 10:41:59 jmcneill Exp $ */

/*-
 * Copyright (c) 2015 Jared D. McNeill <jmcneill@invisible.ca>
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
 * BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED
 * AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#ifndef _ARM_TEGRA_SOCTHERMREG_H
#define _ARM_TEGRA_SOCTHERMREG_H

#define SOC_THERM_TSENSOR_PDIV_REG		0x1c0
#define SOC_THERM_TSENSOR_HOTSPOT_OFF_REG	0x1c4
#define SOC_THERM_TSENSOR_TEMP1_REG		0x1c8
#define SOC_THERM_TSENSOR_TEMP2_REG		0x1cc

#define SOC_THERM_TSENSOR_CONFIG0_OFFSET	0x00
#define SOC_THERM_TSENSOR_CONFIG0_TALL		__BITS(27,8)
#define SOC_THERM_TSENSOR_CONFIG0_STATUS_CLR	__BIT(5)
#define SOC_THERM_TSENSOR_CONFIG0_TCALC_OVERFLOW __BIT(4)
#define SOC_THERM_TSENSOR_CONFIG0_OVERFLOW	__BIT(3)
#define SOC_THERM_TSENSOR_CONFIG0_CPTR_OVERFLOW	__BIT(2)
#define SOC_THERM_TSENSOR_CONFIG0_RO_SEL	__BIT(1)
#define SOC_THERM_TSENSOR_CONFIG0_STOP		__BIT(0)

#define SOC_THERM_TSENSOR_CONFIG1_OFFSET	0x04
#define SOC_THERM_TSENSOR_CONFIG1_TEMP_ENABLE	__BIT(31)
#define SOC_THERM_TSENSOR_CONFIG1_TEN_COUNT	__BITS(29,24)
#define SOC_THERM_TSENSOR_CONFIG1_TIDDQ_EN	__BITS(20,15)
#define SOC_THERM_TSENSOR_CONFIG1_TSAMPLE	__BITS(9,0)

#define SOC_THERM_TSENSOR_CONFIG2_OFFSET	0x08
#define SOC_THERM_TSENSOR_CONFIG2_THERM_A	__BITS(31,16)
#define SOC_THERM_TSENSOR_CONFIG2_THERM_B	__BITS(15,0)

#define SOC_THERM_TSENSOR_STATUS0_OFFSET	0x0c
#define SOC_THERM_TSENSOR_STATUS0_CAPTURE_VALID	__BIT(31)
#define SOC_THERM_TSENSOR_STATUS0_CAPTURE	__BITS(15,0)

#define SOC_THERM_TSENSOR_STATUS1_OFFSET	0x10
#define SOC_THERM_TSENSOR_STATUS1_TEMP_VALID	__BIT(31)
#define SOC_THERM_TSENSOR_STATUS1_TEMP		__BITS(15,0)

#define SOC_THERM_TSENSOR_STATUS2_OFFSET	0x14
#define SOC_THERM_TSENSOR_STATUS2_TEMP_MAX	__BITS(31,16)
#define SOC_THERM_TSENSOR_STATUS2_TEMP_MIN	__BITS(15,0)

#endif /* _ARM_TEGRA_SOCTHERMREG_H */