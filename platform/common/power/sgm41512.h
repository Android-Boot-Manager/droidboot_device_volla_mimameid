/* Copyright Statement:
*
* This software/firmware and related documentation ("MediaTek Software") are
* protected under relevant copyright laws. The information contained herein
* is confidential and proprietary to MediaTek Inc. and/or its licensors.
* Without the prior written permission of MediaTek inc. and/or its licensors,
* any reproduction, modification, use or disclosure of MediaTek Software,
* and information contained herein, in whole or in part, shall be strictly prohibited.
*/
/* MediaTek Inc. (C) 2018. All rights reserved.
*
* BY OPENING THIS FILE, RECEIVER HEREBY UNEQUIVOCALLY ACKNOWLEDGES AND AGREES
* THAT THE SOFTWARE/FIRMWARE AND ITS DOCUMENTATIONS ("MEDIATEK SOFTWARE")
* RECEIVED FROM MEDIATEK AND/OR ITS REPRESENTATIVES ARE PROVIDED TO RECEIVER ON
* AN "AS-IS" BASIS ONLY. MEDIATEK EXPRESSLY DISCLAIMS ANY AND ALL WARRANTIES,
* EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE IMPLIED WARRANTIES OF
* MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE OR NONINFRINGEMENT.
* NEITHER DOES MEDIATEK PROVIDE ANY WARRANTY WHATSOEVER WITH RESPECT TO THE
* SOFTWARE OF ANY THIRD PARTY WHICH MAY BE USED BY, INCORPORATED IN, OR
* SUPPLIED WITH THE MEDIATEK SOFTWARE, AND RECEIVER AGREES TO LOOK ONLY TO SUCH
* THIRD PARTY FOR ANY WARRANTY CLAIM RELATING THERETO. RECEIVER EXPRESSLY ACKNOWLEDGES
* THAT IT IS RECEIVER'S SOLE RESPONSIBILITY TO OBTAIN FROM ANY THIRD PARTY ALL PROPER LICENSES
* CONTAINED IN MEDIATEK SOFTWARE. MEDIATEK SHALL ALSO NOT BE RESPONSIBLE FOR ANY MEDIATEK
* SOFTWARE RELEASES MADE TO RECEIVER'S SPECIFICATION OR TO CONFORM TO A PARTICULAR
* STANDARD OR OPEN FORUM. RECEIVER'S SOLE AND EXCLUSIVE REMEDY AND MEDIATEK'S ENTIRE AND
* CUMULATIVE LIABILITY WITH RESPECT TO THE MEDIATEK SOFTWARE RELEASED HEREUNDER WILL BE,
* AT MEDIATEK'S OPTION, TO REVISE OR REPLACE THE MEDIATEK SOFTWARE AT ISSUE,
* OR REFUND ANY SOFTWARE LICENSE FEES OR SERVICE CHARGE PAID BY RECEIVER TO
* MEDIATEK FOR SUCH MEDIATEK SOFTWARE AT ISSUE.
*/

#ifndef __SGM41512_CHARGER_H
#define __SGM41512_CHARGER_H

#include <bits.h>


enum sgm41512_reg_addr {
	SGM41512_REG_00 = 0x00,
	SGM41512_REG_01,
	SGM41512_REG_02,
	SGM41512_REG_03,
	SGM41512_REG_04,
	SGM41512_REG_05,
	SGM41512_REG_06,
	SGM41512_REG_07,
	SGM41512_REG_08,
	SGM41512_REG_09,
	SGM41512_REG_0A,
	SGM41512_REG_0B,
};

enum SGM41512_streg_idx {
	SGM41512_ST_STATUS = 0,
	SGM41512_ST_FAULT,
	SGM41512_ST_STATUS2,
	SGM41512_STREG_MAX,
};

enum sgm41512_ic_stat {
	SGM41512_ICSTAT_SLEEP = 0,
	SGM41512_ICSTAT_PRECHG,
	SGM41512_ICSTAT_FASTCHG,
	SGM41512_ICSTAT_CHGDONE,
	SGM41512_ICSTAT_MAX,
};

#ifdef MTK_SGM41512_SUPPORT
int sgm41512_probe(void);
#else
static inline int sgm41512_probe(void)
{
	return 0;
}
#endif /* MTK_SGM41512_SUPPORT */


#define SGM41512_SLAVE_ADDR	0x6B

//#define SGM41512_REG_DEVID  	0x0B
/*prize add by zhaopengge 20210623 start*/
#define ETA6963_DEVID		0x07
#define ETA6953_DEVID		0x02
/*prize add by zhaopengge 20210623 end*/
#define SGM41512_DEVID		0x05
//#define SGM41512_DEVID_MASK	0x78
//#define SGM41512_DEVID_SHIFT	3	

#define SGM41512_REG_WTD 		0x05

#define SGM41512_REG_VBUS_STAT  0x08

#define SGM41512_REG_VBUS_STAT_MASK  0xE0
#define SGM41512_REG_VBUS_STAT_SHIFT 5	

#define SGM41512_REG_IINDET		 0x07
#define SGM41512_REG_IINDET_MASK 0x80
#define SGM41512_REG_IINDET_SHIFT 7	

//#define SGM41512_REGRST_MASK	BIT(7)

//#define SGM41512_REG_WDT		0x05
//#define SGM41512_WDT_SHIFT		4
//#define SGM41512_WDT_MASK			0x30



#define SGM41512_REG_STATUS	SGM41512_REG_08

/* ========== OTGCFG 0x00 ============ */
#define SGM41512_REG_OTGCC SGM41512_REG_02
#define SGM41512_OTGCC_MASK	BIT(7)

/* ========== TOP 0x01 ============ */
#define SGM41512_REG_WDTCNTRST SGM41512_REG_01
#define SGM41512_WDTCNTRST_MASK	BIT(6)
#define SGM41512_REG_WDT SGM41512_REG_05
#define SGM41512_WDT_SHIFT	4
#define SGM41512_WDT_MASK		0x30

/* ========== FUNCTION 0x02 ============ */
#define SGM41512_REG_BATFETDIS SGM41512_REG_07
#define SGM41512_BATFETDIS_SHIFT	5
#define SGM41512_BATFETDIS_MASK	BIT(5)
#define SGM41512_REG_HZ	SGM41512_REG_00
#define SGM41512_FORCE_HZ_SHIFT	7
#define SGM41512_FORCE_HZ_MASK	BIT(7)
#define SGM41512_REG_OTG_EN SGM41512_REG_01
#define SGM41512_OTG_EN_SHIFT	5
#define SGM41512_OTG_EN_MASK	BIT(5)
#define SGM41512_REG_CHG_EN SGM41512_REG_01
#define SGM41512_CHG_EN_SHIFT	4
#define SGM41512_CHG_EN_MASK	BIT(4)

#define SGM41512_REG_IINDET_EN SGM41512_REG_07
#define SGM41512_REG_IINDET_EN_MASK	BIT(7)

/* ========== IBUS 0x03 ============ */
#define SGM41512_REG_AICC_EN SGM41512_REG_07
#define SGM41512_AICC_EN_SHIFT	7
#define SGM41512_AICC_EN_MASK	BIT(7)
#define SGM41512_REG_AICR SGM41512_REG_00
#define SGM41512_AICR_SHIFT	0
#define SGM41512_AICR_MASK	0x1F
#define SGM41512_AICR_MIN		100000
#define SGM41512_AICR_MAX		3200000
#define SGM41512_AICR_STEP	100000

/* ========== VBUS 0x04 ============ */
#define SGM41512_REG_VAC_OVP SGM41512_REG_06
#define SGM41512_VAC_OVP_SHIFT	6
#define SGM41512_VAC_OVP_MASK	0xC0
#define SGM41512_REG_MIVRTRACK SGM41512_REG_07
#define SGM41512_MIVRTRACK_SHIFT	0
#define SGM41512_MIVRTRACK_MASK	0x03
#define SGM41512_REG_MIVR SGM41512_REG_06
#define SGM41512_MIVR_SHIFT	0
#define SGM41512_MIVR_MASK	0x0F
#define SGM41512_MIVR_MIN		3900000
#define SGM41512_MIVR_MAX		5400000
#define SGM41512_MIVR_STEP	100000

/* ========== VCHG 0x07 ============ */
#define SGM41512_REG_CV SGM41512_REG_04
#define SGM41512_CV_SHIFT		3
#define SGM41512_CV_MASK		0xF8
#define SGM41512_CV_MIN		3848000
#define SGM41512_CV_MAX		4616000
#define SGM41512_CV_STEP		32000

/* ========== ICHG 0x08 ============ */
#define SGM41512_REG_ICHG SGM41512_REG_02
#define SGM41512_ICHG_SHIFT	0
#define SGM41512_ICHG_MASK	0x3F
#define SGM41512_ICHG_MIN		0
#define SGM41512_ICHG_MAX		3000000
#define SGM41512_ICHG_STEP	60000

/* ========== CHGTIMER 0x09 ============ */
#define SGM41512_REG_SAFETMR_EN SGM41512_REG_05
#define SGM41512_SAFETMR_EN_SHIFT	3
#define SGM41512_SAFETMR_EN_MASK	BIT(3)
#define SGM41512_REG_SAFETMR SGM41512_REG_05
#define SGM41512_SAFETMR_SHIFT	2
#define SGM41512_SAFETMR_MASK	0x04
#define SGM41512_SAFETMR_MIN	5
#define SGM41512_SAFETMR_MAX	10
#define SGM41512_SAFETMR_STEP	5

/* ========== EOC 0x0A ============ */
#define SGM41512_REG_IEOC SGM41512_REG_03
#define SGM41512_IEOC_SHIFT	0
#define SGM41512_IEOC_MASK	0x0F
#define SGM41512_IEOC_MIN		60000
#define SGM41512_IEOC_MAX		960000
#define SGM41512_IEOC_STEP	60000
#define SGM41512_REG_TE SGM41512_REG_05
#define SGM41512_TE_MASK		BIT(7)

/* ========== INFO 0x0B ============ */
#define SGM41512_REG_REGRST SGM41512_REG_0B
#define SGM41512_REGRST_MASK	BIT(7)
#define SGM41512_REG_DEVID SGM41512_REG_0B
#define SGM41512_DEVID_SHIFT	3
#define SGM41512_DEVID_MASK	0x78
#define SGM41512_REG_DEVREV SGM41512_REG_0B
#define SGM41512_DEVREV_SHIFT	0
#define SGM41512_DEVREV_MASK	0x03

/* ========== JEITA 0x0C ============ */
#define SGM41512_REG_JEITA_COOL_ISET SGM41512_REG_05
#define SGM41512_JEITA_COOL_ISET_MASK	BIT(0)
#define SGM41512_REG_JEITA_COOL_VSET SGM41512_REG_07
#define SGM41512_JEITA_COOL_VSET_MASK	BIT(4)

/* ========== STATUS 0x0F ============ */
#define SGM41512_REG_ICSTAT SGM41512_REG_08
#define SGM41512_ICSTAT_SHIFT	3
#define SGM41512_ICSTAT_MASK	0x18
#define SGM41512_VBUSSTAT_SHIFT	5
#define SGM41512_VBUSSTAT_MASK	0xE0
#define SGM41512_CHRGSTAT_SHIFT	3
#define SGM41512_CHRGSTAT_MASK	0x18
#define SGM41512_PGSTAT_SHIFT	2
#define SGM41512_PGSTAT_MASK	BIT(2)
#define SGM41512_THERMSTAT_SHIFT	1
#define SGM41512_THERMSTAT_MASK	BIT(1)
#define SGM41512_VSYSSTAT_SHIFT	0
#define SGM41512_VSYSSTAT_MASK	BIT(0)
#define SGM41512_REG_PORTSTAT SGM41512_REG_08
#define SGM41512_PORTSTAT_SHIFT	5
#define SGM41512_PORTSTAT_MASK	0xE0
#define SGM41512_REG_FAULT SGM41512_REG_09
#define SGM41512_WDFAULT_SHIFT	7
#define SGM41512_WDFAULT_MASK	BIT(7)
#define SGM41512_BOOSTFAULT_SHIFT	6
#define SGM41512_BOOSTFAULT_MASK	BIT(6)
#define SGM41512_CHRGFAULT_SHIFT	4
#define SGM41512_CHRGFAULT_MASK	0x30
#define SGM41512_BATFAULT_SHIFT	3
#define SGM41512_BATFAULT_MASK	BIT(3)
#define SGM41512_NTCFAULT_SHIFT	0
#define SGM41512_NTCFAULT_MASK	0x07

/* ========== STAT0 0x10 ============ */
#define SGM41512_REG_VBUSGD SGM41512_REG_0A
#define SGM41512_ST_VBUSGD_SHIFT		7
#define SGM41512_ST_VBUSGD_MASK		BIT(7)
#define SGM41512_REG_CHGRDY SGM41512_REG_08
#define SGM41512_ST_CHGRDY_SHIFT		2
#define SGM41512_ST_CHGRDY_MASK		BIT(2)
#define SGM41512_REG_CHGDONE SGM41512_REG_ICSTAT
#define SGM41512_ST_CHGDONE_SHIFT		SGM41512_ICSTAT_SHIFT
#define SGM41512_ST_CHGDONE_MASK		SGM41512_ICSTAT_MASK

/* ========== STAT1 0x11 ============ */
#define SGM41512_REG_ST_MIVR	SGM41512_REG_0A
#define SGM41512_ST_MIVR_SHIFT	6
#define SGM41512_ST_MIVR_MASK	BIT(6)
#define SGM41512_REG_ST_AICR	SGM41512_REG_0A
#define SGM41512_ST_AICR_MASK	BIT(5)
#define SGM41512_REG_ST_BATOVP	SGM41512_REG_09
#define SGM41512_ST_BATOVP_SHIFT	3
#define SGM41512_ST_BATOVP_MASK	BIT(3)

/* ========== STAT2 0x12 ============ */
#define SGM41512_REG_SYSMIN	SGM41512_REG_08
#define SGM41512_ST_SYSMIN_SHIFT	0

/* ========== STAT3 0x13 ============ */
#define SGM41512_REG_VACOV	SGM41512_REG_0A
#define SGM41512_ST_VACOV_SHIFT	2
#define SGM41512_ST_VACOV_MASK	BIT(2)

/*

REG0A Register Details

D[1] VINDPM_INT_MASK
VINDPM Event Detection Interrupt Mask
0 = Allow VINDPM INT pulse
1 = Mask VINDPM INT pulse

0 R/W REG_RST
D[0] IINDPM_INT_MASK
IINDPM Event Detection Mask
0 = Allow IINDPM to send INT pulse
1 = Mask IINDPM INT pulse
*/

#define SGM41512_REG_VINDPM	SGM41512_REG_0A
#define SGM41512_VINDPM_SHIFT	1
#define SGM41512_VINDPM_MASK	BIT(1)

#define SGM41512_REG_IINDPM	SGM41512_REG_0A
#define SGM41512_IINDPM_SHIFT	0
#define SGM41512_IINDPM_MASK	BIT(0)

#define SGM41512_VINDPM_IINDPM_MASK	 (BIT(1) | BIT(0))

#endif /* __SGM41512_CHARGER_H */
