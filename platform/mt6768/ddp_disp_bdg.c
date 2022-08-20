/* Copyright Statement:
*
* This software/firmware and related documentation ("MediaTek Software") are
* protected under relevant copyright laws. The information contained herein
* is confidential and proprietary to MediaTek Inc. and/or its licensors.
* Without the prior written permission of MediaTek inc. and/or its licensors,
* any reproduction, modification, use or disclosure of MediaTek Software,
* and information contained herein, in whole or in part, shall be strictly prohibited.
*/
/* MediaTek Inc. (C) 2021. All rights reserved.
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

#include <string.h>
#include <platform/ddp_disp_bdg.h>
#include <platform/ddp_reg_disp_bdg.h>
#include <platform/disp_drv_log.h>
#include <platform/ddp_reg.h>
#include <platform/mt6382.h>
#include "lcm_drv.h"

#define SPI_EN
struct BDG_SYSREG_CTRL_REGS *SYS_REG;		/* 0x00000000 */
struct BDG_TOPCKGEN_REGS *TOPCKGEN;		/* 0x00003000 */
struct BDG_APMIXEDSYS_REGS *APMIXEDSYS;		/* 0x00004000 */
struct BDG_GPIO_REGS *GPIO;			/* 0x00007000 */
struct BDG_EFUSE_REGS *EFUSE;			/* 0x00009000 */
struct BDG_MIPIDSI2_REGS *DSI2_REG;		/* 0x0000d000 */
struct BDG_GCE_REGS *GCE_REG;			/* 0x00010000 */
struct BDG_OCLA_REGS *OCLA_REG;			/* 0x00014000 */
struct BDG_DISP_DSC_REGS *DSC_REG;		/* 0x00020000 */
struct BDG_TX_REGS *TX_REG[HW_NUM];		/* 0x00021000 */
struct DSI_TX_CMDQ_REGS *TX_CMDQ_REG[HW_NUM];	/* 0x00021d00 */
struct BDG_MIPI_TX_REGS *MIPI_TX_REG;		/* 0x00022000 */
struct BDG_DISPSYS_CONFIG_REGS *DISPSYS_REG;	/* 0x00023000 */
struct BDG_RDMA0_REGS *RDMA_REG;		/* 0x00024000 */
struct BDG_MUTEX_REGS *MUTEX_REG;		/* 0x00025000 */
struct DSI_TX_PHY_TIMCON0_REG timcon0;
struct DSI_TX_PHY_TIMCON1_REG timcon1;
struct DSI_TX_PHY_TIMCON2_REG timcon2;
struct DSI_TX_PHY_TIMCON3_REG timcon3;

unsigned int bg_tx_data_phy_cycle;
unsigned int tx_data_rate, ap_tx_data_rate;

unsigned int hsa_byte, hbp_byte, hfp_byte;
unsigned int bllp_byte, bg_tx_line_cycle;

unsigned int dsc_en;
unsigned int mt6382_init;
int mt6382_connected;
unsigned int bdg_tx_mode;

#define T_DCO		5  /* nominal: 200MHz */
int hsrx_clk_div;
int hs_thssettle, fjump_deskew_reg, eye_open_deskew_reg;
int cdr_coarse_trgt_reg, en_dly_deass_thresh_reg;
int max_phase, dll_fbk, coarse_bank, sel_fast;
int post_rcvd_rst_val, post_det_dly_thresh_val;
unsigned int post_rcvd_rst_reg, post_det_dly_thresh_reg;
unsigned int ddl_cntr_ref_reg;

#define REGFLAG_DELAY		0xFFFC
#define REGFLAG_UDELAY		0xFFFB
#define REGFLAG_END_OF_TABLE	0xFFFD
#define REGFLAG_RESET_LOW	0xFFFE
#define REGFLAG_RESET_HIGH	0xFFFF

struct lcm_setting_table {
	unsigned int cmd;
	unsigned char count;
	unsigned char para_list[64];
};

#define MM_CLK			546
#define NS_TO_CYCLE(n, c) ((n) / (c) + (((n) % (c)) ? 1 : 0))

#define DSI_MODULE_to_ID(x)	\
	(x == DISP_BDG_DSI0 ? 0 : 1)
#define DSI_MODULE_BEGIN(x)	(\
	x == DISP_BDG_DSIDUAL ? \
	0 : DSI_MODULE_to_ID(x))
#define DSI_MODULE_END(x) \
	(x == DISP_BDG_DSIDUAL ? \
	1 : DSI_MODULE_to_ID(x))

/* #define SW_EARLY_PORTING */
unsigned int mtk_spi_read(u32 addr)
{
	unsigned int value = 0;

#ifdef SW_EARLY_PORTING
#else
	spislv_read(addr, &value, 4);
#endif
/*
 * DISPMSG("%s, addr=0x%08x, value=0x%08x\n", __func__, addr, value);
*/
	return value;
}

int mtk_spi_write(u32 addr, unsigned int regval)
{
	unsigned int value = regval;
	int ret = 0;
/*
 * DISPMSG("mt6382, %s, addr=0x%08x, value=0x%x\n",
 *	__func__, addr, value);
*/
#ifdef SW_EARLY_PORTING
#else
	ret = spislv_write(addr, &value, 4);

#endif
	return ret;
}
int mtk_spi_mask_write_1(u32 addr, u32 msk, u32 value)
{
/*
 * DISPMSG("mt6382, %s, addr=0x%08x, msk=0x%08x, value=0x%x\n",
 *	__func__, addr, msk, value);
*/
#ifdef SW_EARLY_PORTING
	return 0;
#else
	spislv_write_register_mask(addr, value, msk);
	return 1;
#endif
}

int mtk_spi_mask_write(u32 addr, u32 msk, u32 value)
{
	unsigned int i = 0;
	unsigned int temp;

	for (i = 0; i < 32; i++) {
		temp = 1 << i;
		if ((msk & temp) == temp)
			break;
	}
	value = value << i;
/*
 * DISPMSG("mt6382, %s, i=%02d, temp=0x%08x, addr=0x%08x,
 *	msk=0x%08x, value=0x%x\n",
 *	__func__, i, temp, addr, msk, value);
*/
#ifdef SW_EARLY_PORTING
	return 0;
#else
	spislv_write_register_mask(addr, value, msk);
	return 1;
#endif
}

int mtk_spi_mask_field_write(u32 addr, u32 msk, u32 value)
{
	unsigned int i = 0;
	unsigned int temp;

	for (i = 0; i < 32; i++) {
		temp = 1 << i;
		if ((msk & temp) == temp)
			break;
	}
	value = value << i;
/*
 * DISPMSG("mt6382, %s, i=%02d, temp=0x%08x, addr=0x%08x,
 *	msk=0x%08x, value=0x%x\n",
 *	__func__, i, temp, addr, msk, value);
*/
#ifdef SW_EARLY_PORTING
	return 0;
#else
	spislv_write_register_mask(addr, value, msk);
	return 0;
#endif
}

#ifdef SPI_EN

#define DSI_OUTREG32(cmdq, addr, val) \
	mtk_spi_write((unsigned long)(&addr), val)


#define DSI_MASKREG32(cmdq, addr, mask, val) \
	mtk_spi_mask_write((unsigned long)(addr), mask, val)
#else

#define DSI_OUTREG32(spi_en, cmdq, addr, val) \
{ \
	if (spi_en) { \
		mtk_spi_write((unsigned long)(&addr), val); \
	} \
	else { \
		DISP_REG_SET(cmdq, addr, val); \
	} \
}

#define DSI_MASKREG32(spi_en, cmdq, addr, mask, val) \
{ \
	if (spi_en) { \
		mtk_spi_mask_write((unsigned long)(addr), mask, val); \
	} \
	else { \
		DISP_REG_SET(cmdq, addr, val); \
	} \
}
#endif

bool bdg_is_bdg_connected(void)
{
	DISPFUNCSTART();
	if (mt6382_connected == 0) {
		unsigned long CHIP_ID_REG = 0x0;
		unsigned int ret = 0;

#ifdef CONFIG_MTK_MT6382_BDG
	spislv_init();
	spislv_switch_speed_hz(SPI_TX_LOW_SPEED_HZ,
			SPI_RX_LOW_SPEED_HZ);
	ret = mtk_spi_read(0x0);
#endif
		if (ret == 0)
			mt6382_connected = -1;
		else
			mt6382_connected = 1;
	}
	if (mt6382_connected > 0)
		return true;
	else
		return false;
}

static void bdg_get_chip_id(void)
{

}

void bdg_test_write_spi(void)
{
	int i = 0;

	for (i = 0; i < 1000; i++)
		DSI_OUTREG32(NULL, TX_REG[0]->DSI_TX_SELF_PAT_CON0, 0x11);
}
void bdg_tx_pull_lcm_reset_pin(void)
{
	bdg_tx_set_lcm_reset_pin(1);
	mdelay(10);
	bdg_tx_set_lcm_reset_pin(0);
	mdelay(10);
	bdg_tx_set_lcm_reset_pin(1);
	mdelay(10);

}
void bdg_tx_set_lcm_reset_pin(unsigned int value)
{
	int rst_pin;

	rst_pin = lcm_util_get_pin("mtkfb_pins_lcm_rst_out0_gpio");
	if (rst_pin < 0) {
		OUTREG32(MMSYS_CONFIG_BASE+0x150, value);
		dprintf(0, "reg set lcm rst pin\n");
	} else {
		dprintf(0, "gpio set lcm rst pin\n");
		mt_set_gpio_mode(rst_pin, GPIO_MODE_00);
		mt_set_gpio_dir(rst_pin, GPIO_DIR_OUT);
		if (value)
			mt_set_gpio_out(rst_pin, GPIO_OUT_ONE);
		else
			mt_set_gpio_out(rst_pin, GPIO_OUT_ZERO);
	}
}

void bdg_tx_pull_6382_reset_pin(void)
{
	bdg_tx_set_6382_reset_pin(1);
	mdelay(10);
	bdg_tx_set_6382_reset_pin(0);
	mdelay(10);
	bdg_tx_set_6382_reset_pin(1);
	mdelay(10);
}
void bdg_tx_set_6382_reset_pin(unsigned int value)
{
	int rst_pin;

	rst_pin = lcm_util_get_pin("6382_rst_out0_gpio");
	if (rst_pin < 0) {
		OUTREG32(MMSYS_CONFIG_BASE+0x150, value);
		dprintf(0, "reg set lcm rst pin\n");
	} else {
		dprintf(0, "gpio set lcm rst pin\n");
		mt_set_gpio_mode(rst_pin, GPIO_MODE_00);
		mt_set_gpio_dir(rst_pin, GPIO_DIR_OUT);
		if (value)
			mt_set_gpio_out(rst_pin, GPIO_OUT_ONE);
		else
			mt_set_gpio_out(rst_pin, GPIO_OUT_ZERO);
	}
}

void bdg_tx_set_test_pattern(void)
{
	DSI_OUTREG32(NULL, TX_REG[0]->DSI_TX_SELF_PAT_CON0, 0x11);
	DSI_OUTREG32(NULL, TX_REG[0]->DSI_TX_INTSTA, 0x0);
}

struct lcm_setting_table nt36672c_60hz[] = {
	{0xFF, 1, {0x10} },
	{0xFB, 1, {0x01} },
/* DSC on */
	{0xC0, 1, {0x03} },
	{0xC1, 16, {0x89, 0x28, 0x00, 0x08, 0x00, 0xAA,
		0x02, 0x0E,	0x00, 0x2B, 0x00, 0x07, 0x0D,
		0xB7, 0x0C, 0xB7} },
	{0xC2, 2, {0x1B, 0xA0} },

	{0xFF, 1, {0x20} },
	{0xFB, 1, {0x01} },
	{0x01, 1, {0x66} },
	{0x32, 1, {0x4D} },
	{0x69, 1, {0xD1} },
	{0xF2, 1, {0x64} },
	{0xF4, 1, {0x64} },
	{0xF6, 1, {0x64} },
	{0xF9, 1, {0x64} },

	{0xFF, 1, {0x26} },
	{0xFB, 1, {0x01} },
	{0x81, 1, {0x0E} },
	{0x84, 1, {0x03} },
	{0x86, 1, {0x03} },
	{0x88, 1, {0x07} },

	{0xFF, 1, {0x27} },
	{0xFB, 1, {0x01} },
	{0xE3, 1, {0x01} },
	{0xE4, 1, {0xEC} },
	{0xE5, 1, {0x02} },
	{0xE6, 1, {0xE3} },
	{0xE7, 1, {0x01} },
	{0xE8, 1, {0xEC} },
	{0xE9, 1, {0x02} },
	{0xEA, 1, {0x22} },
	{0xEB, 1, {0x03} },
	{0xEC, 1, {0x32} },
	{0xED, 1, {0x02} },
	{0xEE, 1, {0x22} },

	{0xFF, 1, {0x2A} },
	{0xFB, 1, {0x01} },
	{0x0C, 1, {0x04} },
	{0x0F, 1, {0x01} },
	{0x11, 1, {0xE0} },
	{0x15, 1, {0x0E} },
	{0x16, 1, {0x78} },
	{0x19, 1, {0x0D} },
	{0x1A, 1, {0xF4} },
	{0x37, 1, {0X6E} },
	{0x88, 1, {0X76} },

	{0xFF, 1, {0x2C} },
	{0xFB, 1, {0x01} },
	{0x4D, 1, {0x1E} },
	{0x4E, 1, {0x04} },
	{0x4F, 1, {0X00} },
	{0x9D, 1, {0X1E} },
	{0x9E, 1, {0X04} },

	{0xFF, 1, {0xF0} },
	{0xFB, 1, {0x01} },
	{0x5A, 1, {0x00} },

	{0xFF, 1, {0xE0} },
	{0xFB, 1, {0x01} },
	{0x25, 1, {0x02} },
	{0x4E, 1, {0x02} },
	{0x85, 1, {0x02} },

	{0xFF, 1, {0XD0} },
	{0xFB, 1, {0x01} },
	{0X09, 1, {0XAD} },

	{0xFF, 1, {0X20} },
	{0xFB, 1, {0x01} },
	{0XF8, 1, {0X64} },

	{0xFF, 1, {0X2A} },
	{0xFB, 1, {0x01} },
	{0X1A, 1, {0XF0} },
	{0x30, 1, {0x5E} },
	{0x31, 1, {0xCA} },
	{0x34, 1, {0xFE} },
	{0x35, 1, {0x35} },
	{0x36, 1, {0xA2} },
	{0x37, 1, {0xF8} },
	{0x38, 1, {0x37} },
	{0x39, 1, {0xA0} },
	{0x3A, 1, {0x5E} },
	{0x53, 1, {0xD7} },
	{0x88, 1, {0x72} },
	{0x88, 1, {0x72} },

	{0xFF, 1, {0x24} },
	{0xFB, 1, {0x01} },
	{0xC6, 1, {0xC0} },

	{0xFF, 1, {0xE0} },
	{0xFB, 1, {0x01} },
	{0x25, 1, {0x00} },
	{0x4E, 1, {0x02} },
	{0x35, 1, {0x82} },

	{0xFF, 1, {0xC0} },
	{0xFB, 1, {0x01} },
	{0x9C, 1, {0x11} },
	{0x9D, 1, {0x11} },
/* 60HZ VESA DSC */
	{0xFF, 1, {0x25} },
	{0xFB, 1, {0x01} },
	{0x18, 1, {0x22} },

/* CCMRUN */
	{0xFF, 1, {0x10} },
	{0xFB, 1, {0x01} },
	{0xC0, 1, {0x03} },
	{0x51, 1, {0x00} },
	{0x35, 1, {0x00} },
	{0x53, 1, {0x24} },
	{0x55, 1, {0x00} },
	{0xFF, 1, {0x10} },
	{0x11, 0, {} },
	{REGFLAG_DELAY, 120, {} },

/* Display On */
	{0x29, 0, {} },
};

struct lcm_setting_table nt36672c_90hz[] = {
	{0xFF, 1, {0x10} },
	{0xFB, 1, {0x01} },

/* DSC on */
	{0xC0, 1, {0x03} },
	{0xC1, 16, {0x89, 0x28, 0x00, 0x08, 0x00, 0xAA,
		0x02, 0x0E,	0x00, 0x2B, 0x00, 0x07, 0x0D,
		0xB7, 0x0C, 0xB7} },
	{0xC2, 2, {0x1B, 0xA0} },

	{0xFF, 1, {0x20} },
	{0xFB, 1, {0x01} },
	{0x01, 1, {0x66} },
	{0x32, 1, {0x4D} },
	{0x69, 1, {0xD1} },
	{0xF2, 1, {0x64} },
	{0xF4, 1, {0x64} },
	{0xF6, 1, {0x64} },
	{0xF9, 1, {0x64} },

	{0xFF, 1, {0x26} },
	{0xFB, 1, {0x01} },
	{0x81, 1, {0x0E} },
	{0x84, 1, {0x03} },
	{0x86, 1, {0x03} },
	{0x88, 1, {0x07} },

	{0xFF, 1, {0x27} },
	{0xFB, 1, {0x01} },
	{0xE3, 1, {0x01} },
	{0xE4, 1, {0xEC} },
	{0xE5, 1, {0x02} },
	{0xE6, 1, {0xE3} },
	{0xE7, 1, {0x01} },
	{0xE8, 1, {0xEC} },
	{0xE9, 1, {0x02} },
	{0xEA, 1, {0x22} },
	{0xEB, 1, {0x03} },
	{0xEC, 1, {0x32} },
	{0xED, 1, {0x02} },
	{0xEE, 1, {0x22} },

	{0xFF, 1, {0x2A} },
	{0xFB, 1, {0x01} },
	{0x0C, 1, {0x04} },
	{0x0F, 1, {0x01} },
	{0x11, 1, {0xE0} },
	{0x15, 1, {0x0E} },
	{0x16, 1, {0x78} },
	{0x19, 1, {0x0D} },
	{0x1A, 1, {0xF4} },
	{0x37, 1, {0X6E} },
	{0x88, 1, {0X76} },

	{0xFF, 1, {0x2C} },
	{0xFB, 1, {0x01} },
	{0x4D, 1, {0x1E} },
	{0x4E, 1, {0x04} },
	{0x4F, 1, {0X00} },
	{0x9D, 1, {0X1E} },
	{0x9E, 1, {0X04} },

	{0xFF, 1, {0xF0} },
	{0xFB, 1, {0x01} },
	{0x5A, 1, {0x00} },

	{0xFF, 1, {0xE0} },
	{0xFB, 1, {0x01} },
	{0x25, 1, {0x02} },
	{0x4E, 1, {0x02} },
	{0x85, 1, {0x02} },

	{0xFF, 1, {0XD0} },
	{0xFB, 1, {0x01} },
	{0X09, 1, {0XAD} },

	{0xFF, 1, {0X20} },
	{0xFB, 1, {0x01} },
	{0XF8, 1, {0X64} },

	{0xFF, 1, {0X2A} },
	{0xFB, 1, {0x01} },
	{0X1A, 1, {0XF0} },
	{0x30, 1, {0x5E} },
	{0x31, 1, {0xCA} },
	{0x34, 1, {0xFE} },
	{0x35, 1, {0x35} },
	{0x36, 1, {0xA2} },
	{0x37, 1, {0xF8} },
	{0x38, 1, {0x37} },
	{0x39, 1, {0xA0} },
	{0x3A, 1, {0x5E} },
	{0x53, 1, {0xD7} },
	{0x88, 1, {0x72} },
	{0x88, 1, {0x72} },

	{0xFF, 1, {0x24} },
	{0xFB, 1, {0x01} },
	{0xC6, 1, {0xC0} },

	{0xFF, 1, {0xE0} },
	{0xFB, 1, {0x01} },
	{0x25, 1, {0x00} },
	{0x4E, 1, {0x02} },
	{0x35, 1, {0x82} },

	{0xFF, 1, {0xC0} },
	{0xFB, 1, {0x01} },
	{0x9C, 1, {0x11} },
	{0x9D, 1, {0x11} },

/* 90HZ VESA DSC */
	{0xFF, 1, {0x25} },
	{0xFB, 1, {0x01} },
	{0x18, 1, {0x21} },

/* CCMRUN */
	{0xFF, 1, {0x10} },
	{0xFB, 1, {0x01} },
	{0xC0, 1, {0x03} },
	{0x51, 1, {0x00} },
	{0x35, 1, {0x00} },
	{0x53, 1, {0x24} },
	{0x55, 1, {0x00} },
	{0xFF, 1, {0x10} },
	{0x11, 0, {} },
	{REGFLAG_DELAY, 120, {} },

/* Display On*/
	{0x29, 0, {} },
};

struct lcm_setting_table nt36672c_120hz[] = {
	{0xFF, 1, {0x10} },
	{0xFB, 1, {0x01} },
	/*DSC on*/
	{0xC0, 1, {0x03} },
	{0xC1, 16, {0x89, 0x28, 0x00, 0x08, 0x00,
		0xAA, 0x02, 0x0E, 0x00, 0x2B, 0x00,
		0x07, 0x0D, 0xB7, 0x0C, 0xB7} },
	{0xC2, 2, {0x1B, 0xA0} },

	{0xFF, 1, {0x20} },
	{0xFB, 1, {0x01} },
	{0x01, 1, {0x66} },
	{0x32, 1, {0x4D} },
	{0x69, 1, {0xD1} },
	{0xF2, 1, {0x64} },
	{0xF4, 1, {0x64} },
	{0xF6, 1, {0x64} },
	{0xF9, 1, {0x64} },

	{0xFF, 1, {0x26} },
	{0xFB, 1, {0x01} },
	{0x81, 1, {0x0E} },
	{0x84, 1, {0x03} },
	{0x86, 1, {0x03} },
	{0x88, 1, {0x07} },

	{0xFF, 1, {0x27} },
	{0xFB, 1, {0x01} },
	{0xE3, 1, {0x01} },
	{0xE4, 1, {0xEC} },
	{0xE5, 1, {0x02} },
	{0xE6, 1, {0xE3} },
	{0xE7, 1, {0x01} },
	{0xE8, 1, {0xEC} },
	{0xE9, 1, {0x02} },
	{0xEA, 1, {0x22} },
	{0xEB, 1, {0x03} },
	{0xEC, 1, {0x32} },
	{0xED, 1, {0x02} },
	{0xEE, 1, {0x22} },

	{0xFF, 1, {0x2A} },
	{0xFB, 1, {0x01} },
	{0x0C, 1, {0x04} },
	{0x0F, 1, {0x01} },
	{0x11, 1, {0xE0} },
	{0x15, 1, {0x0E} },
	{0x16, 1, {0x78} },
	{0x19, 1, {0x0D} },
	{0x1A, 1, {0xF4} },
	{0x37, 1, {0X6E} },
	{0x88, 1, {0X76} },

	{0xFF, 1, {0x2C} },
	{0xFB, 1, {0x01} },
	{0x4D, 1, {0x1E} },
	{0x4E, 1, {0x04} },
	{0x4F, 1, {0X00} },
	{0x9D, 1, {0X1E} },
	{0x9E, 1, {0X04} },

	{0xFF, 1, {0xF0} },
	{0xFB, 1, {0x01} },
	{0x5A, 1, {0x00} },

	{0xFF, 1, {0xE0} },
	{0xFB, 1, {0x01} },
	{0x25, 1, {0x02} },
	{0x4E, 1, {0x02} },
	{0x85, 1, {0x02} },

	{0xFF, 1, {0XD0} },
	{0xFB, 1, {0x01} },
	{0X09, 1, {0XAD} },

	{0xFF, 1, {0X20} },
	{0xFB, 1, {0x01} },
	{0XF8, 1, {0X64} },

	{0xFF, 1, {0X2A} },
	{0xFB, 1, {0x01} },
	{0X1A, 1, {0XF0} },
	{0x30, 1, {0x5E} },
	{0x31, 1, {0xCA} },
	{0x34, 1, {0xFE} },
	{0x35, 1, {0x35} },
	{0x36, 1, {0xA2} },
	{0x37, 1, {0xF8} },
	{0x38, 1, {0x37} },
	{0x39, 1, {0xA0} },
	{0x3A, 1, {0x5E} },
	{0x53, 1, {0xD7} },
	{0x88, 1, {0x72} },
	{0x88, 1, {0x72} },

	{0xFF, 1, {0x24} },
	{0xFB, 1, {0x01} },
	{0xC6, 1, {0xC0} },

	{0xFF, 1, {0xE0} },
	{0xFB, 1, {0x01} },
	{0x25, 1, {0x00} },
	{0x4E, 1, {0x02} },
	{0x35, 1, {0x82} },

	{0xFF, 1, {0xC0} },
	{0xFB, 1, {0x01} },
	{0x9C, 1, {0x11} },
	{0x9D, 1, {0x11} },

/* 120HZ VESA DSC */
	{0xFF, 1, {0x25} },
	{0xFB, 1, {0x01} },
	{0x18, 1, {0x20} },
/* CCMRUN */
	{0xFF, 1, {0x10} },
	{0xFB, 1, {0x01} },
	{0xC0, 1, {0x03} },
	{0x51, 1, {0x00} },
	{0x35, 1, {0x00} },
	{0x53, 1, {0x24} },
	{0x55, 1, {0x00} },
	{0xFF, 1, {0x10} },
	{0x11, 0, {} },
	{REGFLAG_DELAY, 120, {} },
/* Display On*/
	{0x29, 0, {} },
};

struct lcm_setting_table nt36672c_wo_dsc[] = {
	{0xC0, 1, {0x00} },
};

void set_LDO_on(void *cmdq)
{
	unsigned int reg1, reg2, reg3;
	unsigned int timeout = 100;

	while (timeout) {
		reg1 = (mtk_spi_read((unsigned long)
				(&SYS_REG->LDO_STATUS)) & 0x300);
		DISPMSG("mt6382, %s, LDO_STATUS=0x%x\n", __func__, reg1);

		if (reg1 == 0x300)
			break;

		udelay(10);
		timeout--;
	}

	if (timeout == 0)
		return;
	reg1 = (mtk_spi_read((unsigned long)
			(&EFUSE->STATUS)) & 0x7fffff);
	DISPMSG("mt6382, %s, EFUSE_STATUS=0x%x\n", __func__, reg1);
	if (reg1 != 0) {
		reg1 = (mtk_spi_read((unsigned long)(&EFUSE->TRIM1)) & 0x3f);
		reg2 = (mtk_spi_read((unsigned long)(&EFUSE->TRIM2)) & 0xf);
		reg3 = (mtk_spi_read((unsigned long)(&EFUSE->TRIM3)) & 0xf);
		DISPMSG("mt6382, %s, TRIM1=0x%x, TRIM2=0x%x, TRIM3=0x%x\n",
			__func__, reg1, reg2, reg3);
		if ((reg1 != 0) || (reg2 != 0) || (reg2 != 0)) {
			mtk_spi_mask_write((u32)(DISPSYS_BDG_SYSREG_CTRL_BASE +
				REG_SYSREG_LDO_CTRL0),
				SYSREG_LDO_CTRL0_FLD_RG_PHYLDO_MASKB, 1);
		}
	}
}

void set_LDO_off(void *cmdq)
{
	DISPFUNCSTART();

	mtk_spi_mask_write((u32)(DISPSYS_BDG_SYSREG_CTRL_BASE +
		REG_SYSREG_LDO_CTRL1),
		SYSREG_LDO_CTRL1_FLD_RG_PHYLDO2_EN, 0);
	mtk_spi_mask_write((u32)(DISPSYS_BDG_SYSREG_CTRL_BASE +
		REG_SYSREG_LDO_CTRL1),
		SYSREG_LDO_CTRL1_FLD_RG_PHYLDO1_LP_EN, 1);
	DISPFUNCEND();
}

void set_mtcmos_on(void *cmdq)
{
	unsigned int reg = 0;

	DISPFUNCSTART();
	mtk_spi_mask_write((u32)(DISPSYS_BDG_SYSREG_CTRL_BASE +
		REG_SYSREG_PWR_CTRL),
		SYSREG_PWR_CTRL_FLD_REG_PWR_ON, 1);
	mtk_spi_mask_write((u32)(DISPSYS_BDG_SYSREG_CTRL_BASE +
		REG_SYSREG_PWR_CTRL),
		SYSREG_PWR_CTRL_FLD_REG_PWR_ON_2ND, 1);
	mtk_spi_mask_write((u32)(DISPSYS_BDG_SYSREG_CTRL_BASE +
		REG_SYSREG_RST_CTRL),
		SYSREG_RST_CTRL_FLD_REG_SW_RST_EN_DISP_PWR_WRAP, 0);
	mtk_spi_mask_write((u32)(DISPSYS_BDG_SYSREG_CTRL_BASE +
		REG_SYSREG_PWR_CTRL),
		SYSREG_PWR_CTRL_FLD_REG_PISO_EN, 0);
	mtk_spi_mask_write((u32)(DISPSYS_BDG_SYSREG_CTRL_BASE +
		REG_SYSREG_PWR_CTRL),
		SYSREG_PWR_CTRL_FLD_REG_DISP_PWR_CLK_DIS, 0);
	mtk_spi_mask_write((u32)(DISPSYS_BDG_SYSREG_CTRL_BASE +
		REG_SYSREG_PWR_CTRL),
		SYSREG_PWR_CTRL_FLD_REG_SYSBUF_SRAM_SLEEP_B, 1);
	mtk_spi_mask_write((u32)(DISPSYS_BDG_SYSREG_CTRL_BASE +
		REG_SYSREG_PWR_CTRL),
		SYSREG_PWR_CTRL_FLD_REG_GCE_SRAM_SLEEP_B, 1);

	reg = mtk_spi_read((unsigned long)(&SYS_REG->SYSREG_PWR_CTRL));
	reg = reg | 0x00880000;
	reg = reg & 0xffbbefff;
	DSI_OUTREG32(cmdq, SYS_REG->SYSREG_PWR_CTRL, reg);

	DISPFUNCEND();
}

void set_mtcmos_off(void *cmdq)
{
	unsigned int reg = 0;

	DISPFUNCSTART();

	reg = mtk_spi_read((unsigned long)(&SYS_REG->SYSREG_PWR_CTRL));
	reg = reg & 0x00000fff;
	reg = reg | 0x00661000;
	DSI_OUTREG32(cmdq, SYS_REG->SYSREG_PWR_CTRL, reg);

	reg = mtk_spi_read((unsigned long)(&SYS_REG->SYSREG_PWR_CTRL));
	reg = reg & 0xffddffff;
	DSI_OUTREG32(cmdq, SYS_REG->SYSREG_PWR_CTRL, reg);

	mtk_spi_mask_write((u32)(DISPSYS_BDG_SYSREG_CTRL_BASE +
		REG_SYSREG_PWR_CTRL),
		SYSREG_PWR_CTRL_FLD_REG_DISP_PWR_CLK_DIS, 1);
	mtk_spi_mask_write((u32)(DISPSYS_BDG_SYSREG_CTRL_BASE +
		REG_SYSREG_PWR_CTRL),
		SYSREG_PWR_CTRL_FLD_REG_PISO_EN, 1);
	mtk_spi_mask_write((u32)(DISPSYS_BDG_SYSREG_CTRL_BASE +
		REG_SYSREG_RST_CTRL),
		SYSREG_RST_CTRL_FLD_REG_SW_RST_EN_DISP_PWR_WRAP, 1);
	mtk_spi_mask_write((u32)(DISPSYS_BDG_SYSREG_CTRL_BASE +
		REG_SYSREG_PWR_CTRL),
		SYSREG_PWR_CTRL_FLD_REG_PWR_ON_2ND, 0);
	mtk_spi_mask_write((u32)(DISPSYS_BDG_SYSREG_CTRL_BASE +
		REG_SYSREG_PWR_CTRL),
		SYSREG_PWR_CTRL_FLD_REG_PWR_ON, 0);

	DISPFUNCEND();
}

void set_pll_on(void *cmdq)
{
	DISPFUNCSTART();
}

void set_pll_off(void *cmdq)
{
	DISPFUNCSTART();
}

void ana_macro_on(void *cmdq)
{
	unsigned int reg = 0;

	DISPFUNCSTART();
/*select pll power on `MAINPLL_CON3*/
	DSI_OUTREG32(cmdq, APMIXEDSYS->MAINPLL_CON3, 3);
	udelay(1);

	DSI_OUTREG32(cmdq, APMIXEDSYS->MAINPLL_CON3, 1);
	DSI_OUTREG32(cmdq, APMIXEDSYS->MAINPLL_CON0, 0xff000000);
	DSI_OUTREG32(cmdq, APMIXEDSYS->MAINPLL_CON1, 0x000fc000);
	udelay(2);

	DSI_OUTREG32(cmdq, APMIXEDSYS->MAINPLL_CON0, 0xff000001);
	udelay(25);

	DSI_OUTREG32(cmdq, APMIXEDSYS->MAINPLL_CON0, 0xff800001);

	DSI_OUTREG32(cmdq, TOPCKGEN->CLK_CFG_0_CLR, 0xffffffff);

/*set clock source*/
	reg = (3 << 24) | (3 << 16) | (1 << 8) | (1 << 0);
	DSI_OUTREG32(cmdq, TOPCKGEN->CLK_CFG_0_SET, reg);

/*config update*/
	reg = (1 << 4) | (1 << 3) | (1 << 1) | (1 << 0);
	DSI_OUTREG32(cmdq, TOPCKGEN->CLK_CFG_UPDATE, reg);

	udelay(6);

	DISPFUNCEND();
}

void ana_macro_off(void *cmdq)
{
	unsigned int reg = 0;

	DISPFUNCSTART();

	DSI_OUTREG32(cmdq, DSI2_REG->DSI2_DEVICE_PHY_SHUTDOWNZ_OS, 0);
	DSI_OUTREG32(cmdq, DSI2_REG->DSI2_DEVICE_PHY_RSTZ_OS, 0);

/*config update*/
	reg = (3 << 24) | (3 << 16) | (1 << 8) | (3 << 0);
	DSI_OUTREG32(cmdq, TOPCKGEN->CLK_CFG_0_CLR, reg);
	reg = (1 << 4) | (1 << 3) | (1 << 1) | (1 << 0);
	DSI_OUTREG32(cmdq, TOPCKGEN->CLK_CFG_UPDATE, reg);

/*#5000ns wait clk mux stable*/
	udelay(6);
	set_pll_off(cmdq);
}

void set_topck_on(void *cmdq)
{
	unsigned int reg = 0;

	DISPFUNCSTART();

	reg = mtk_spi_read((unsigned long)(&TOPCKGEN->CLK_MODE));
	reg = reg & 0xfffffffd;
	DSI_OUTREG32(cmdq, TOPCKGEN->CLK_MODE, reg);

	reg = mtk_spi_read((unsigned long)(&TOPCKGEN->CLK_CFG_0));
	reg = reg & 0xdf7f7f7f;
	DSI_OUTREG32(cmdq, TOPCKGEN->CLK_CFG_0, reg);
	DISPFUNCEND();
}

void set_topck_off(void *cmdq)
{
	unsigned int reg = 0;

	DISPFUNCSTART();
/*select clock source*/
	DSI_OUTREG32(cmdq, TOPCKGEN->CLK_CFG_0_CLR, 0x03030103);
	DSI_OUTREG32(cmdq, TOPCKGEN->CLK_CFG_0_SET, 0);

/*config update*/
	DSI_OUTREG32(cmdq, TOPCKGEN->CLK_CFG_UPDATE, 0x0000001b);

/*#5000ns wait clk mux stable*/
	udelay(6);


	DSI_OUTREG32(cmdq, TOPCKGEN->CLK_CFG_0_CLR, 0x00808080);
	DSI_OUTREG32(cmdq, TOPCKGEN->CLK_CFG_0_SET, 0x00808080);
	DSI_OUTREG32(cmdq, TOPCKGEN->CLK_CFG_UPDATE, 0x0000001b);
	reg = mtk_spi_read((unsigned long)(&TOPCKGEN->CLK_MODE));
	reg = reg | 0x00000001;
	DSI_OUTREG32(cmdq, TOPCKGEN->CLK_MODE, reg);
}

void set_subsys_on(void *cmdq)
{
	unsigned int reg = 0;

	DISPFUNCSTART();

	reg = mtk_spi_read((unsigned long)(&GCE_REG->GCE_CTL_INT0));
	reg = reg & 0x0000ffff;
	DSI_OUTREG32(cmdq, GCE_REG->GCE_CTL_INT0, reg);

/*Clear CG*/
	DSI_OUTREG32(cmdq, DISPSYS_REG->MMSYS_CG_CON0, 0);

/*Turn off all DCM, Clock on*/
	reg = mtk_spi_read((unsigned long)(&EFUSE->DCM_ON));
	reg = reg & 0xfffffff8;
	DSI_OUTREG32(cmdq, EFUSE->DCM_ON, reg);

	reg = mtk_spi_read((unsigned long)(&GCE_REG->GCE_CTL_INT0));
	reg = reg | 0x0000ffff;
	DSI_OUTREG32(cmdq, GCE_REG->GCE_CTL_INT0, reg);

	DSI_OUTREG32(cmdq, DISPSYS_REG->MMSYS_HW_DCM_1ST_DIS0, 0xffffffff);
	DSI_OUTREG32(cmdq, DISPSYS_REG->MMSYS_HW_DCM_2ND_DIS0, 0xffffffff);

/*enable 26m clock*/
	DSI_OUTREG32(cmdq, SYS_REG->DISP_MISC1, 1);
	DISPFUNCEND();
}

void set_subsys_off(void *cmdq)
{
	unsigned int reg = 0;

	DISPFUNCSTART();

	reg = mtk_spi_read((unsigned long)(&GCE_REG->GCE_CTL_INT0));
	reg = reg | 0x00010000;
	DSI_OUTREG32(cmdq, GCE_REG->GCE_CTL_INT0, reg);

	reg = mtk_spi_read((unsigned long)(&GCE_REG->GCE_CTL_INT0));
	reg = reg & 0x0001ffff;
	DSI_OUTREG32(cmdq, GCE_REG->GCE_CTL_INT0, reg);

/*Set CG*/
	DSI_OUTREG32(cmdq, DISPSYS_REG->MMSYS_CG_CON0, 0xffffffff);
	/*Turn on all DCM, Clock off*/
	reg = mtk_spi_read((unsigned long)(&EFUSE->DCM_ON));
	reg = reg | 0x00000007;
	DSI_OUTREG32(cmdq, EFUSE->DCM_ON, reg);

	reg = mtk_spi_read((unsigned long)(&GCE_REG->GCE_CTL_INT0));
	reg = reg | 0x0000ffff;
	DSI_OUTREG32(cmdq, GCE_REG->GCE_CTL_INT0, reg);

	DSI_OUTREG32(cmdq, DISPSYS_REG->MMSYS_HW_DCM_1ST_DIS0, 0);
	DSI_OUTREG32(cmdq, DISPSYS_REG->MMSYS_HW_DCM_2ND_DIS0, 0);
}

int dsi_get_pcw(int data_rate, int pcw_ratio)
{
	int pcw, tmp, pcw_floor;

	/**
	 * PCW bit 24~30 = floor(pcw)
	 * PCW bit 16~23 = (pcw - floor(pcw))*256
	 * PCW bit 8~15 = (pcw*256 - floor(pcw)*256)*256
	 * PCW bit 0~7 = (pcw*256*256 - floor(pcw)*256*256)*256
	 */
	pcw = data_rate * pcw_ratio / 26;
	pcw_floor = data_rate * pcw_ratio % 26;
	tmp = ((pcw & 0xFF) << 24) |
		(((256 * pcw_floor / 26) & 0xFF) << 16) |
		(((256 * (256 * pcw_floor % 26) / 26) & 0xFF) << 8) |
		((256 * (256 * (256 * pcw_floor % 26) % 26) / 26) & 0xFF);

	return tmp;
}

int bdg_mipi_tx_dphy_clk_setting(enum DISP_BDG_ENUM module,
				 void *cmdq, LCM_DSI_PARAMS *dsi_params)
{
	int i = 0;
	unsigned int j = 0;
	unsigned int data_Rate;

	unsigned int pcw_ratio = 0;
	unsigned int posdiv = 0;
	unsigned int prediv = 0;

/* Delta1 is SSC range, default is 0%~-5% */
	unsigned int delta1 = 2;
	unsigned int pdelta1 = 0;
	MIPITX_PHY_LANE_SWAP *swap_base;
	enum MIPI_TX_PAD_VALUE pad_mapping[MIPITX_PHY_LANE_NUM] = {
					PAD_D0P, PAD_D1P, PAD_D2P,
					PAD_D3P, PAD_CKP, PAD_CKP};

	DISPFUNCSTART();

	if (dsi_params->data_rate != 0) {
		data_Rate = dsi_params->data_rate;
	} else if (dsi_params->PLL_CLOCK) {
		data_Rate = dsi_params->PLL_CLOCK * 2;
	} else {
		DISPMSG("PLL clock should not be 0!!!\n");
		ASSERT(0);
		return -1;
	}

	DISPMSG("%s, data_Rate=%d\n", __func__, data_Rate);

	/* DPHY SETTING */
	/* MIPITX lane swap setting */
	for (i = DSI_MODULE_BEGIN(module); i <= DSI_MODULE_END(module); i++) {
		/* step 0 MIPITX lane swap setting */
		swap_base = dsi_params->lane_swap[i];
		DISPMSG(
			"MIPITX Lane Swap mapping: 0=%d|1=%d|2=%d|3=%d|CK=%d|RX=%d\n",
			 swap_base[MIPITX_PHY_LANE_0],
			 swap_base[MIPITX_PHY_LANE_1],
			 swap_base[MIPITX_PHY_LANE_2],
			 swap_base[MIPITX_PHY_LANE_3],
			 swap_base[MIPITX_PHY_LANE_CK],
			 swap_base[MIPITX_PHY_LANE_RX]);

		if (unlikely(dsi_params->lane_swap_en)) {
			DISPMSG("MIPITX Lane Swap Enabled for DSI Port %d\n",
				 i);
			DISPMSG(
				"MIPITX Lane Swap mapping: %d|%d|%d|%d|%d|%d\n",
				 swap_base[MIPITX_PHY_LANE_0],
				 swap_base[MIPITX_PHY_LANE_1],
				 swap_base[MIPITX_PHY_LANE_2],
				 swap_base[MIPITX_PHY_LANE_3],
				 swap_base[MIPITX_PHY_LANE_CK],
				 swap_base[MIPITX_PHY_LANE_RX]);

			/* CKMODE_EN */
			for (j = MIPITX_PHY_LANE_0; j < MIPITX_PHY_LANE_CK;
			     j++) {
				if (dsi_params->lane_swap[i][j] ==
				    MIPITX_PHY_LANE_CK)
					break;
			}
			switch (j) {
			case MIPITX_PHY_LANE_0:
				mtk_spi_mask_write(
					(u32)(DISPSYS_BDG_MIPI_TX_BASE +
					REG_MIPI_TX_D0_CKMODE_EN),
					MIPI_TX_DX_FLD_DSI_CKMODE_EN, 1);
				break;
			case MIPITX_PHY_LANE_1:
				mtk_spi_mask_write(
					(u32)(DISPSYS_BDG_MIPI_TX_BASE +
					REG_MIPI_TX_D1_CKMODE_EN),
					MIPI_TX_DX_FLD_DSI_CKMODE_EN, 1);
				break;
			case MIPITX_PHY_LANE_2:
				mtk_spi_mask_write(
					(u32)(DISPSYS_BDG_MIPI_TX_BASE +
					REG_MIPI_TX_D2_CKMODE_EN),
					MIPI_TX_DX_FLD_DSI_CKMODE_EN, 1);
				break;
			case MIPITX_PHY_LANE_3:
				mtk_spi_mask_write(
					(u32)(DISPSYS_BDG_MIPI_TX_BASE +
					REG_MIPI_TX_D3_CKMODE_EN),
					MIPI_TX_DX_FLD_DSI_CKMODE_EN, 1);
				break;
			case MIPITX_PHY_LANE_CK:
				mtk_spi_mask_write(
					(u32)(DISPSYS_BDG_MIPI_TX_BASE +
					REG_MIPI_TX_CK_CKMODE_EN),
					MIPI_TX_DX_FLD_DSI_CKMODE_EN, 1);
				break;
			default:
				break;
			}

			/* LANE_0 */
			mtk_spi_mask_write(
				(u32)(DISPSYS_BDG_MIPI_TX_BASE +
				REG_MIPI_TX_PHY_SEL0),
				MIPI_TX_PHY_SEL0_FLD_MIPI_TX_PHY0_SEL,
				pad_mapping[swap_base[MIPITX_PHY_LANE_0]]);
			mtk_spi_mask_write(
				(u32)(DISPSYS_BDG_MIPI_TX_BASE +
				REG_MIPI_TX_PHY_SEL0),
				MIPI_TX_PHY_SEL0_FLD_MIPI_TX_PHY1AB_SEL,
				pad_mapping[swap_base[MIPITX_PHY_LANE_0]] + 1);

			/* LANE_1 */
			mtk_spi_mask_write(
				(u32)(DISPSYS_BDG_MIPI_TX_BASE +
				REG_MIPI_TX_PHY_SEL0),
				MIPI_TX_PHY_SEL0_FLD_MIPI_TX_PHY1_SEL,
				pad_mapping[swap_base[MIPITX_PHY_LANE_1]]);
			mtk_spi_mask_write(
				(u32)(DISPSYS_BDG_MIPI_TX_BASE +
				REG_MIPI_TX_PHY_SEL1),
				MIPI_TX_PHY_SEL1_FLD_MIPI_TX_PHY2BC_SEL,
				pad_mapping[swap_base[MIPITX_PHY_LANE_1]] + 1);

			/* LANE_2 */
			mtk_spi_mask_write(
				(u32)(DISPSYS_BDG_MIPI_TX_BASE +
				REG_MIPI_TX_PHY_SEL0),
				MIPI_TX_PHY_SEL0_FLD_MIPI_TX_PHY2_SEL,
				pad_mapping[swap_base[MIPITX_PHY_LANE_2]]);
			mtk_spi_mask_write(
				(u32)(DISPSYS_BDG_MIPI_TX_BASE +
				REG_MIPI_TX_PHY_SEL0),
				MIPI_TX_PHY_SEL0_FLD_MIPI_TX_CPHY0BC_SEL,
				pad_mapping[swap_base[MIPITX_PHY_LANE_2]] + 1);

			/* LANE_3 */
			mtk_spi_mask_write(
				(u32)(DISPSYS_BDG_MIPI_TX_BASE +
				REG_MIPI_TX_PHY_SEL1),
				MIPI_TX_PHY_SEL1_FLD_MIPI_TX_PHY3_SEL,
				pad_mapping[swap_base[MIPITX_PHY_LANE_3]]);
			mtk_spi_mask_write(
				(u32)(DISPSYS_BDG_MIPI_TX_BASE +
				REG_MIPI_TX_PHY_SEL1),
				MIPI_TX_PHY_SEL1_FLD_MIPI_TX_CPHYXXX_SEL,
				pad_mapping[swap_base[MIPITX_PHY_LANE_3]] + 1);

			/* CK LANE */
			mtk_spi_mask_write(
				(u32)(DISPSYS_BDG_MIPI_TX_BASE +
				REG_MIPI_TX_PHY_SEL0),
				MIPI_TX_PHY_SEL0_FLD_MIPI_TX_PHYC_SEL,
				pad_mapping[swap_base[MIPITX_PHY_LANE_CK]]);
			mtk_spi_mask_write(
				(u32)(DISPSYS_BDG_MIPI_TX_BASE +
				REG_MIPI_TX_PHY_SEL0),
				MIPI_TX_PHY_SEL0_FLD_MIPI_TX_CPHY1CA_SEL,
				pad_mapping[swap_base[MIPITX_PHY_LANE_CK]] + 1);

			mtk_spi_mask_write(
				(u32)(DISPSYS_BDG_MIPI_TX_BASE +
				REG_MIPI_TX_PHY_SEL1),
				MIPI_TX_PHY_SEL1_FLD_MIPI_TX_LPRX0AB_SEL,
				pad_mapping[swap_base[MIPITX_PHY_LANE_RX]]);
			mtk_spi_mask_write(
				(u32)(DISPSYS_BDG_MIPI_TX_BASE +
				REG_MIPI_TX_PHY_SEL1),
				MIPI_TX_PHY_SEL1_FLD_MIPI_TX_LPRX0BC_SEL,
				pad_mapping[swap_base[MIPITX_PHY_LANE_RX]] + 1);
			/* LPRX SETTING */

			/* HS_DATA SETTING */
			mtk_spi_mask_write(
				(u32)(DISPSYS_BDG_MIPI_TX_BASE +
				REG_MIPI_TX_PHY_SEL2),
				MIPI_TX_PHY_SEL2_FLD_MIPI_TX_PHY2_HSDATA_SEL,
				pad_mapping[swap_base[MIPITX_PHY_LANE_2]]);
			mtk_spi_mask_write(
				(u32)(DISPSYS_BDG_MIPI_TX_BASE +
				REG_MIPI_TX_PHY_SEL2),
				MIPI_TX_PHY_SEL2_FLD_MIPI_TX_PHY0_HSDATA_SEL,
				pad_mapping[swap_base[MIPITX_PHY_LANE_0]]);
			mtk_spi_mask_write(
				(u32)(DISPSYS_BDG_MIPI_TX_BASE +
				REG_MIPI_TX_PHY_SEL2),
				MIPI_TX_PHY_SEL2_FLD_MIPI_TX_PHYC_HSDATA_SEL,
				pad_mapping[swap_base[MIPITX_PHY_LANE_CK]]);
			mtk_spi_mask_write(
				(u32)(DISPSYS_BDG_MIPI_TX_BASE +
				REG_MIPI_TX_PHY_SEL2),
				MIPI_TX_PHY_SEL2_FLD_MIPI_TX_PHY1_HSDATA_SEL,
				pad_mapping[swap_base[MIPITX_PHY_LANE_1]]);
			mtk_spi_mask_write(
				(u32)(DISPSYS_BDG_MIPI_TX_BASE +
				REG_MIPI_TX_PHY_SEL3),
				MIPI_TX_PHY_SEL3_FLD_MIPI_TX_PHY3_HSDATA_SEL,
				pad_mapping[swap_base[MIPITX_PHY_LANE_3]]);

		} else {
			mtk_spi_mask_write(
				(u32)(DISPSYS_BDG_MIPI_TX_BASE +
				REG_MIPI_TX_CK_CKMODE_EN),
				MIPI_TX_DX_FLD_DSI_CKMODE_EN, 1);
		}
	}

	/* MIPI INIT */
	for (i = DSI_MODULE_BEGIN(module); i <= DSI_MODULE_END(module); i++) {
		unsigned int tmp = 0;

		/* step 0: RG_DSI0_PLL_IBIAS = 0*/
		DSI_OUTREG32(cmdq, MIPI_TX_REG->MIPI_TX_PLL_CON4, 0x00FF12E0);
		/* BG_LPF_EN / BG_CORE_EN */
		DSI_OUTREG32(cmdq, MIPI_TX_REG->MIPI_TX_LANE_CON, 0x3FFF0180);
		udelay(2);
		/* BG_LPF_EN=1,TIEL_SEL=0 */
		DSI_OUTREG32(cmdq, MIPI_TX_REG->MIPI_TX_LANE_CON, 0x3FFF0080);

		/* Switch OFF each Lane */
		mtk_spi_mask_write((u32)(DISPSYS_BDG_MIPI_TX_BASE +
			REG_MIPI_TX_D0_SW_CTL_EN),
			MIPI_TX_DX_FLD_DSI_SW_CTL_EN, 0);
		mtk_spi_mask_write((u32)(DISPSYS_BDG_MIPI_TX_BASE +
			REG_MIPI_TX_D1_SW_CTL_EN),
			MIPI_TX_DX_FLD_DSI_SW_CTL_EN, 0);
		mtk_spi_mask_write((u32)(DISPSYS_BDG_MIPI_TX_BASE +
			REG_MIPI_TX_D2_SW_CTL_EN),
			MIPI_TX_DX_FLD_DSI_SW_CTL_EN, 0);
		mtk_spi_mask_write((u32)(DISPSYS_BDG_MIPI_TX_BASE +
			REG_MIPI_TX_D3_SW_CTL_EN),
			MIPI_TX_DX_FLD_DSI_SW_CTL_EN, 0);
		mtk_spi_mask_write((u32)(DISPSYS_BDG_MIPI_TX_BASE +
			REG_MIPI_TX_CK_SW_CTL_EN),
			MIPI_TX_DX_FLD_DSI_SW_CTL_EN, 0);

		/* step 1: SDM_RWR_ON / SDM_ISO_EN */
		mtk_spi_mask_write((u32)(DISPSYS_BDG_MIPI_TX_BASE +
			REG_MIPI_TX_PLL_PWR),
			MIPI_TX_PLL_PWR_FLD_AD_DSI_PLL_SDM_PWR_ON, 1);
		mtk_spi_mask_write((u32)(DISPSYS_BDG_MIPI_TX_BASE +
			REG_MIPI_TX_PLL_PWR),
			MIPI_TX_PLL_PWR_FLD_AD_DSI_PLL_SDM_ISO_EN, 0);

		if (data_Rate > 2500) {
			DISPERR("mipitx Data Rate exceed limitation(%d)\n",
				    data_Rate);
			ASSERT(0);
			return -2;
		} else if (data_Rate >= 2000) { /* 2G ~ 2.5G */
			pcw_ratio = 1;
			posdiv    = 0;
			prediv    = 0;
		} else if (data_Rate >= 1000) { /* 1G ~ 2G */
			pcw_ratio = 2;
			posdiv    = 1;
			prediv    = 0;
		} else if (data_Rate >= 500) { /* 500M ~ 1G */
			pcw_ratio = 4;
			posdiv    = 2;
			prediv    = 0;
		} else if (data_Rate > 250) { /* 250M ~ 500M */
			pcw_ratio = 8;
			posdiv    = 3;
			prediv    = 0;
		} else if (data_Rate >= 125) { /* 125M ~ 250M */
			pcw_ratio = 16;
			posdiv    = 4;
			prediv    = 0;
		} else {
			DISPERR("dataRate is too low(%d)\n", data_Rate);
			ASSERT(0);
			return -3;
		}

		/* step 3 */
		/* PLL PCW config */
		tmp = dsi_get_pcw(data_Rate, pcw_ratio);
		DSI_OUTREG32(cmdq, MIPI_TX_REG->MIPI_TX_PLL_CON0, tmp);

		mtk_spi_mask_write((u32)(DISPSYS_BDG_MIPI_TX_BASE +
			REG_MIPI_TX_PLL_CON1),
			MIPI_TX_PLL_CON1_FLD_RG_DSI_PLL_POSDIV, posdiv);

		/* SSC config */
		if (dsi_params->ssc_disable != 1) {
			mtk_spi_mask_write((u32)(DISPSYS_BDG_MIPI_TX_BASE +
				REG_MIPI_TX_PLL_CON2),
				MIPI_TX_PLL_CON2_FLD_RG_DSI_PLL_SDM_SSC_PH_INIT,
				1);
			mtk_spi_mask_write((u32)(DISPSYS_BDG_MIPI_TX_BASE +
				REG_MIPI_TX_PLL_CON2),
				MIPI_TX_PLL_CON2_FLD_RG_DSI_PLL_SDM_SSC_PRD,
				0x1B1);

			delta1 = (dsi_params->ssc_range == 0) ?
				delta1 : dsi_params->ssc_range;
			ASSERT(delta1 <= 8);
			pdelta1 = (delta1 * (data_Rate / 2) * pcw_ratio *
				   262144 + 281664) / 563329;
			mtk_spi_mask_write((u32)(DISPSYS_BDG_MIPI_TX_BASE +
				REG_MIPI_TX_PLL_CON3),
				MIPI_TX_PLL_CON2_FLD_RG_DSI_PLL_SDM_SSC_DELTA,
				pdelta1);
			mtk_spi_mask_write((u32)(DISPSYS_BDG_MIPI_TX_BASE +
				REG_MIPI_TX_PLL_CON3),
				MIPI_TX_PLL_CON2_FLD_RG_DSI_PLL_SDM_SSC_DELTA1,
				pdelta1);
			DISPMSG("%s, PLL config:data_rate=%d, pcw_ratio=%d, delta1=%d,pdelta1=0x%x\n",
				__func__, data_Rate, pcw_ratio,
				delta1, pdelta1);
		}
	}

	for (i = DSI_MODULE_BEGIN(module); i <= DSI_MODULE_END(module); i++) {
		if (data_Rate && (dsi_params->ssc_disable != 1))
			mtk_spi_mask_write((u32)(DISPSYS_BDG_MIPI_TX_BASE +
				REG_MIPI_TX_PLL_CON2),
				MIPI_TX_PLL_CON2_FLD_RG_DSI_PLL_SDM_SSC_EN, 1);
		else
			mtk_spi_mask_write((u32)(DISPSYS_BDG_MIPI_TX_BASE +
				REG_MIPI_TX_PLL_CON2),
				MIPI_TX_PLL_CON2_FLD_RG_DSI_PLL_SDM_SSC_EN, 0);
	}

	for (i = DSI_MODULE_BEGIN(module); i <= DSI_MODULE_END(module); i++) {
		/* PLL EN */
		mtk_spi_mask_write((u32)(DISPSYS_BDG_MIPI_TX_BASE +
			REG_MIPI_TX_PLL_CON1),
			MIPI_TX_PLL_CON1_FLD_RG_DSI_PLL_EN, 1);

		udelay(50);
		mtk_spi_mask_write((u32)(DISPSYS_BDG_MIPI_TX_BASE +
			REG_MIPI_TX_SW_CTRL_CON4),
			MIPI_TX_SW_CTRL_CON4_FLD_MIPI_TX_SW_ANA_CK_EN, 1);
	}

	DISPFUNCEND();
	return 0;
}

int bdg_tx_phy_config(enum DISP_BDG_ENUM module,
			void *cmdq, LCM_DSI_PARAMS *tx_params)
{
	int i;
	u32 ui, cycle_time;
	unsigned int hs_trail;

	DISPFUNCSTART();

	if (tx_params->data_rate != 0) {
		ui = 1000 / tx_params->data_rate;
		cycle_time = 8000 / tx_params->data_rate;
		tx_data_rate = tx_params->data_rate;
	} else if (tx_params->PLL_CLOCK) {
		ui = 1000 / (tx_params->PLL_CLOCK * 2);
		cycle_time = 8000 / (tx_params->PLL_CLOCK * 2);
		tx_data_rate = tx_params->PLL_CLOCK * 2;
	} else {
		DISPMSG("PLL clock should not be 0!!!\n");
		ASSERT(0);
	}
	DISPMSG(
		"%s, tx_data_rate=%d, cycle_time=%d, ui=%d\n",
		__func__, tx_data_rate, cycle_time, ui);

#if 1
	/* lpx >= 50ns (spec) */
	/* lpx = 60ns */
	timcon0.LPX = NS_TO_CYCLE(60, cycle_time);
	if (timcon0.LPX < 2)
		timcon0.LPX = 2;

	/* hs_prep = 40ns+4*UI ~ 85ns+6*UI (spec) */
	/* hs_prep = 64ns+5*UI */
	timcon0.HS_PRPR = NS_TO_CYCLE((64 + 5 * ui), cycle_time) + 1;

	/* hs_zero = (200+10*UI) - hs_prep */
	timcon0.HS_ZERO = NS_TO_CYCLE((200 + 10 * ui), cycle_time);
	timcon0.HS_ZERO = timcon0.HS_ZERO > timcon0.HS_PRPR ?
			timcon0.HS_ZERO - timcon0.HS_PRPR : timcon0.HS_ZERO;
	if (timcon0.HS_ZERO < 1)
		timcon0.HS_ZERO = 1;

	/* hs_trail > max(8*UI, 60ns+4*UI) (spec) */
	/* hs_trail = 80ns+4*UI */
	hs_trail = 80 + 4 * ui;
	timcon0.HS_TRAIL = (hs_trail > cycle_time) ?
				NS_TO_CYCLE(hs_trail, cycle_time) + 1 : 2;

	/* hs_exit > 100ns (spec) */
	/* hs_exit = 120ns */
	/* timcon1.DA_HS_EXIT = NS_TO_CYCLE(120, cycle_time); */
	/* hs_exit = 2*lpx */
	timcon1.DA_HS_EXIT = 2 * timcon0.LPX;

	/* ta_go = 4*lpx (spec) */
	timcon1.TA_GO = 4 * timcon0.LPX;

	/* ta_get = 5*lpx (spec) */
	timcon1.TA_GET = 5 * timcon0.LPX;

	/* ta_sure = lpx ~ 2*lpx (spec) */
	timcon1.TA_SURE = 3 * timcon0.LPX / 2;

	/* clk_hs_prep = 38ns ~ 95ns (spec) */
	/* clk_hs_prep = 80ns */
	timcon3.CLK_HS_PRPR = NS_TO_CYCLE(80, cycle_time);

	/* clk_zero + clk_hs_prep > 300ns (spec) */
	/* clk_zero = 400ns - clk_hs_prep */
	timcon2.CLK_ZERO = NS_TO_CYCLE(400, cycle_time) -
				timcon3.CLK_HS_PRPR;
	if (timcon2.CLK_ZERO < 1)
		timcon2.CLK_ZERO = 1;

	/* clk_trail > 60ns (spec) */
	/* clk_trail = 100ns */
	timcon2.CLK_TRAIL = NS_TO_CYCLE(100, cycle_time) + 1;
	if (timcon2.CLK_TRAIL < 2)
		timcon2.CLK_TRAIL = 2;

	/* clk_exit > 100ns (spec) */
	/* clk_exit = 200ns */
	/* timcon3.CLK_EXIT = NS_TO_CYCLE(200, cycle_time); */
	/* clk_exit = 2*lpx */
	timcon3.CLK_HS_EXIT = 2 * timcon0.LPX;

	/* clk_post > 60ns+52*UI (spec) */
	/* clk_post = 96ns+52*UI */
	timcon3.CLK_HS_POST = NS_TO_CYCLE((96 + 52 * ui), cycle_time);
#else
	hs_trail = (tx_params->HS_TRAIL == 0) ?
				(NS_TO_CYCLE(((1 * 4 * ui) + 80)
				* tx_params->PLL_CLOCK * 2, 8000) + 1) :
				tx_params->HS_TRAIL;
	/* +3 is recommended from designer becauase of HW latency */
	timcon0.HS_TRAIL = (1 > hs_trail) ? 1 : hs_trail;

	timcon0.HS_PRPR = (tx_params->HS_PRPR == 0) ?
			(NS_TO_CYCLE((64 + 5 * ui), cycle_time) + 1) :
			tx_params->HS_PRPR;
	/* HS_PRPR can't be 1. */
	if (timcon0.HS_PRPR < 1)
		timcon0.HS_PRPR = 1;

	timcon0.HS_ZERO = (tx_params->HS_ZERO == 0) ?
				NS_TO_CYCLE((200 + 10 * ui), cycle_time) :
				tx_params->HS_ZERO;
	timcon_temp = timcon0.HS_PRPR;
	if (timcon_temp < timcon0.HS_ZERO)
		timcon0.HS_ZERO -= timcon0.HS_PRPR;

	timcon0.LPX = (tx_params->LPX == 0) ?
		(NS_TO_CYCLE(tx_params->PLL_CLOCK * 2 * 75, 8000) + 1) :
								tx_params->LPX;
	if (timcon0.LPX < 1)
		timcon0.LPX = 1;

	timcon1.TA_GET = (tx_params->TA_GET == 0) ?  (5 * timcon0.LPX) :
							tx_params->TA_GET;
	timcon1.TA_SURE = (tx_params->TA_SURE == 0) ?
				(3 * timcon0.LPX / 2) : tx_params->TA_SURE;
	timcon1.TA_GO = (tx_params->TA_GO == 0) ?  (4 * timcon0.LPX) :
							tx_params->TA_GO;
	/* --------------------------------------------------------------
	 * NT35510 need fine tune timing
	 * Data_hs_exit = 60 ns + 128UI
	 * Clk_post = 60 ns + 128 UI.
	 * --------------------------------------------------------------
	 */
	timcon1.DA_HS_EXIT = (tx_params->DA_HS_EXIT == 0) ?
				(2 * timcon0.LPX) : tx_params->DA_HS_EXIT;

	timcon2.CLK_TRAIL = ((tx_params->CLK_TRAIL == 0) ?
				NS_TO_CYCLE(100 * tx_params->PLL_CLOCK * 2,
				8000) : tx_params->CLK_TRAIL) + 1;
	/* CLK_TRAIL can't be 1. */
	if (timcon2.CLK_TRAIL < 2)
		timcon2.CLK_TRAIL = 2;

	timcon2.CLK_ZERO = (tx_params->CLK_ZERO == 0) ?
						NS_TO_CYCLE(400, cycle_time) :
						tx_params->CLK_ZERO;

	timcon3.CLK_HS_PRPR = (tx_params->CLK_HS_PRPR == 0) ?
				NS_TO_CYCLE(80 * tx_params->PLL_CLOCK * 2,
				8000) : tx_params->CLK_HS_PRPR;

	if (timcon3.CLK_HS_PRPR < 1)
		timcon3.CLK_HS_PRPR = 1;

	timcon3.CLK_HS_EXIT = (tx_params->CLK_HS_EXIT == 0) ?
				(2 * timcon0.LPX) : tx_params->CLK_HS_EXIT;
	timcon3.CLK_HS_POST = (tx_params->CLK_HS_POST == 0) ?
				NS_TO_CYCLE((96 + 52 * ui), cycle_time) :
				tx_params->CLK_HS_POST;

#endif
	bg_tx_data_phy_cycle = (timcon1.DA_HS_EXIT + 1) + timcon0.LPX +
					timcon0.HS_PRPR + timcon0.HS_ZERO + 1;

	DISPMSG(
		"%s, bg_tx_data_phy_cycle=%d, LPX=%d, HS_PRPR=%d, HS_ZERO=%d, HS_TRAIL=%d, DA_HS_EXIT=%d\n",
		__func__, bg_tx_data_phy_cycle, timcon0.LPX,
		timcon0.HS_PRPR, timcon0.HS_ZERO, timcon0.HS_TRAIL,
		timcon1.DA_HS_EXIT);

	DISPMSG(
		"%s, TA_GO=%d, TA_GET=%d, TA_SURE=%d, CLK_HS_PRPR=%d, CLK_ZERO=%d, CLK_TRAIL=%d, CLK_HS_EXIT=%d, CLK_HS_POST=%d\n",
		__func__, timcon1.TA_GO, timcon1.TA_GET, timcon1.TA_SURE,
		timcon3.CLK_HS_PRPR, timcon2.CLK_ZERO, timcon2.CLK_TRAIL,
		timcon3.CLK_HS_EXIT, timcon3.CLK_HS_POST);

	for (i = DSI_MODULE_BEGIN(module); i <= DSI_MODULE_END(module); i++) {
		mtk_spi_mask_write((u32)(DISPSYS_BDG_TX_DSI0_BASE
			+ REG_DSI_TX_PHY_TIMCON0),
			DSI_TX_PHY_TIMCON0_FLD_LPX, timcon0.LPX);
		mtk_spi_mask_write((u32)(DISPSYS_BDG_TX_DSI0_BASE
			+ REG_DSI_TX_PHY_TIMCON0),
			DSI_TX_PHY_TIMCON0_FLD_HS_PRPR, timcon0.HS_PRPR);
		mtk_spi_mask_write((u32)(DISPSYS_BDG_TX_DSI0_BASE
			+ REG_DSI_TX_PHY_TIMCON0),
			DSI_TX_PHY_TIMCON0_FLD_HS_ZERO, timcon0.HS_ZERO);
		mtk_spi_mask_write((u32)(DISPSYS_BDG_TX_DSI0_BASE
			+ REG_DSI_TX_PHY_TIMCON0),
			DSI_TX_PHY_TIMCON0_FLD_HS_TRAIL, timcon0.HS_TRAIL);

		mtk_spi_mask_write((u32)(DISPSYS_BDG_TX_DSI0_BASE
			+ REG_DSI_TX_PHY_TIMCON1),
			DSI_TX_PHY_TIMCON1_FLD_TA_GO, timcon1.TA_GO);
		mtk_spi_mask_write((u32)(DISPSYS_BDG_TX_DSI0_BASE
			+ REG_DSI_TX_PHY_TIMCON1),
			DSI_TX_PHY_TIMCON1_FLD_TA_SURE, timcon1.TA_SURE);
		mtk_spi_mask_write((u32)(DISPSYS_BDG_TX_DSI0_BASE
			+ REG_DSI_TX_PHY_TIMCON1),
			DSI_TX_PHY_TIMCON1_FLD_TA_GET, timcon1.TA_GET);
		mtk_spi_mask_write((u32)(DISPSYS_BDG_TX_DSI0_BASE
			+ REG_DSI_TX_PHY_TIMCON1),
			DSI_TX_PHY_TIMCON1_FLD_DA_HS_EXIT,
			timcon1.DA_HS_EXIT);

		mtk_spi_mask_write((u32)(DISPSYS_BDG_TX_DSI0_BASE
			+ REG_DSI_TX_PHY_TIMCON2),
			DSI_TX_PHY_TIMCON2_FLD_CLK_ZERO,
			timcon2.CLK_ZERO);
		mtk_spi_mask_write((u32)(DISPSYS_BDG_TX_DSI0_BASE
			+ REG_DSI_TX_PHY_TIMCON2),
			DSI_TX_PHY_TIMCON2_FLD_CLK_TRAIL,
			timcon2.CLK_TRAIL);

		mtk_spi_mask_write((u32)(DISPSYS_BDG_TX_DSI0_BASE
			+ REG_DSI_TX_PHY_TIMCON3),
			DSI_TX_PHY_TIMCON3_FLD_DCLK_HS_PRPR,
			timcon3.CLK_HS_PRPR);
		mtk_spi_mask_write((u32)(DISPSYS_BDG_TX_DSI0_BASE
			+ REG_DSI_TX_PHY_TIMCON3),
			DSI_TX_PHY_TIMCON3_FLD_CCLK_HS_POST,
			timcon3.CLK_HS_POST);
		mtk_spi_mask_write((u32)(DISPSYS_BDG_TX_DSI0_BASE
			+ REG_DSI_TX_PHY_TIMCON3),
			DSI_TX_PHY_TIMCON3_FLD_CCLK_HS_EXIT,
			timcon3.CLK_HS_EXIT);

		DISPMSG("%s, PHY_TIMECON0=0x%08x,PHY_TIMECON1=0x%08x,PHY_TIMECON2=0x%08x,PHY_TIMECON3=0x%08x\n",
			__func__,
			mtk_spi_read((unsigned long)
				(&TX_REG[i]->DSI_TX_PHY_TIMECON0)),
			mtk_spi_read((unsigned long)
				(&TX_REG[i]->DSI_TX_PHY_TIMECON1)),
			mtk_spi_read((unsigned long)
				(&TX_REG[i]->DSI_TX_PHY_TIMECON2)),
			mtk_spi_read((unsigned long)
				(&TX_REG[i]->DSI_TX_PHY_TIMECON3)));
	}


	return 0;
}
int bdg_tx_txrx_ctrl(enum DISP_BDG_ENUM module,
			void *cmdq, LCM_DSI_PARAMS *tx_params)
{
	int i;
	int lane_num = tx_params->LANE_NUM;
	bool hstx_cklp_en = tx_params->cont_clock ? FALSE : TRUE;
	bool dis_eotp_en = tx_params->IsCphy ? TRUE : FALSE;
	bool ext_te_en = tx_params->mode ? FALSE : TRUE;

	DISPFUNCSTART();

	switch (lane_num) {
	case ONE_LANE:
		lane_num = 0x1;
		break;
	case TWO_LANE:
		lane_num = 0x3;
		break;
	case THREE_LANE:
		lane_num = 0x7;
		break;
	case FOUR_LANE:
	default:
		lane_num = 0xF;
		break;
	}

	for (i = DSI_MODULE_BEGIN(module); i <= DSI_MODULE_END(module); i++) {
		mtk_spi_mask_write((u32)(DISPSYS_BDG_TX_DSI0_BASE
			+ REG_DSI_TX_TXRX_CON),
			DSI_TX_TXRX_CON_FLD_LANE_NUM, lane_num);
		mtk_spi_mask_write((u32)(DISPSYS_BDG_TX_DSI0_BASE
			+ REG_DSI_TX_TXRX_CON),
			DSI_TX_TXRX_CON_FLD_HSTX_CKLP_EN, hstx_cklp_en);
		mtk_spi_mask_write((u32)(DISPSYS_BDG_TX_DSI0_BASE
			+ REG_DSI_TX_TXRX_CON),
			DSI_TX_TXRX_CON_FLD_DIS_EOT, dis_eotp_en);
		mtk_spi_mask_write((u32)(DISPSYS_BDG_TX_DSI0_BASE
			+ REG_DSI_TX_TXRX_CON),
			DSI_TX_TXRX_CON_FLD_EXT_TE_EN, ext_te_en);

	}

	return 0;
}

int bdg_tx_ps_ctrl(enum DISP_BDG_ENUM module,
			void *cmdq, LCM_DSI_PARAMS *tx_params)
{
	int i;
	unsigned int ps_wc, width, bpp, ps_sel;

	DISPFUNCSTART();

	width = tx_params->horizontal_active_pixel / 1;

	switch (tx_params->data_format.format) {
	case LCM_DSI_FORMAT_RGB565:
		ps_sel = PACKED_PS_16BIT_RGB565;
		bpp = 16;
		break;
	case LCM_DSI_FORMAT_RGB666_LOOSELY:
		ps_sel = LOOSELY_PS_24BIT_RGB666;
		bpp = 24;
		break;
	case LCM_DSI_FORMAT_RGB666:
		ps_sel = PACKED_PS_18BIT_RGB666;
		bpp = 18;
		break;
	case LCM_DSI_FORMAT_RGB888:
		ps_sel = PACKED_PS_24BIT_RGB888;
		bpp = 24;
		break;
	case LCM_DSI_FORMAT_RGB101010:
		ps_sel = PACKED_PS_30BIT_RGB101010;
		bpp = 30;
		break;
	default:
		DISPMSG("format not support!!!\n");
		return -1;
	}
	if (dsc_en) {
		ps_sel = PACKED_COMPRESSION;
		width = (width + 2) / 3;
	}
	ps_wc = (width * bpp) / 8;
	DISPMSG(
		"%s, DSI_WIDTH=%d, DSI_HEIGHT=%d, ps_sel=%d, ps_wc=%d\n",
		__func__, width, tx_params->vertical_active_line,
		ps_sel, ps_wc);

	for (i = DSI_MODULE_BEGIN(module); i <= DSI_MODULE_END(module); i++) {

		mtk_spi_mask_write((u32)(DISPSYS_BDG_TX_DSI0_BASE
			+ REG_DSI_TX_VACT_NL),
			DSI_TX_VACT_NL_FLD_DSI_TX_VACT_NL,
			tx_params->vertical_active_line);

		mtk_spi_mask_write((u32)(DISPSYS_BDG_TX_DSI0_BASE
			+ REG_DSI_TX_PSCON),
			DSI_TX_FLD_CUSTOM_HEADER, 0);
		mtk_spi_mask_write((u32)(DISPSYS_BDG_TX_DSI0_BASE
			+ REG_DSI_TX_PSCON),
			DSI_TX_FLD_DSI_PS_WC, ps_wc);
		mtk_spi_mask_write((u32)(DISPSYS_BDG_TX_DSI0_BASE
			+ REG_DSI_TX_PSCON),
			DSI_TX_FLD_DSI_PS_SEL, ps_sel);

		mtk_spi_mask_write((u32)(DISPSYS_BDG_TX_DSI0_BASE
			+ REG_DSI_TX_SIZE_CON),
			DSI_TX_SIZE_FLD_DSI_WIDTH, width);
		mtk_spi_mask_write((u32)(DISPSYS_BDG_TX_DSI0_BASE
			+ REG_DSI_TX_SIZE_CON),
			DSI_TX_SIZE_FLD_DSI_HEIGHT,
			tx_params->vertical_active_line);

	}

	return 0;
}

int bdg_tx_vdo_timing_set(enum DISP_BDG_ENUM module,
			void *cmdq, LCM_DSI_PARAMS *tx_params)
{
	int i;
	u32 dsi_buf_bpp, data_init_byte;

	DISPFUNCSTART();

	switch (tx_params->data_format.format) {
	case LCM_DSI_FORMAT_RGB565:
		dsi_buf_bpp = 16;
		break;
	case LCM_DSI_FORMAT_RGB666:
		dsi_buf_bpp = 18;
		break;
	case LCM_DSI_FORMAT_RGB666_LOOSELY:
	case LCM_DSI_FORMAT_RGB888:
		dsi_buf_bpp = 24;
		break;
	case LCM_DSI_FORMAT_RGB101010:
		dsi_buf_bpp = 30;
		break;
	default:
		DISPMSG("format not support!!!\n");
		return -1;
	}

	if (tx_params->IsCphy) {
		DISPMSG("C-PHY mode, need check!!!\n");
	} else {
		data_init_byte = bg_tx_data_phy_cycle * tx_params->LANE_NUM;

		switch (tx_params->mode) {
		case DSI_CMD_MODE:
			break;
		case DSI_SYNC_PULSE_VDO_MODE:
			hsa_byte =
				(((tx_params->horizontal_sync_active *
				dsi_buf_bpp) / 8) - 10);
			hbp_byte =
				(((tx_params->horizontal_backporch *
				dsi_buf_bpp) / 8) - 10);
			hfp_byte =
				(((tx_params->horizontal_frontporch *
				dsi_buf_bpp) / 8) - 12);
			break;
		case DSI_SYNC_EVENT_VDO_MODE:
			hsa_byte = 0;	/* don't care */
			hbp_byte =
				(((tx_params->horizontal_backporch +
					tx_params->horizontal_sync_active) *
					dsi_buf_bpp) / 8) - 10;
			hfp_byte =
				(((tx_params->horizontal_frontporch *
				dsi_buf_bpp) / 8) - 12);
			break;
		case DSI_BURST_VDO_MODE:
			hsa_byte = 0;	/* don't care */
			hbp_byte =
				(((tx_params->horizontal_backporch +
				tx_params->horizontal_sync_active) *
				dsi_buf_bpp) / 8) - 10;
			hfp_byte =
				(((tx_params->horizontal_frontporch *
				dsi_buf_bpp) / 8) - 12 - 6);
			break;
		}

		bllp_byte = 16 * tx_params->LANE_NUM;
	}

	if (hsa_byte < 0) {
		DISPMSG("error!hsa = %d < 0!\n", hsa_byte);
		hsa_byte = 0;
		return -1;
	}

	if (hfp_byte > data_init_byte) {
		hfp_byte -= data_init_byte;
	} else {
		hfp_byte = 4;
		DISPMSG("hfp is too short!\n");
		return -2;
	}

	DISPMSG(
		"%s, mode=0x%x, t_vsa=%d, t_vbp=%d, t_vfp=%d, hsa_byte=%d, hbp_byte=%d, hfp_byte=%d, bllp_byte=%d\n",
		__func__, tx_params->mode, tx_params->vertical_sync_active,
		tx_params->vertical_backporch, tx_params->vertical_frontporch,
		hsa_byte, hbp_byte, hfp_byte, bllp_byte);

	for (i = DSI_MODULE_BEGIN(module); i <= DSI_MODULE_END(module); i++) {
		DSI_OUTREG32(cmdq, TX_REG[i]->DSI_TX_VSA_NL,
					tx_params->vertical_sync_active);
		DSI_OUTREG32(cmdq, TX_REG[i]->DSI_TX_VBP_NL,
					(tx_params->vertical_backporch));
		DSI_OUTREG32(cmdq, TX_REG[i]->DSI_TX_VFP_NL,
					(tx_params->vertical_frontporch));

		DSI_OUTREG32(cmdq, TX_REG[i]->DSI_TX_HSA_WC, hsa_byte);
		DSI_OUTREG32(cmdq, TX_REG[i]->DSI_TX_HBP_WC, hbp_byte);
		DSI_OUTREG32(cmdq, TX_REG[i]->DSI_TX_HFP_WC, hfp_byte);
		DSI_OUTREG32(cmdq, TX_REG[i]->DSI_TX_BLLP_WC, bllp_byte);
	}

	return 0;
}

int bdg_tx_buf_rw_set(enum DISP_BDG_ENUM module,
			void *cmdq, LCM_DSI_PARAMS *tx_params)
{
	int i;
	unsigned int width, height, rw_times, tmp;

	DISPFUNCSTART();

	tmp = (mtk_spi_read((unsigned long)
			(&TX_REG[0]->DSI_TX_COM_CON)) & 0x01000000) >> 24;
	width = tx_params->horizontal_active_pixel / 1;
	height = tx_params->vertical_active_line;

	if (dsc_en)
		width = (width + 2) / 3;


	if (tmp == 1 && tx_params->mode == 0) {
		if ((width * 3 % 9) == 0)
			rw_times = (width * 3 / 9) * height;
		else
			rw_times = (width * 3 / 9 + 1) * height;
	} else {
		if ((width * height * 3 % 9) == 0)
			rw_times = width * height * 3 / 9;
		else
			rw_times = width * height * 3 / 9 + 1;
	}

	DISPMSG(
		"%s, mode=0x%x, tmp=%d, width=%d, height=%d, rw_times=%d\n",
		__func__, tx_params->mode, tmp, width, height, rw_times);

	for (i = DSI_MODULE_BEGIN(module); i <= DSI_MODULE_END(module); i++) {
		DSI_OUTREG32(cmdq, TX_REG[i]->DSI_TX_BUF_RW_TIMES, rw_times);

		mtk_spi_mask_write((u32)(DISPSYS_BDG_TX_DSI0_BASE
			+ REG_DSI_TX_BUF_CON0),
			DSI_TX_SIZE_FLD_ANTI_LATENCY_BUF_EN, 1);
	}

	return 0;
}

int bdg_tx_enable_hs_clk(enum DISP_BDG_ENUM module,
				void *cmdq, bool enable)
{
	int i;

	DISPFUNCSTART();

	for (i = DSI_MODULE_BEGIN(module); i <= DSI_MODULE_END(module); i++) {
		if (enable) {
			mtk_spi_mask_write((u32)(DISPSYS_BDG_TX_DSI0_BASE
				+ REG_DSI_TX_PHY_LCCON),
				DSI_TX_SIZE_FLD_LC_HS_TX_EN, 1);
		} else {
			mtk_spi_mask_write((u32)(DISPSYS_BDG_TX_DSI0_BASE
				+ REG_DSI_TX_BUF_CON0),
				DSI_TX_SIZE_FLD_LC_HS_TX_EN, 0);
		}
	}
	return 0;
}


int bdg_tx_set_mode(enum DISP_BDG_ENUM module,
				void *cmdq, unsigned int mode)
{
	int i = 0;

	DISPMSG("%s, mode=%d\n", __func__, mode);
	for (i = DSI_MODULE_BEGIN(module); i <= DSI_MODULE_END(module); i++) {
		mtk_spi_mask_write((u32)(DISPSYS_BDG_TX_DSI0_BASE
			+ REG_DSI_TX_MODE_CON),
			DSI_TX_MODE_CON_FLD_MODE, mode);
	}
	if (mode == CMD_MODE)
		mtk_spi_mask_write((u32)(DISPSYS_BDG_MMSYS_CONFIG_BASE
			+ REG_MIPI_RX_POST_CTRL),
			MIPI_RX_POST_CTRL_FLD_MIPI_RX_MODE_SEL, 0);
	else
		mtk_spi_mask_write((u32)(DISPSYS_BDG_MMSYS_CONFIG_BASE
			+ REG_MIPI_RX_POST_CTRL),
			MIPI_RX_POST_CTRL_FLD_MIPI_RX_MODE_SEL, 1);

	return 0;
}

int bdg_tx_bist_pattern(enum DISP_BDG_ENUM module,
				void *cmdq, bool enable, unsigned int sel,
				unsigned int red, unsigned int green,
				unsigned int blue)
{
	int i;

	DISPFUNCSTART();

	return 0;
}

int bdg_tx_start(enum DISP_BDG_ENUM module, void *cmdq)
{
	int i;

	DISPFUNCSTART();
	for (i = DSI_MODULE_BEGIN(module); i <= DSI_MODULE_END(module); i++) {
		mtk_spi_mask_write((u32)(DISPSYS_BDG_TX_DSI0_BASE
			+ REG_DSI_TX_START),
			DSI_TX_START_FLD_DSI_TX_START, 0);
		mtk_spi_mask_write((u32)(DISPSYS_BDG_TX_DSI0_BASE
			+ REG_DSI_TX_START),
			DSI_TX_START_FLD_DSI_TX_START, 1);
	}

	return 0;
}

int bdg_set_dcs_read_cmd(bool enable, void *cmdq)
{
	DISPFUNCSTART();
	if (enable) {

		DSI_OUTREG32(cmdq,
			DSI2_REG->DSI2_DEVICE_DDI_CLK_MGR_CFG_OS, 55);

		mtk_spi_mask_write((u32)(DISPSYS_BDG_TX_DSI0_BASE
			+ REG_DSI_TX_RACK),
			REG_DSI_TX_RACK_FLD_DSI_TX_RACK_BYPASS, 1);
		mtk_spi_mask_write((u32)(DISPSYS_BDG_MMSYS_CONFIG_BASE
			+ REG_MIPI_RX_POST_CTRL),
			MIPI_RX_POST_CTRL_FLD_MIPI_RX_MODE_SEL, 0);

	} else {
		DSI_OUTREG32(cmdq,
			DSI2_REG->DSI2_DEVICE_DDI_CLK_MGR_CFG_OS, 0);

		mtk_spi_mask_write((u32)(DISPSYS_BDG_TX_DSI0_BASE
			+ REG_DSI_TX_RACK),
			REG_DSI_TX_RACK_FLD_DSI_TX_RACK_BYPASS, 0);
		mtk_spi_mask_write((u32)(DISPSYS_BDG_MMSYS_CONFIG_BASE
			+ REG_MIPI_RX_POST_CTRL),
			MIPI_RX_POST_CTRL_FLD_MIPI_RX_MODE_SEL, 1);

	}
	return 0;
}

int bdg_tx_stop(enum DISP_BDG_ENUM module, void *cmdq)
{
	int i;

	DISPFUNCSTART();
	for (i = DSI_MODULE_BEGIN(module); i <= DSI_MODULE_END(module); i++)
		mtk_spi_mask_write(
			(u32)(DISPSYS_BDG_TX_DSI0_BASE + REG_DSI_TX_START),
			DSI_TX_START_FLD_DSI_TX_START, 0);
	return 0;
}

int bdg_tx_reset(enum DISP_BDG_ENUM module, void *cmdq)
{
	int i;

	DISPFUNCSTART();

	for (i = DSI_MODULE_BEGIN(module); i <= DSI_MODULE_END(module); i++) {
		mtk_spi_mask_write((u32)(DISPSYS_BDG_TX_DSI0_BASE
			+ REG_DSI_TX_COM_CON),
			DSI_TX_COM_CON_FLD_DSI_RESET, 1);
		mtk_spi_mask_write((u32)(DISPSYS_BDG_TX_DSI0_BASE
			+ REG_DSI_TX_COM_CON),
			DSI_TX_COM_CON_FLD_DSI_RESET, 0);
	}

	return 0;
}

int bdg_vm_mode_set(enum DISP_BDG_ENUM module, bool enable,
			unsigned int long_pkt, void *cmdq)
{
	int i = 0;

	for (i = DSI_MODULE_BEGIN(module); i <= DSI_MODULE_END(module); i++) {
		if (enable) {
			DSI_OUTREG32(cmdq,
				TX_REG[i]->DSI_VM_CMD_CON1, 0x10);
			DSI_OUTREG32(cmdq,
				DISPSYS_REG->DDI_POST_CTRL, 0x00001110);
			if (long_pkt > 1)
				DSI_OUTREG32(cmdq,
					TX_REG[i]->DSI_VM_CMD_CON0, 0x37);
			else
				DSI_OUTREG32(cmdq,
					TX_REG[i]->DSI_VM_CMD_CON0, 0x35);
		} else {
			DSI_OUTREG32(cmdq,
				TX_REG[i]->DSI_VM_CMD_CON0, 0);
			DSI_OUTREG32(cmdq,
				DISPSYS_REG->DDI_POST_CTRL, 0x00001100);
		}
	}
	return 0;
}

int bdg_tx_cmd_mode(enum DISP_BDG_ENUM module, void *cmdq)
{
	int i;

	DISPFUNCSTART();
	for (i = DSI_MODULE_BEGIN(module); i <= DSI_MODULE_END(module); i++) {
		DSI_OUTREG32(cmdq,
			TX_REG[i]->DSI_TX_MEM_CONTI, 0x3c);
		DSI_OUTREG32(cmdq,
			TX_REG[i]->DSI_TX_CMDQ, 0x2c3909);
		DSI_OUTREG32(cmdq,
			TX_REG[i]->DSI_TX_CMDQ_CON, 1);
	}
	udelay(10);
	return 0;
}

int bdg_mutex_trigger(enum DISP_BDG_ENUM module, void *cmdq)
{
	DISPFUNCSTART();
	mtk_spi_mask_write((u32)(DISPSYS_BDG_MUTEX_REGS_BASE
		+ REG_BDG_TX_DISP_MUTEX0_EN),
		BDG_TX_DISP_MUTEX0_EN_FLD_MUTEX0_EN, 0);
	mtk_spi_mask_write((u32)(DISPSYS_BDG_MUTEX_REGS_BASE
		+ REG_BDG_TX_DISP_MUTEX0_EN),
		BDG_TX_DISP_MUTEX0_EN_FLD_MUTEX0_EN, 1);

	return 0;
}

int bdg_dsi_dump_reg(enum DISP_BDG_ENUM module)
{
	unsigned int i, k;

	DISPFUNCSTART();

	for (i = DSI_MODULE_BEGIN(module); i <= DSI_MODULE_END(module); i++) {
		unsigned long dsc_base_addr = (unsigned long)DSC_REG;
		unsigned long dsi_base_addr = (unsigned long)TX_REG[i];
		unsigned long mipi_base_addr = (unsigned long)MIPI_TX_REG;

		DISPMSG("0x%08x: 0x%08x\n", 0x0000d00c,
			mtk_spi_read(0x0000d00c));
		DISPMSG("0x%08x: 0x%08x\n", 0x0000d2d0,
			mtk_spi_read(0x0000d2d0));
		DISPMSG("0x%08x: 0x%08x\n", 0x0000d2b0,
			mtk_spi_read(0x0000d2b0));
		DISPMSG("0x%08x: 0x%08x\n", 0x0000d270,
			mtk_spi_read(0x0000d270));
		DISPMSG("0x%08x: 0x%08x\n", 0x0000d250,
			mtk_spi_read(0x0000d250));
		DISPMSG("0x%08x: 0x%08x\n", 0x0000d230,
			mtk_spi_read(0x0000d230));
		DISPMSG("0x%08x: 0x%08x\n", 0x0000d210,
			mtk_spi_read(0x0000d210));
		DISPMSG("0x%08x: 0x%08x\n", 0x0000d280,
			mtk_spi_read(0x0000d280));
		DISPMSG("0x%08x: 0x%08x\n", 0x0000d260,
			mtk_spi_read(0x0000d260));
		DISPMSG("0x%08x: 0x%08x\n", 0x0000d240,
			mtk_spi_read(0x0000d240));
		DISPMSG("0x%08x: 0x%08x\n", 0x0000d220,
			mtk_spi_read(0x0000d220));
		DISPMSG("0x%08x: 0x%08x\n", 0x0000d200,
			mtk_spi_read(0x0000d200));

		DISPMSG("========================== mt6382 DSI%d REGS ==\n", i);
		for (k = 0; k < 0x210; k += 16) {
			DISPMSG("0x%08x: 0x%08x 0x%08x 0x%08x 0x%08x\n", k,
				mtk_spi_read(dsi_base_addr + k),
				mtk_spi_read(dsi_base_addr + k + 0x4),
				mtk_spi_read(dsi_base_addr + k + 0x8),
				mtk_spi_read(dsi_base_addr + k + 0xc));
		}

		DISPMSG("========================== mt6382 DSI%d CMD REGS ==\n",
			i);
		/* only dump first 32 bytes cmd */
		for (k = 0; k < 32; k += 16) {
			DISPMSG("0x%08x: 0x%08x 0x%08x 0x%08x 0x%08x\n", k,
				mtk_spi_read((dsi_base_addr + 0x200 + k)),
				mtk_spi_read((dsi_base_addr + 0x200 + k + 0x4)),
				mtk_spi_read((dsi_base_addr + 0x200 + k + 0x8)),
				mtk_spi_read((
					dsi_base_addr + 0x200 + k + 0xc)));
		}

		DISPMSG("===================== MIPI%d REGS ==\n", i);
		for (k = 0; k < 0x200; k += 16) {
			DISPMSG("0x%08x: 0x%08x 0x%08x 0x%08x 0x%08x\n", k,
				mtk_spi_read(mipi_base_addr + k),
				mtk_spi_read(mipi_base_addr + k + 0x4),
				mtk_spi_read(mipi_base_addr + k + 0x8),
				mtk_spi_read(mipi_base_addr + k + 0xc));
		}

		if (dsc_en) {
			DISPMSG("================ mt6382 DSC%d REGS ==\n", i);
			for (k = 0; k < sizeof(struct BDG_DISP_DSC_REGS);
				k += 16) {
				DISPMSG("0x%08x: 0x%08x 0x%08x 0x%08x 0x%08x\n",
					k,
					mtk_spi_read(dsc_base_addr + k),
					mtk_spi_read(dsc_base_addr + k + 0x4),
					mtk_spi_read(dsc_base_addr + k + 0x8),
					mtk_spi_read(dsc_base_addr + k + 0xc));
			}
		}
	}

	return 0;
}


int bdg_tx_wait_for_idle(enum DISP_BDG_ENUM module)
{
	int i;
	unsigned int timeout = 5000; /* unit: usec */
	unsigned int status;

	for (i = DSI_MODULE_BEGIN(module);
		i <= DSI_MODULE_END(module); i++) {
		while (timeout) {
			udelay(1);
			status = mtk_spi_read(
				(unsigned long)(&TX_REG[i]->DSI_TX_INTSTA));

			if (!(status & 0x80000000))
				break;
			timeout--;
		}
	}
	if (timeout == 0) {
		DISPMSG("%s, wait timeout!\n", __func__);
		return -1;
	}

	return 0;
}

int ap_tx_phy_config(enum DISP_BDG_ENUM module,
			void *cmdq, LCM_DSI_PARAMS *tx_params)
{
	return 0;
}

int bdg_dsi_line_timing_dphy_setting(enum DISP_BDG_ENUM module,
			void *cmdq, LCM_DSI_PARAMS *tx_params)
{
	unsigned int width, height, lanes, ps_wc, new_hfp_byte;
	unsigned int bg_tx_total_word_cnt = 0;
	unsigned int bg_tx_line_time = 0, disp_pipe_line_time = 0;
	unsigned int rxtx_ratio = 0;

	DISPFUNCSTART();
	width = tx_params->horizontal_active_pixel;
	height = tx_params->vertical_active_line;
	lanes = tx_params->LANE_NUM;

	/* Step 1. Show Bridge DSI TX Setting. */
	/* get bdg-tx hsa_byte, hbp_byte, new_hfp_byte and bllp_byte */
	if (dsc_en) {
		/* for 8bpp, 1/3 compression */
		ps_wc = width * 24 / 8 / 3;
		rxtx_ratio = RXTX_RATIO;
	} else {
		/* for 8bpp, 1/3 compression */
		ps_wc = width * 24 / 8;
		rxtx_ratio = 100;
	}
	new_hfp_byte = hfp_byte;

	DISPMSG("%s, dsc_en=%d, hsa_byte=%d, hbp_byte=%d, new_hfp_byte=%d, bllp_byte=%d, ps_wc=%d\n",
		__func__, dsc_en, hsa_byte, hbp_byte,
		new_hfp_byte, bllp_byte, ps_wc);

	/* get lpx, hs_prep, hs_zero,... refer to bdg_tx_phy_config()*/

	/* Step 2. Bridge DSI TX Cycle Count. */
	/* get bg_tx_line_cycle, bg_tx_total_word_cnt */
	/* D-PHY, DSI TX (Bridge IC) back to LP mode during V-Active */
	switch (tx_params->mode) {
	case CMD_MODE:
		bg_tx_total_word_cnt = 0;
		break;
	case SYNC_PULSE_VDO_MODE:
		bg_tx_total_word_cnt = 4 +	/* hss packet */
			(4 + hsa_byte + 2) +	/* hsa packet */
			4 +			/* hse packet */
			(4 + hbp_byte + 2) +	/* hbp packet */
			(4 + ps_wc + 2) +	/* rgb packet */
			(4 + new_hfp_byte + 2) +/* hfp packet */
			bg_tx_data_phy_cycle * lanes;
		break;
	case SYNC_EVENT_VDO_MODE:
		bg_tx_total_word_cnt = 4 +	/* hss packet */
			(4 + hbp_byte + 2) +	/* hbp packet */
			(4 + ps_wc + 2) +	/* rgb packet */
			(4 + new_hfp_byte + 2) +/* hfp packet */
			bg_tx_data_phy_cycle * lanes;
		break;
	case BURST_VDO_MODE:
		bg_tx_total_word_cnt = 4 +	/* hss packet */
			(4 + hbp_byte + 2) +	/* hbp packet */
			(4 + ps_wc + 2) +	/* rgb packet */
			(4 + bllp_byte + 2) +	/* bllp packet */
			(4 + new_hfp_byte + 2) +/* hfp packet */
			bg_tx_data_phy_cycle * lanes;
		break;
	}

	bg_tx_line_cycle = (bg_tx_total_word_cnt + (lanes - 1)) / lanes;
	bg_tx_line_time = bg_tx_line_cycle * 8000 / tx_data_rate;

	disp_pipe_line_time = width * 1000 / MM_CLK;

	DISPMSG("bg_tx_total_word_cnt=%d, bg_tx_line_cycle=%d\n",
		bg_tx_total_word_cnt, bg_tx_line_cycle);
	DISPMSG("disp_pipe_line_time=%d, bg_tx_line_time=%d\n",
		disp_pipe_line_time, bg_tx_line_time);

	if ((tx_params->mode != CMD_MODE) &&
		(disp_pipe_line_time > bg_tx_line_time)) {
		DISPMSG("error!! disp_pipe_line_time(%d) > bg_tx_line_time(%d)\n",
			disp_pipe_line_time, bg_tx_line_time);

		return -1;
	}
	/* Step 3. Decide AP DSI TX Data Rate and PHY Timing */
	/* get ap_tx_data_rate and set back to ap */
	/* get lpx, hs_prep, hs_zero,... refer to DSI_DPHY_TIMCONFIG()*/
	ap_tx_data_rate = tx_data_rate * rxtx_ratio / 100;

	/* Step 4. Decide AP DSI TX Blanking */
	/* get ap-tx hsa_byte, hbp_byte, new_hfp_byte and bllp_byte */
	/* refer to DSI_DPHY_Calc_VDO_Timing() */

	/* Step 5. fine-tune data rate */
#ifdef aaa
	if (dsc_en) {
		ap_tx_hsa_wc = 4;
		ap_tx_hbp_wc = 4;
	} else {
		ap_tx_hsa_wc = hsa_byte;
		ap_tx_hbp_wc = hbp_byte;
		ap_tx_data_phy_cycle = bg_tx_data_phy_cycle;
	}
	ap_tx_bllp_wc  = bllp_byte;
	ap_tx_total_word_cnt = bg_tx_line_cycle *
		lanes * rxtx_ratio / 100;

	switch (tx_params->mode) {
	case CMD_MODE:
		ap_tx_total_word_cnt_no_hfp_wc = 0;
		break;
	case SYNC_PULSE_VDO_MODE:
		ap_tx_total_word_cnt_no_hfp_wc = 4 +	/* hss packet */
			(4 + ap_tx_hsa_wc + 2) +	/* hsa packet */
			4 +				/* hse packet */
			(4 + ap_tx_hbp_wc + 2) +	/* hbp packet */
			(4 + ps_wc + 2) +		/* rgb packet */
			(4 + 2) +			/* hfp packet */
			ap_tx_data_phy_cycle * lanes;
		break;
	case SYNC_EVENT_VDO_MODE:
		ap_tx_total_word_cnt_no_hfp_wc = 4 +	/* hss packet */
			(4 + ap_tx_hbp_wc + 2) +	/* hbp packet */
			(4 + ps_wc + 2) +		/* rgb packet */
			(4 + 2) +			/* hfp packet */
			ap_tx_data_phy_cycle * lanes;
		break;
	case BURST_VDO_MODE:
		ap_tx_total_word_cnt_no_hfp_wc = 4 +	/* hss packet */
			(4 + ap_tx_hbp_wc + 2) +	/* hbp packet */
			(4 + ps_wc + 2) +		/* rgb packet */
			(4 + ap_tx_bllp_wc + 2) +	/* bllp packet */
			(4 + 2) +			/* hfp packet */
			ap_tx_data_phy_cycle * lanes;
		break;
	}

	ap_tx_hfp_wc = ap_tx_total_word_cnt - ap_tx_total_word_cnt_no_hfp_wc;

	DISPMSG("%s, ap_tx_hsa_wc=%d, ap_tx_hbp_wc=%d, ap_tx_bllp_wc=%d, ap_tx_data_phy_cycle=%d\n",
		__func__, ap_tx_hsa_wc, ap_tx_hbp_wc,
		ap_tx_bllp_wc, ap_tx_data_phy_cycle);
	DISPMSG("%s, ap_tx_hfp_wc=%d, ap_tx_total_word_cnt=%d, ap_tx_total_word_cnt_no_hfp_wc=%d\n",
		__func__, ap_tx_hfp_wc, ap_tx_total_word_cnt,
		ap_tx_total_word_cnt_no_hfp_wc);

#endif
	return 0;
}

unsigned int get_ap_data_rate(void)
{
	DISPMSG("%s, ap_tx_data_rate=%d\n", __func__, ap_tx_data_rate);

	return ap_tx_data_rate;
}

unsigned int get_bdg_data_rate(void)
{
	DISPMSG("%s, tx_data_rate=%d\n", __func__, tx_data_rate);

	return tx_data_rate;
}

int set_bdg_data_rate(unsigned int data_rate)
{

	if (data_rate > 2500 || data_rate < 100) {
		DISPMSG("error, please check data rate=%d MHz\n", data_rate);
		return -1;
	}
	tx_data_rate = data_rate;

	return 0;
}

unsigned int get_bdg_line_cycle(void)
{
	DISPMSG("%s, bg_tx_line_cycle=%d\n", __func__, bg_tx_line_cycle);

	return bg_tx_line_cycle;
}

unsigned int get_dsc_state(void)
{
	DISPMSG("%s, dsc_en=%d\n", __func__, dsc_en);

	return dsc_en;
}

unsigned int get_mt6382_init(void)
{
	DISPMSG("%s, mt6382_init=%d\n", __func__, mt6382_init);

	return mt6382_init;
}

unsigned int get_bdg_tx_mode(void)
{
	DISPMSG("%s, bdg_tx_mode=%d\n", __func__, bdg_tx_mode);

	return bdg_tx_mode;
}

void dsi_set_cmdq_v2(enum DISP_BDG_ENUM module, void *cmdq,
			unsigned int cmd, unsigned char count,
		     unsigned char *para_list, unsigned char force_update)
{
	UINT32 i = 0;
	unsigned long goto_addr, mask_para, set_para;
	struct DSI_TX_T0_INS t0;
	struct DSI_TX_T2_INS t2;
	struct DSI_TX_CMDQ *tx_cmdq;

	memset(&t0, 0, sizeof(struct DSI_TX_T0_INS));
	memset(&t2, 0, sizeof(struct DSI_TX_T2_INS));

	tx_cmdq = TX_CMDQ_REG[0]->data;

	bdg_tx_wait_for_idle(module);

	if (count > 1) {
		t2.CONFG = 2;
		if (cmd < 0xB0)
			t2.Data_ID = TX_DCS_LONG_PACKET_ID;
		else
			t2.Data_ID = TX_GERNERIC_LONG_PACKET_ID;
		t2.WC16 = count + 1;

		DSI_OUTREG32(cmdq, tx_cmdq[0], AS_UINT32(&t2));

		goto_addr = (unsigned long)(&tx_cmdq[1].byte0);
		mask_para = (0xFFu << ((goto_addr & 0x3u) * 8));
		set_para = cmd;
		goto_addr = (goto_addr & (~0x3UL));

		DSI_MASKREG32(cmdq, goto_addr, mask_para, set_para);

		for (i = 0; i < count; i++) {
			goto_addr = (unsigned long)(&tx_cmdq[1].byte1) + i;
			mask_para = (0xFFu << ((goto_addr & 0x3u) * 8));
			set_para = (unsigned long)para_list[i];
			goto_addr = (goto_addr & (~0x3UL));

			DSI_MASKREG32(cmdq, goto_addr, mask_para, set_para);
		}

		DSI_OUTREG32(cmdq, TX_REG[0]->DSI_TX_CMDQ_CON,
				(1 << 15) | (2 + (count) / 4));
	} else {
		t0.CONFG = 0;
		t0.Data0 = cmd;
		if (count) {
			if (cmd < 0xB0)
				t0.Data_ID = TX_DCS_SHORT_PACKET_ID_1;
			else
				t0.Data_ID = TX_GERNERIC_SHORT_PACKET_ID_2;
			t0.Data1 = para_list[0];
		} else {
			if (cmd < 0xB0)
				t0.Data_ID = TX_DCS_SHORT_PACKET_ID_0;
			else
				t0.Data_ID = TX_GERNERIC_SHORT_PACKET_ID_1;
			t0.Data1 = 0;
		}

		DSI_OUTREG32(cmdq, tx_cmdq[0], AS_UINT32(&t0));
		DSI_OUTREG32(cmdq, TX_REG[0]->DSI_TX_CMDQ_CON, 1);
	}

	if (force_update) {
		bdg_tx_start(module, cmdq);
		bdg_tx_wait_for_idle(module);
	}
}

void push_table(struct lcm_setting_table *table, unsigned int count,
			unsigned char force_update)
{
	unsigned int i, cmd;

	for (i = 0; i < count; i++) {
		cmd = table[i].cmd;

		switch (cmd) {
		case REGFLAG_DELAY:
			if (table[i].count <= 10)
				mdelay(table[i].count);
			else
				mdelay(table[i].count);
			break;
		case REGFLAG_UDELAY:
			udelay(table[i].count);
			break;
		case REGFLAG_END_OF_TABLE:
			break;
		default:
			dsi_set_cmdq_v2(DISP_BDG_DSI0, NULL, cmd,
				table[i].count, table[i].para_list,
				force_update);
			if (table[i].count > 1)
				UDELAY(100);
			break;
		}
	}
}


int lcm_init(enum DISP_BDG_ENUM module)
{
	DISPFUNCSTART();
	DISPFUNCEND();

	return 0;
}


int bdg_tx_init(enum DISP_BDG_ENUM module,
		   disp_ddp_path_config *config, void *cmdq)
{
	int ret = 0;
	LCM_DSI_PARAMS *tx_params;

	DISPFUNCSTART();

	tx_params = &(config->dsi_config);
	tx_params->data_rate = tx_data_rate > 0 ?
				tx_data_rate : tx_params->data_rate;
	dsc_en = tx_params->bdg_dsc_enable;
	bdg_tx_mode = tx_params->mode;

	DISPMSG("%s, tx_data_rate=%d, bdg_ssc_disable=%d, ssc_disable=%d, dsc_enable=%d, bdg_tx_mode=%d\n",
		__func__, tx_data_rate, tx_params->ssc_disable,
		tx_params->ssc_disable, dsc_en, bdg_tx_mode);

	ret |= bdg_mipi_tx_dphy_clk_setting(module, cmdq, tx_params);
	udelay(20);

	ret |= bdg_tx_phy_config(module, cmdq, tx_params);
	ret |= bdg_tx_txrx_ctrl(module, cmdq, tx_params);
	ret |= bdg_tx_ps_ctrl(module, cmdq, tx_params);
	ret |= bdg_tx_vdo_timing_set(module, cmdq, tx_params);
	ret |= bdg_tx_buf_rw_set(module, cmdq, tx_params);
	ret |= bdg_tx_enable_hs_clk(module, cmdq, TRUE);
	ret |= bdg_tx_set_mode(module, cmdq, CMD_MODE);
	ret |= bdg_dsi_line_timing_dphy_setting(module, cmdq, tx_params);

	/* panel init*/

	DISPFUNCEND();
	return ret;
}

int bdg_tx_deinit(enum DISP_BDG_ENUM module, void *cmdq)
{
	int i;

	DISPFUNCSTART();

	bdg_tx_enable_hs_clk(module, cmdq, FALSE);
	bdg_tx_set_mode(module, cmdq, CMD_MODE);
	bdg_tx_reset(module, cmdq);

	for (i = DSI_MODULE_BEGIN(module); i <= DSI_MODULE_END(module); i++) {
		mtk_spi_mask_write((u32)(DISPSYS_BDG_TX_DSI0_BASE
			+ REG_DSI_TX_TXRX_CON),
			DSI_TX_TXRX_CON_FLD_LANE_NUM, 0);
	}
	DISPFUNCEND();
	return 0;
}


void calculate_datarate_cfgs_rx(unsigned int data_rate)
{
	unsigned int timebase = 1000, itminrx = 4;
	unsigned int des_div_en_dly_deass_th = 1;
	unsigned int hs_clk_freq = data_rate * 1000 / 2;

	DISPFUNCSTART();
	ddl_cntr_ref_reg = 5 * hs_clk_freq / 1000 / 2 / 16;

	if (data_rate >= 4500) {
		sel_fast = 1;
		max_phase = 63;
		dll_fbk = 7;
		coarse_bank = 0;
	} else if (data_rate >= 4000) {
		sel_fast = 1;
		max_phase = 71;
		dll_fbk = 8;
		coarse_bank = 1;
	} else if (data_rate >= 3600) {
		sel_fast = 1;
		max_phase = 79;
		dll_fbk = 9;
		coarse_bank = 1;
	} else if (data_rate >= 3230) {
		sel_fast = 1;
		max_phase = 87;
		dll_fbk = 10;
		coarse_bank = 1;
	} else if (data_rate >= 3000) {
		sel_fast = 0;
		max_phase = 71;
		dll_fbk = 8;
		coarse_bank = 0;
	} else if (data_rate >= 2700) {
		sel_fast = 0;
		max_phase = 79;
		dll_fbk = 9;
		coarse_bank = 1;
	} else if (data_rate >= 2455) {
		sel_fast = 0;
		max_phase = 87;
		dll_fbk = 10;
		coarse_bank = 1;
	} else if (data_rate >= 2250) {
		sel_fast = 0;
		max_phase = 95;
		dll_fbk = 11;
		coarse_bank = 1;
	} else if (data_rate >= 2077) {
		sel_fast = 0;
		max_phase = 103;
		dll_fbk = 12;
		coarse_bank = 1;
	} else if (data_rate >= 1929) {
		sel_fast = 0;
		max_phase = 111;
		dll_fbk = 13;
		coarse_bank = 2;
	} else if (data_rate >= 1800) {
		sel_fast = 0;
		max_phase = 119;
		dll_fbk = 14;
		coarse_bank = 2;
	} else if (data_rate >= 1688) {
		sel_fast = 0;
		max_phase = 127;
		dll_fbk = 15;
		coarse_bank = 2;
	} else if (data_rate >= 1588) {
		sel_fast = 0;
		max_phase = 135;
		dll_fbk = 16;
		coarse_bank = 2;
	} else if (data_rate >= 1500) {
		sel_fast = 0;
		max_phase = 143;
		dll_fbk = 17;
		coarse_bank = 3;
	} else {
		sel_fast = 0;
		max_phase = 0;
		dll_fbk = 0;
		coarse_bank = 0;
	}

	if (hs_clk_freq * 2 < 160000)
		hsrx_clk_div = 1;
	else if (hs_clk_freq * 2 < 320000)
		hsrx_clk_div = 2;
	else if (hs_clk_freq * 2 < 640000)
		hsrx_clk_div = 3;
	else if (hs_clk_freq * 2 < 1280000)
		hsrx_clk_div = 4;
	else if (hs_clk_freq * 2 < 2560000)
		hsrx_clk_div = 5;
	else
		hsrx_clk_div = 6;

	hs_thssettle = 115 + 1000 * 6 / data_rate;
	hs_thssettle = (hs_thssettle - T_DCO - (itminrx + 3) *
		T_DCO - 2 * T_DCO) / T_DCO - 1;
	fjump_deskew_reg = max_phase / 10 / 4;
	eye_open_deskew_reg = max_phase * 4 / 10 / 2;

	DISPMSG("data_rate=%d, ddl_cntr_ref_reg=%d, hs_thssettle=%d, fjump_deskew_reg=%d\n",
		data_rate, ddl_cntr_ref_reg, hs_thssettle, fjump_deskew_reg);

	if (hs_clk_freq * 2 >= 900000)
		cdr_coarse_trgt_reg = timebase * hs_clk_freq * 2 * 2 /
		32 / 1000000 - 1;
	else
		cdr_coarse_trgt_reg = timebase * 900000 * 2 / 32 / 1000000 - 1;

	en_dly_deass_thresh_reg = 1000000 * 7 * 6 / hs_clk_freq / 2 /
		T_DCO + des_div_en_dly_deass_th;

	post_rcvd_rst_val = 2 * T_DCO * hs_clk_freq * 2 / 7 / 1000000 - 1;
	post_rcvd_rst_reg = (post_rcvd_rst_val > 0) ? post_rcvd_rst_val : 1;

	post_det_dly_thresh_val = ((189 * 1000000 / hs_clk_freq / 2) -
		(9 * 7 * 1000000 / hs_clk_freq / 2)) / T_DCO - 7;
	post_det_dly_thresh_reg = (post_det_dly_thresh_val > 0) ?
		post_det_dly_thresh_val : 1;

	DISPMSG("cdr_coarse_trgt_reg=%d, post_rcvd_rst_val=%d, fjump_deskew_reg=%d\n",
		cdr_coarse_trgt_reg, post_rcvd_rst_val, post_rcvd_rst_reg);
	DISPMSG("en_dly_deass_thresh_reg=%d, post_det_dly_thresh_val=%d, post_det_dly_thresh_reg=%d\n",
		en_dly_deass_thresh_reg, post_det_dly_thresh_val,
		post_det_dly_thresh_reg);
}

void mipi_rx_enable(void *cmdq)
{
	DSI_OUTREG32(cmdq, OCLA_REG->OCLA_LANEC_CON, 0);
	DSI_OUTREG32(cmdq, OCLA_REG->OCLA_LANE0_CON, 0);
	DSI_OUTREG32(cmdq, OCLA_REG->OCLA_LANE1_CON, 0);
	DSI_OUTREG32(cmdq, OCLA_REG->OCLA_LANE2_CON, 0);
	DSI_OUTREG32(cmdq, OCLA_REG->OCLA_LANE3_CON, 0);
}

void mipi_rx_unset_forcerxmode(void *cmdq)
{
	mtk_spi_mask_write((u32)(DISPSYS_BDG_OCLA_BASE
		+ REG_OCLA_LANEC_CON),
		OCLA_LANE_CON_FLD_FORCE_RX_MODE, 0);
	mtk_spi_mask_write((u32)(DISPSYS_BDG_OCLA_BASE
		+ REG_OCLA_LANE0_CON),
		OCLA_LANE_CON_FLD_FORCE_RX_MODE, 0);
	mtk_spi_mask_write((u32)(DISPSYS_BDG_OCLA_BASE
		+ REG_OCLA_LANE1_CON),
		OCLA_LANE_CON_FLD_FORCE_RX_MODE, 0);
	mtk_spi_mask_write((u32)(DISPSYS_BDG_OCLA_BASE
		+ REG_OCLA_LANE2_CON),
		OCLA_LANE_CON_FLD_FORCE_RX_MODE, 0);
	mtk_spi_mask_write((u32)(DISPSYS_BDG_OCLA_BASE
		+ REG_OCLA_LANE3_CON),
		OCLA_LANE_CON_FLD_FORCE_RX_MODE, 0);
}

void mipi_rx_set_forcerxmode(void *cmdq)
{
	mtk_spi_mask_write((u32)(DISPSYS_BDG_OCLA_BASE
		+ REG_OCLA_LANEC_CON),
		OCLA_LANE_CON_FLD_FORCE_RX_MODE, 1);
	mtk_spi_mask_write((u32)(DISPSYS_BDG_OCLA_BASE
		+ REG_OCLA_LANE0_CON),
		OCLA_LANE_CON_FLD_FORCE_RX_MODE, 1);
	mtk_spi_mask_write((u32)(DISPSYS_BDG_OCLA_BASE
		+ REG_OCLA_LANE1_CON),
		OCLA_LANE_CON_FLD_FORCE_RX_MODE, 1);
	mtk_spi_mask_write((u32)(DISPSYS_BDG_OCLA_BASE
		+ REG_OCLA_LANE2_CON),
		OCLA_LANE_CON_FLD_FORCE_RX_MODE, 1);
	mtk_spi_mask_write((u32)(DISPSYS_BDG_OCLA_BASE
		+ REG_OCLA_LANE3_CON),
		OCLA_LANE_CON_FLD_FORCE_RX_MODE, 1);
}

void startup_seq_common(void *cmdq)
{
	DISPFUNCSTART();

	DSI_OUTREG32(cmdq, OCLA_REG->OCLA_LANE_SWAP, 0x30213102);
	mipi_rx_enable(cmdq);
	mipi_rx_set_forcerxmode(cmdq);

	mtk_spi_mask_field_write(MIPI_RX_PHY_BASE
		+ CORE_DIG_ANACTRL_RW_COMMON_ANACTRL_0 * 4,
		CORE_DIG_ANACTRL_RW_COMMON_ANACTRL_0_CB_LP_DCO_EN_DLY_MASK,
		63);

	mtk_spi_mask_field_write(MIPI_RX_PHY_BASE
		+ PPI_STARTUP_RW_COMMON_STARTUP_1_1 * 4,
		PPI_STARTUP_RW_COMMON_STARTUP_1_1_PHY_READY_DLY_MASK, 563);

	mtk_spi_mask_field_write(MIPI_RX_PHY_BASE
		+ PPI_STARTUP_RW_COMMON_DPHY_6 * 4,
		PPI_STARTUP_RW_COMMON_DPHY_6_LP_DCO_CAL_addr_MASK, 39);

	mtk_spi_mask_field_write(MIPI_RX_PHY_BASE
		+ PPI_CALIBCTRL_RW_COMMON_BG_0 * 4,
		PPI_CALIBCTRL_RW_COMMON_BG_0_BG_MAX_COUNTER_MASK, 500);

	/* cfg_clk = 26 MHz */
	mtk_spi_mask_field_write(MIPI_RX_PHY_BASE
		+ PPI_RW_TERMCAL_CFG_0 * 4,
		PPI_RW_TERMCAL_CFG_0_TERMCAL_TIMER_MASK, 25);
	mtk_spi_mask_field_write(MIPI_RX_PHY_BASE
		+ PPI_RW_OFFSETCAL_CFG_0 * 4,
		PPI_RW_OFFSETCAL_CFG_0_OFFSETCAL_WAIT_THRESH_MASK, 5);
	mtk_spi_mask_field_write(MIPI_RX_PHY_BASE
		+ PPI_RW_LPDCOCAL_TIMEBASE * 4,
		PPI_RW_LPDCOCAL_TIMEBASE_LPCDCOCAL_TIMEBASE_MASK, 103);

	mtk_spi_mask_field_write(MIPI_RX_PHY_BASE
		+ PPI_RW_LPDCOCAL_NREF * 4,
		PPI_RW_LPDCOCAL_NREF_LPDCOCAL_NREF_MASK, 800);

	mtk_spi_mask_field_write(MIPI_RX_PHY_BASE
		+ PPI_RW_LPDCOCAL_NREF_RANGE * 4,
		PPI_RW_LPDCOCAL_NREF_RANGE_LPDCOCAL_NREF_RANGE_MASK, 15);

	mtk_spi_mask_field_write(MIPI_RX_PHY_BASE
		+ PPI_RW_LPDCOCAL_TWAIT_CONFIG * 4,
		PPI_RW_LPDCOCAL_TWAIT_CONFIG_LPDCOCAL_TWAIT_PON_MASK, 127);

	/* cfg_clk = 26 MHz */
	mtk_spi_mask_field_write(MIPI_RX_PHY_BASE
		+ PPI_RW_LPDCOCAL_TWAIT_CONFIG * 4,
		PPI_RW_LPDCOCAL_TWAIT_CONFIG_LPDCOCAL_TWAIT_COARSE_MASK, 32);

	mtk_spi_mask_field_write(MIPI_RX_PHY_BASE
		+ PPI_RW_LPDCOCAL_VT_CONFIG * 4,
		PPI_RW_LPDCOCAL_VT_CONFIG_LPDCOCAL_TWAIT_FINE_MASK, 32);

	mtk_spi_mask_field_write(MIPI_RX_PHY_BASE
		+ PPI_RW_LPDCOCAL_VT_CONFIG * 4,
		PPI_RW_LPDCOCAL_VT_CONFIG_LPCDCOCAL_VT_NREF_RANGE_MASK, 15);

	mtk_spi_mask_field_write(MIPI_RX_PHY_BASE
		+ PPI_RW_LPDCOCAL_VT_CONFIG * 4,
		PPI_RW_LPDCOCAL_VT_CONFIG_LPCDCOCAL_USE_IDEAL_NREF_MASK, 0);

	mtk_spi_mask_field_write(MIPI_RX_PHY_BASE
		+ PPI_RW_LPDCOCAL_VT_CONFIG * 4,
		PPI_RW_LPDCOCAL_VT_CONFIG_LPCDCOCAL_VT_TRACKING_EN_MASK, 0);

	mtk_spi_mask_field_write(MIPI_RX_PHY_BASE
		+ PPI_RW_LPDCOCAL_COARSE_CFG * 4,
		PPI_RW_LPDCOCAL_COARSE_CFG_NCOARSE_START_MASK, 1);

	mtk_spi_mask_field_write(MIPI_RX_PHY_BASE
		+ CORE_DIG_IOCTRL_RW_AFE_CB_CTRL_2_6 * 4,
		CORE_DIG_IOCTRL_RW_AFE_CB_CTRL_2_6_OA_CB_HSTXLB_DCO_PON_OVR_EN_MASK,
		1);

	mtk_spi_mask_field_write(MIPI_RX_PHY_BASE
		+ PPI_RW_COMMON_CFG * 4,
		PPI_RW_COMMON_CFG_CFG_CLK_DIV_FACTOR_MASK, 3);
	DISPFUNCEND();
}

int check_stopstate(void *cmdq)
{
	unsigned int timeout = 50000;
	unsigned int stop_state = 0, count = 0;

	DISPFUNCSTART();
	while (timeout) {
		stop_state = mtk_spi_read(
			(unsigned long)(&OCLA_REG->OCLA_LANE0_STOPSTATE));

		DISPMSG("stop_state=0x%x, timeout=%d\n", stop_state, timeout);

		if ((stop_state & 0x1) == 0x1)
			count++;

		if (count > 5)
			break;

		udelay(1);
		timeout--;
	}

	if (timeout == 0) {
		DISPMSG("%s, wait timeout!\n", __func__);
		return -1;
	}
	mipi_rx_unset_forcerxmode(cmdq);
	mt6382_init = 1;
	DISPFUNCEND();

	return 0;
}
#if 0
int polling_status(void)
{
	unsigned int timeout = 5000;
	unsigned int status = 0;

	DISPFUNCSTART();
	while (timeout) {
		status = mtk_spi_read((
			unsigned long)(&TX_REG[0]->DSI_TX_STATE_DBG7));
		DISPMSG("status=0x%x, timeout=%d\n", status, timeout);

		if ((status & 0x800) == 0x800)
			break;

		udelay(1);
		timeout--;
	}

	if (timeout == 0) {
		DISPMSG("%s, wait timeout!\n", __func__);
		return -1;
	}
	DISPFUNCEND();
	return 0;
}

void disp_init_bdg_gce_obj(void)
{
	struct device *dev;
	int index;

	dev = disp_get_device();
	if (dev == NULL) {
		DISPERR("get device fail\n");
		return;
	}

	index = of_property_match_string(dev->of_node,
			"gce-client-names", "BDG_CLIENT_CFG0");

	if (index < 0) {
		DISPERR("%s map client fail\n", __func__);
		return;
	}

	disp_bdg_gce_client = cmdq_mbox_create(dev, index);
	if (disp_bdg_gce_client == NULL) {
		DISPERR("%s create client fail\n", __func__);
		return;
	}

	disp_bdg_gce_base = cmdq_register_device(dev);
	if (disp_bdg_gce_base == NULL)
		DISPERR("%s register client fail\n", __func__);


	bdg_dsi0_eof_gce_event = cmdq_dev_get_event(dev, "bdg_dsi0_eof");
	if (bdg_dsi0_eof_gce_event == 0)
		DISPERR("%s register EOF GCE event %d\n", __func__,
				bdg_dsi0_eof_gce_event);

	bdg_dsi0_sof_gce_event = cmdq_dev_get_event(dev, "bdg_dsi0_sof");
	if (bdg_dsi0_sof_gce_event)
		DISPERR("%s register SOF GCE event %d\n", __func__,
				bdg_dsi0_sof_gce_event);

	bdg_dsi0_te_gce_event = cmdq_dev_get_event(dev, "bdg_dsi0_te");
	if (bdg_dsi0_te_gce_event)
		DISPERR("%s register TE GCE event %d\n", __func__,
				bdg_dsi0_te_gce_event);

	bdg_dsi0_done_gce_event = cmdq_dev_get_event(dev, "bdg_dsi0_done");
	if (bdg_dsi0_done_gce_event)
		DISPERR("%s register DONE GCE event %d\n", __func__,
				bdg_dsi0_done_gce_event);

	bdg_dsi0_target_gce_event =
		cmdq_dev_get_event(dev, "bdg_dsi0_target_line");
	if (bdg_dsi0_target_gce_event)
		DISPERR("%s register TARGET GCE event %d\n", __func__,
				bdg_dsi0_target_gce_event);

	bdg_rdma0_sof_gce_event = cmdq_dev_get_event(dev, "bdg_rdma0_sof");
	if (bdg_rdma0_sof_gce_event)
		DISPERR("%s register TARGET GCE event %d\n", __func__,
				bdg_rdma0_sof_gce_event);

	bdg_rdma0_eof_gce_event = cmdq_dev_get_event(dev, "bdg_rdma0_eof");
	if (bdg_rdma0_eof_gce_event)
		DISPERR("%s register TARGET GCE event %d\n", __func__,
				bdg_rdma0_eof_gce_event);
}

static void bdg_cmdq_cb(struct cmdq_cb_data data)
{
	struct cmdq_pkt *cmdq_handle = data.data;

	cmdq_pkt_destroy(cmdq_handle);
}

void bdg_dsi_vfp_gce(unsigned int vfp)
{
	struct cmdq_pkt *cmdq_handle;
	int ret;

	if (!disp_bdg_gce_client) {
		DISPERR("%s not valid gce client\n", __func__);
		return;
	}

	cmdq_handle = cmdq_pkt_create(disp_bdg_gce_client);

	cmdq_pkt_clear_event(cmdq_handle, bdg_dsi0_target_gce_event);
	cmdq_pkt_clear_event(cmdq_handle, bdg_rdma0_sof_gce_event);

	/* set DSI TARGET_LINE unmask */
	/* TODO: remove fixed TARGET_NL count */
	/* TODO: not to usd fixed address */
	cmdq_pkt_write(cmdq_handle, disp_bdg_gce_base,
		0x00021300, 0x107d0, ~0);
	cmdq_pkt_wait_no_clear(cmdq_handle, bdg_dsi0_target_gce_event);
	cmdq_pkt_wait_no_clear(cmdq_handle, bdg_rdma0_sof_gce_event);
	/* set BDG VFP to vfp - 1 */
	cmdq_pkt_write(cmdq_handle, disp_bdg_gce_base, 0x00021028, vfp - 1, ~0);

	/* set DSI TARGET_LINE mask */
	cmdq_pkt_write(cmdq_handle, disp_bdg_gce_base, 0x00021300, 0x7d0, ~0);

	/* TODO: use cmdq async flush */
	if (0)
		cmdq_pkt_flush_threaded(cmdq_handle, bdg_cmdq_cb, cmdq_handle);
	else {
		ret = cmdq_pkt_flush(cmdq_handle);
		if (ret)
			DISPERR("%s cmdq_timeout\n", __func__);
	}

	cmdq_pkt_destroy(cmdq_handle);
}
#endif

#define _n36672c_
int bdg_dsc_init(enum DISP_BDG_ENUM module,
			void *cmdq, LCM_DSI_PARAMS *tx_params)
{
	unsigned int width, height;

	DISPFUNCSTART();
	width = tx_params->horizontal_active_pixel / 1;
	height = tx_params->vertical_active_line;

#ifdef _n36672c_
/*
*Resolution = 1080x2400
*Slice width = 540
*Slice height = 8
*Format = RGB888
*DSC version = v1.1
*Compression rate = 1/3
*/
	DSI_OUTREG32(cmdq, DSC_REG->DISP_DSC_PIC_W, 0x01670438);
	DSI_OUTREG32(cmdq, DSC_REG->DISP_DSC_PIC_H, 0x095f095f);
	DSI_OUTREG32(cmdq, DSC_REG->DISP_DSC_SLICE_W, 0x00b3021c);
	DSI_OUTREG32(cmdq, DSC_REG->DISP_DSC_SLICE_H, 0x012b0007);
	DSI_OUTREG32(cmdq, DSC_REG->DISP_DSC_CHUNK_SIZE, 0x0000021c);
	DSI_OUTREG32(cmdq, DSC_REG->DISP_DSC_BUF_SIZE, 0x000010e0);
	DSI_OUTREG32(cmdq, DSC_REG->DISP_DSC_MODE, 0x00000101);
	DSI_OUTREG32(cmdq, DSC_REG->DISP_DSC_CFG, 0x0000d022);
	DSI_OUTREG32(cmdq, DSC_REG->DISP_DSC_PAD, 0x00000000);

	DSI_OUTREG32(cmdq, DSC_REG->DISP_DSC_OBUF, 0x00000000);

	DSI_OUTREG32(cmdq, DSC_REG->DISP_DSC_PPS[0], 0x000c8089);
	DSI_OUTREG32(cmdq, DSC_REG->DISP_DSC_PPS[1], 0x020e00aa);
	DSI_OUTREG32(cmdq, DSC_REG->DISP_DSC_PPS[2], 0x002b0020);
	DSI_OUTREG32(cmdq, DSC_REG->DISP_DSC_PPS[3], 0x000c0007);
	DSI_OUTREG32(cmdq, DSC_REG->DISP_DSC_PPS[4], 0x0cb70db7);
	DSI_OUTREG32(cmdq, DSC_REG->DISP_DSC_PPS[5], 0x1ba01800);
	DSI_OUTREG32(cmdq, DSC_REG->DISP_DSC_PPS[6], 0x20000c03);
	DSI_OUTREG32(cmdq, DSC_REG->DISP_DSC_PPS[7], 0x330b0b06);
	DSI_OUTREG32(cmdq, DSC_REG->DISP_DSC_PPS[8], 0x382a1c0e);
	DSI_OUTREG32(cmdq, DSC_REG->DISP_DSC_PPS[9], 0x69625446);
	DSI_OUTREG32(cmdq, DSC_REG->DISP_DSC_PPS[10], 0x7b797770);
	DSI_OUTREG32(cmdq, DSC_REG->DISP_DSC_PPS[11], 0x00007e7d);
	DSI_OUTREG32(cmdq, DSC_REG->DISP_DSC_PPS[12], 0x00800880);
	DSI_OUTREG32(cmdq, DSC_REG->DISP_DSC_PPS[13], 0xf8c100a1);
	DSI_OUTREG32(cmdq, DSC_REG->DISP_DSC_PPS[14], 0xe8e3f0e3);
	DSI_OUTREG32(cmdq, DSC_REG->DISP_DSC_PPS[15], 0xe103e0e3);
	DSI_OUTREG32(cmdq, DSC_REG->DISP_DSC_PPS[16], 0xd943e123);
	DSI_OUTREG32(cmdq, DSC_REG->DISP_DSC_PPS[17], 0xd185d965);
	DSI_OUTREG32(cmdq, DSC_REG->DISP_DSC_PPS[18], 0xd1a7d1a5);
	DSI_OUTREG32(cmdq, DSC_REG->DISP_DSC_PPS[19], 0x0000d1ed);
	DSI_OUTREG32(cmdq, DSC_REG->DISP_DSC_SHADOW, 0x00000020);
#else
/*
*Resolution = 1080x2160
*Slice width = 1080
*Slice height = 20
*Format = RGB888
*DSC version = v1.2
*Compression rate = 1/3
*/
	DSI_OUTREG32(cmdq, DSC_REG->DISP_DSC_PIC_W, 0x01670438);
	DSI_OUTREG32(cmdq, DSC_REG->DISP_DSC_PIC_H, 0x086F086F);
	DSI_OUTREG32(cmdq, DSC_REG->DISP_DSC_SLICE_W, 0x01670438);
	DSI_OUTREG32(cmdq, DSC_REG->DISP_DSC_SLICE_H, 0x006B0013);
	DSI_OUTREG32(cmdq, DSC_REG->DISP_DSC_CHUNK_SIZE, 0x00000438);
	DSI_OUTREG32(cmdq, DSC_REG->DISP_DSC_BUF_SIZE, 0x00005460);
	DSI_OUTREG32(cmdq, DSC_REG->DISP_DSC_MODE, 0x00000100);
	DSI_OUTREG32(cmdq, DSC_REG->DISP_DSC_CFG, 0x0000D022);
	DSI_OUTREG32(cmdq, DSC_REG->DISP_DSC_PAD, 0x00000000);

	DSI_OUTREG32(cmdq, DSC_REG->DISP_DSC_OBUF, 0x00000000);

	DSI_OUTREG32(cmdq, DSC_REG->DISP_DSC_PPS[0], 0x000C8089);
	DSI_OUTREG32(cmdq, DSC_REG->DISP_DSC_PPS[1], 0x031C0200);
	DSI_OUTREG32(cmdq, DSC_REG->DISP_DSC_PPS[2], 0x028C0020);
	DSI_OUTREG32(cmdq, DSC_REG->DISP_DSC_PPS[3], 0x000C000F);
	DSI_OUTREG32(cmdq, DSC_REG->DISP_DSC_PPS[4], 0x028B050E);
	DSI_OUTREG32(cmdq, DSC_REG->DISP_DSC_PPS[5], 0x10F01800);
	DSI_OUTREG32(cmdq, DSC_REG->DISP_DSC_PPS[6], 0x20000C03);
	DSI_OUTREG32(cmdq, DSC_REG->DISP_DSC_PPS[7], 0x330B0B06);
	DSI_OUTREG32(cmdq, DSC_REG->DISP_DSC_PPS[8], 0x382A1C0E);
	DSI_OUTREG32(cmdq, DSC_REG->DISP_DSC_PPS[9], 0x69625446);
	DSI_OUTREG32(cmdq, DSC_REG->DISP_DSC_PPS[10], 0x7B797770);
	DSI_OUTREG32(cmdq, DSC_REG->DISP_DSC_PPS[11], 0x00007E7D);
	DSI_OUTREG32(cmdq, DSC_REG->DISP_DSC_PPS[12], 0x01040900);
	DSI_OUTREG32(cmdq, DSC_REG->DISP_DSC_PPS[13], 0xF9450125);
	DSI_OUTREG32(cmdq, DSC_REG->DISP_DSC_PPS[14], 0xE967F167);
	DSI_OUTREG32(cmdq, DSC_REG->DISP_DSC_PPS[15], 0xE187E167);
	DSI_OUTREG32(cmdq, DSC_REG->DISP_DSC_PPS[16], 0xD9C7E1A7);
	DSI_OUTREG32(cmdq, DSC_REG->DISP_DSC_PPS[17], 0xD1E9D9C9);
	DSI_OUTREG32(cmdq, DSC_REG->DISP_DSC_PPS[18], 0xD20DD1E9);
	DSI_OUTREG32(cmdq, DSC_REG->DISP_DSC_PPS[19], 0x0000D230);
	DSI_OUTREG32(cmdq, DSC_REG->DISP_DSC_SHADOW, 0x00000040);
#endif
	DISPFUNCEND();
	return 0;
}

int mipi_dsi_rx_mac_init(enum DISP_BDG_ENUM module,
			disp_ddp_path_config *config, void *cmdq)
{
	int ret = 0;
	unsigned int lanes = 0, ipi_mode_qst, out_type;
	unsigned int temp, frame_width;
	unsigned int ipi_tx_delay_qst, t_ipi_tx_delay;
	unsigned int t_ppi_clk, t_ipi_clk, t_hact_ppi, t_hact_ipi;

	/*
	 * bit0: LPRX EoTp enable
	 * bit1: LPTX EoTp enable
	 * bit2: HSRX EoTp enable
	 */
	unsigned int eotp_cfg = 4;
	unsigned int timeout = 5000;
	unsigned int phy_ready = 0, count = 0;
	LCM_DSI_PARAMS *tx_params;

	DISPFUNCSTART();
	tx_params = &(config->dsi_config);
	lanes = tx_params->LANE_NUM > 0 ? tx_params->LANE_NUM - 1 : 0;
	ipi_mode_qst = tx_params->mode == SYNC_PULSE_VDO_MODE ? 0 : 1;
	out_type = tx_params->mode == CMD_MODE ? 1 : 0;
	frame_width = tx_params->horizontal_active_pixel;

	DSI_OUTREG32(cmdq, SYS_REG->DISP_MISC0, 1);
	DSI_OUTREG32(cmdq, DSI2_REG->DSI2_DEVICE_N_LANES_OS, 3);
	DSI_OUTREG32(cmdq, DSI2_REG->DSI2_DEVICE_PHY_SHUTDOWNZ_OS, 1);
	DSI_OUTREG32(cmdq, DSI2_REG->DSI2_DEVICE_PHY_RSTZ_OS, 1);

	DSI_OUTREG32(cmdq, DSI2_REG->DSI2_DEVICE_SOFT_RSTN_OS, 0);
	udelay(1);
	DSI_OUTREG32(cmdq, DSI2_REG->DSI2_DEVICE_SOFT_RSTN_OS, 1);

	DSI_OUTREG32(cmdq, DSI2_REG->DSI2_DEVICE_VERSION_OS, 0);
	DSI_OUTREG32(cmdq, DSI2_REG->DSI2_DEVICE_N_LANES_OS, lanes);
	DSI_OUTREG32(cmdq, DSI2_REG->DSI2_DEVICE_INT_ST_MAIN_OS, 0);
	DSI_OUTREG32(cmdq, DSI2_REG->DSI2_DEVICE_EOTP_CFG_OS, eotp_cfg);

	DSI_OUTREG32(cmdq, DSI2_REG->DSI2_DEVICE_CTRL_CFG_OS, 0);

	DSI_OUTREG32(cmdq, DSI2_REG->DSI2_DEVICE_FIFO_STATUS_OS, 0);
	DSI_OUTREG32(cmdq, DSI2_REG->DSI2_DEVICE_PHY_MODE_OS,
		tx_params->IsCphy);
	DSI_OUTREG32(cmdq, DSI2_REG->DSI2_DEVICE_PHY_TEST_CTRL0_OS, 0);
	DSI_OUTREG32(cmdq, DSI2_REG->DSI2_DEVICE_PHY_TEST_CTRL1_OS, 0);
	DSI_OUTREG32(cmdq, DSI2_REG->DSI2_DEVICE_PHY_DATA_STATUS_OS, 0);

	DSI_OUTREG32(cmdq, DSI2_REG->DSI2_DEVICE_LPTXRDY_TO_CNT_OS, 0);
	DSI_OUTREG32(cmdq, DSI2_REG->DSI2_DEVICE_LPTX_TO_CNT_OS, 0);
	DSI_OUTREG32(cmdq, DSI2_REG->DSI2_DEVICE_HSRX_TO_CNT_OS, 0);

	/* Interrupt Registers */
	DSI_OUTREG32(cmdq, DSI2_REG->DSI2_DEVICE_INT_ST_PHY_FATAL_OS, 0);
	DSI_OUTREG32(cmdq, DSI2_REG->DSI2_DEVICE_INT_MASK_N_PHY_FATAL_OS,
		0xffffffff);
	DSI_OUTREG32(cmdq, DSI2_REG->DSI2_DEVICE_INT_FORCE_PHY_FATAL_OS, 0);
	DSI_OUTREG32(cmdq, DSI2_REG->DSI2_DEVICE_INT_ST_PHY_OS, 0);
	DSI_OUTREG32(cmdq, DSI2_REG->DSI2_DEVICE_INT_MASK_N_PHY_OS,
		0xffffffff);
	DSI_OUTREG32(cmdq, DSI2_REG->DSI2_DEVICE_INT_FORCE_PHY_OS, 0);
	DSI_OUTREG32(cmdq, DSI2_REG->DSI2_DEVICE_INT_ST_DSI_FATAL_OS, 0);
	DSI_OUTREG32(cmdq, DSI2_REG->DSI2_DEVICE_INT_MASK_N_DSI_FATAL_OS,
		0xffffffff);
	DSI_OUTREG32(cmdq, DSI2_REG->DSI2_DEVICE_INT_FORCE_DSI_FATAL_OS, 0);
	DSI_OUTREG32(cmdq, DSI2_REG->DSI2_DEVICE_INT_ST_DSI_OS, 0);
	DSI_OUTREG32(cmdq, DSI2_REG->DSI2_DEVICE_INT_MASK_N_DSI_OS,
		0xffffffff);
	DSI_OUTREG32(cmdq, DSI2_REG->DSI2_DEVICE_INT_FORCE_DSI_OS, 0);

	DSI_OUTREG32(cmdq, DSI2_REG->DSI2_DEVICE_INT_ST_DDI_FATAL_OS, 0);
	DSI_OUTREG32(cmdq, DSI2_REG->DSI2_DEVICE_INT_MASK_N_DDI_FATAL_OS,
		0xffffffff);
	DSI_OUTREG32(cmdq, DSI2_REG->DSI2_DEVICE_INT_FORCE_DDI_FATAL_OS, 0);
	DSI_OUTREG32(cmdq, DSI2_REG->DSI2_DEVICE_INT_ST_DDI_OS, 0);
	DSI_OUTREG32(cmdq, DSI2_REG->DSI2_DEVICE_INT_MASK_N_DDI_OS,
		0xffffffff);
	DSI_OUTREG32(cmdq, DSI2_REG->DSI2_DEVICE_INT_FORCE_DDI_OS, 0);

	DSI_OUTREG32(cmdq, DSI2_REG->DSI2_DEVICE_INT_ST_IPI_FATAL_OS, 0);
	DSI_OUTREG32(cmdq, DSI2_REG->DSI2_DEVICE_INT_MASK_N_IPI_FATAL_OS,
		0xffffffff);
	DSI_OUTREG32(cmdq, DSI2_REG->DSI2_DEVICE_INT_FORCE_IPI_FATAL_OS, 0);

	DSI_OUTREG32(cmdq, DSI2_REG->DSI2_DEVICE_INT_ST_FIFO_FATAL_OS, 0);
	DSI_OUTREG32(cmdq, DSI2_REG->DSI2_DEVICE_INT_MASK_N_FIFO_FATAL_OS,
		0xffffffff);
	DSI_OUTREG32(cmdq, DSI2_REG->DSI2_DEVICE_INT_FORCE_FIFO_FATAL_OS, 0);


	DSI_OUTREG32(cmdq, DSI2_REG->DSI2_DEVICE_INT_ST_ERR_RPT_OS, 0);
	DSI_OUTREG32(cmdq, DSI2_REG->DSI2_DEVICE_INT_MASK_N_ERR_RPT_OS,
		0xffffffff);
	DSI_OUTREG32(cmdq, DSI2_REG->DSI2_DEVICE_INT_FORCE_ERR_RPT_OS, 0);

	DSI_OUTREG32(cmdq, DSI2_REG->DSI2_DEVICE_INT_ST_RX_TRIGGERS_OS, 0);
	DSI_OUTREG32(cmdq, DSI2_REG->DSI2_DEVICE_INT_MASK_N_RX_TRIGGERS_OS,
		0xffffffff);
	DSI_OUTREG32(cmdq, DSI2_REG->DSI2_DEVICE_INT_FORCE_RX_TRIGGERS_OS, 0);

	DSI_OUTREG32(cmdq, DSI2_REG->DSI2_DEVICE_DDI_RDY_TO_CNT_OS, 0);
	DSI_OUTREG32(cmdq, DSI2_REG->DSI2_DEVICE_DDI_RESP_TO_CNT_OS, 0);
	DSI_OUTREG32(cmdq, DSI2_REG->DSI2_DEVICE_DDI_VALID_VC_CFG_OS, 0xf);
	DSI_OUTREG32(cmdq, DSI2_REG->DSI2_DEVICE_DDI_CLK_MGR_CFG_OS, 0);

	/* video mode/ipi */
	DSI_OUTREG32(cmdq, DSI2_REG->DSI2_DEVICE_IPI_MODE_CFG_OS, 0);

	if (ipi_mode_qst)
		DSI_OUTREG32(cmdq, DSI2_REG->DSI2_DEVICE_IPI_MODE_CFG_OS, 1);

	t_ipi_clk  = 1000 / MM_CLK;
	t_hact_ipi = frame_width * 1000 / MM_CLK;

	if (tx_params->IsCphy) {
		temp = 7000;
		t_ppi_clk = temp / ap_tx_data_rate;
		t_hact_ppi = ((6 + frame_width * 3) * temp /
		ap_tx_data_rate / (lanes + 1) / 2);
	} else {
		temp = 8000;
		t_ppi_clk  = temp / ap_tx_data_rate;
		t_hact_ppi = ((6 + frame_width * 3) * temp /
			ap_tx_data_rate / (lanes + 1));
	}

	if (t_hact_ppi > t_hact_ipi)
		ipi_tx_delay_qst = ((t_hact_ppi - t_hact_ipi) *
			MM_CLK + 20 * temp * MM_CLK / ap_tx_data_rate)
			/ 1000 + 4;
	else
		ipi_tx_delay_qst =  20 * temp * MM_CLK /
			ap_tx_data_rate / 1000 + 4;

	DISPMSG("ap_tx_data_rate=%d, temp=%d, t_ppi_clk=%d, t_ipi_clk=%d\n",
		ap_tx_data_rate, temp, t_ppi_clk, t_ipi_clk);
	DISPMSG("t_hact_ppi=%d, t_hact_ipi=%d\n", t_hact_ppi, t_hact_ipi);

	t_ipi_tx_delay = ipi_tx_delay_qst * 1000 / MM_CLK;

	DISPMSG("ipi_tx_delay_qst=%d, t_ipi_tx_delay=%d\n",
		ipi_tx_delay_qst, t_ipi_tx_delay);

	DSI_OUTREG32(cmdq, DSI2_REG->DSI2_DEVICE_IPI_VALID_VC_CFG_OS, 0);
	DSI_OUTREG32(cmdq, DSI2_REG->DSI2_DEVICE_IPI_TX_DELAY_OS,
		ipi_tx_delay_qst);

	while (timeout) {
		phy_ready = mtk_spi_read(
			(unsigned long)(&OCLA_REG->OCLA_PHY_READY));

		if ((phy_ready & 0x1) == 0x1)
			count++;

		if (count > 5)
			break;

		udelay(1);
		timeout--;
	}

	if (timeout == 0) {
		ret = -1;
		DISPMSG("%s, wait timeout!\n", __func__);
	}

	DISPFUNCEND();
	return ret;
}

void startup_seq_dphy_specific(unsigned int data_rate)
{
	DISPMSG("%s, data_rate=%d\n", __func__, data_rate);

	mtk_spi_mask_field_write(MIPI_RX_PHY_BASE +
		CORE_DIG_RW_COMMON_7 * 4,
		CORE_DIG_RW_COMMON_7_LANE0_HSRX_WORD_CLK_SEL_GATING_REG_MASK,
		0);

	mtk_spi_mask_field_write(MIPI_RX_PHY_BASE +
		CORE_DIG_RW_COMMON_7 * 4,
		CORE_DIG_RW_COMMON_7_LANE1_HSRX_WORD_CLK_SEL_GATING_REG_MASK,
		0);

	mtk_spi_mask_field_write(MIPI_RX_PHY_BASE +
		CORE_DIG_RW_COMMON_7 * 4,
		CORE_DIG_RW_COMMON_7_LANE2_HSRX_WORD_CLK_SEL_GATING_REG_MASK,
		0);

	mtk_spi_mask_field_write(MIPI_RX_PHY_BASE +
		CORE_DIG_RW_COMMON_7 * 4,
		CORE_DIG_RW_COMMON_7_LANE3_HSRX_WORD_CLK_SEL_GATING_REG_MASK,
		0);

	mtk_spi_mask_field_write(MIPI_RX_PHY_BASE +
		CORE_DIG_RW_COMMON_7 * 4,
		CORE_DIG_RW_COMMON_7_LANE4_HSRX_WORD_CLK_SEL_GATING_REG_MASK,
		0);

	if (data_rate >= 1500)
		mtk_spi_mask_field_write(MIPI_RX_PHY_BASE
		+ PPI_STARTUP_RW_COMMON_DPHY_7 * 4,
		PPI_STARTUP_RW_COMMON_DPHY_7_DPHY_DDL_CAL_addr_MASK, 40);
	else
		mtk_spi_mask_field_write(MIPI_RX_PHY_BASE
		+ PPI_STARTUP_RW_COMMON_DPHY_7 * 4,
		PPI_STARTUP_RW_COMMON_DPHY_7_DPHY_DDL_CAL_addr_MASK, 104);

	mtk_spi_mask_field_write(MIPI_RX_PHY_BASE
		+ PPI_STARTUP_RW_COMMON_DPHY_8 * 4,
		PPI_STARTUP_RW_COMMON_DPHY_8_CPHY_DDL_CAL_addr_MASK, 80);

	/* cfg_clk = 26 MHz */
	mtk_spi_mask_field_write(MIPI_RX_PHY_BASE
		+ PPI_RW_DDLCAL_CFG_0 * 4,
		PPI_RW_DDLCAL_CFG_0_DDLCAL_TIMEBASE_TARGET_MASK, 125);

	mtk_spi_mask_field_write(MIPI_RX_PHY_BASE
		+ PPI_RW_DDLCAL_CFG_7 * 4,
		PPI_RW_DDLCAL_CFG_7_DDLCAL_DECR_WAIT_MASK, 34);

	mtk_spi_mask_field_write(MIPI_RX_PHY_BASE
		+ PPI_RW_DDLCAL_CFG_1 * 4,
		PPI_RW_DDLCAL_CFG_1_DDLCAL_DISABLE_TIME_MASK, 25);

	mtk_spi_mask_field_write(MIPI_RX_PHY_BASE
		+ PPI_RW_DDLCAL_CFG_2 * 4,
		PPI_RW_DDLCAL_CFG_2_DDLCAL_WAIT_MASK, 4);

	mtk_spi_mask_field_write(MIPI_RX_PHY_BASE
		+ PPI_RW_DDLCAL_CFG_2 * 4,
		PPI_RW_DDLCAL_CFG_2_DDLCAL_TUNE_MODE_MASK, 2);

	mtk_spi_mask_field_write(MIPI_RX_PHY_BASE
		+ PPI_RW_DDLCAL_CFG_2 * 4,
		PPI_RW_DDLCAL_CFG_2_DDLCAL_DDL_DLL_MASK, 1);

	/* cfg_clk = 26 MHz */
	mtk_spi_mask_field_write(MIPI_RX_PHY_BASE
		+ PPI_RW_DDLCAL_CFG_2 * 4,
		PPI_RW_DDLCAL_CFG_2_DDLCAL_ENABLE_WAIT_MASK, 25);

	mtk_spi_mask_field_write(MIPI_RX_PHY_BASE
		+ PPI_RW_DDLCAL_CFG_2 * 4,
		PPI_RW_DDLCAL_CFG_2_DDLCAL_UPDATE_SETTINGS_MASK, 1);

	mtk_spi_mask_field_write(MIPI_RX_PHY_BASE
		+ PPI_RW_DDLCAL_CFG_4 * 4,
		PPI_RW_DDLCAL_CFG_4_DDLCAL_STUCK_THRESH_MASK, 10);

	mtk_spi_mask_field_write(MIPI_RX_PHY_BASE
		+ PPI_RW_DDLCAL_CFG_6 * 4,
		PPI_RW_DDLCAL_CFG_6_DDLCAL_MAX_DIFF_MASK, 10);

	/* cfg_clk = 26 MHz */
	mtk_spi_mask_field_write(MIPI_RX_PHY_BASE
		+ PPI_RW_DDLCAL_CFG_7 * 4,
		PPI_RW_DDLCAL_CFG_7_DDLCAL_START_DELAY_MASK, 12);

	if (data_rate > 1500) {
		mtk_spi_mask_field_write(MIPI_RX_PHY_BASE
			+ PPI_RW_DDLCAL_CFG_3 * 4,
			PPI_RW_DDLCAL_CFG_3_DDLCAL_COUNTER_REF_MASK,
			ddl_cntr_ref_reg);

		mtk_spi_mask_field_write(MIPI_RX_PHY_BASE
			+ PPI_RW_DDLCAL_CFG_1 * 4,
			PPI_RW_DDLCAL_CFG_1_DDLCAL_MAX_PHASE_MASK,
			max_phase);

		mtk_spi_mask_field_write(MIPI_RX_PHY_BASE
			+ PPI_RW_DDLCAL_CFG_5 * 4,
			PPI_RW_DDLCAL_CFG_5_DDLCAL_DLL_FBK_MASK,
			dll_fbk);

		mtk_spi_mask_field_write(MIPI_RX_PHY_BASE
			+ PPI_RW_DDLCAL_CFG_5 * 4,
			PPI_RW_DDLCAL_CFG_5_DDLCAL_DDL_COARSE_BANK_MASK,
			coarse_bank);
	}

	mtk_spi_mask_field_write(MIPI_RX_PHY_BASE
		+ CORE_DIG_IOCTRL_RW_AFE_LANE0_CTRL_2_8 * 4,
		CORE_DIG_IOCTRL_RW_AFE_LANE0_CTRL_2_8_OA_LANE0_HSRX_CDPHY_SEL_FAST_MASK,
		sel_fast);

	mtk_spi_mask_field_write(MIPI_RX_PHY_BASE
		+ CORE_DIG_IOCTRL_RW_AFE_LANE1_CTRL_2_8 * 4,
		CORE_DIG_IOCTRL_RW_AFE_LANE1_CTRL_2_8_OA_LANE1_HSRX_CDPHY_SEL_FAST_MASK,
		sel_fast);

	mtk_spi_mask_field_write(MIPI_RX_PHY_BASE
		+ CORE_DIG_IOCTRL_RW_AFE_LANE2_CTRL_2_8 * 4,
		CORE_DIG_IOCTRL_RW_AFE_LANE2_CTRL_2_8_OA_LANE2_HSRX_CDPHY_SEL_FAST_MASK,
		sel_fast);

	mtk_spi_mask_field_write(MIPI_RX_PHY_BASE
		+ CORE_DIG_IOCTRL_RW_AFE_LANE3_CTRL_2_8 * 4,
		CORE_DIG_IOCTRL_RW_AFE_LANE3_CTRL_2_8_OA_LANE3_HSRX_CDPHY_SEL_FAST_MASK,
		sel_fast);

	mtk_spi_mask_field_write(MIPI_RX_PHY_BASE
		+ CORE_DIG_IOCTRL_RW_AFE_LANE4_CTRL_2_8 * 4,
		CORE_DIG_IOCTRL_RW_AFE_LANE4_CTRL_2_8_OA_LANE4_HSRX_CDPHY_SEL_FAST_MASK,
		sel_fast);

	mtk_spi_mask_field_write(MIPI_RX_PHY_BASE
		+ CORE_DIG_DLANE_0_RW_LP_0 * 4,
		CORE_DIG_DLANE_0_RW_LP_0_LP_0_TTAGO_REG_MASK, 6);

	mtk_spi_mask_field_write(MIPI_RX_PHY_BASE
		+ CORE_DIG_DLANE_1_RW_LP_0 * 4,
		CORE_DIG_DLANE_1_RW_LP_0_LP_0_TTAGO_REG_MASK, 6);

	mtk_spi_mask_field_write(MIPI_RX_PHY_BASE
		+ CORE_DIG_DLANE_2_RW_LP_0 * 4,
		CORE_DIG_DLANE_2_RW_LP_0_LP_0_TTAGO_REG_MASK, 6);

	mtk_spi_mask_field_write(MIPI_RX_PHY_BASE
		+ CORE_DIG_DLANE_3_RW_LP_0 * 4,
		CORE_DIG_DLANE_3_RW_LP_0_LP_0_TTAGO_REG_MASK, 6);

	mtk_spi_mask_field_write(MIPI_RX_PHY_BASE
		+ CORE_DIG_IOCTRL_RW_AFE_LANE0_CTRL_2_2 * 4,
		CORE_DIG_IOCTRL_RW_AFE_LANE0_CTRL_2_2_OA_LANE0_SEL_LANE_CFG_MASK, 0);

	mtk_spi_mask_field_write(MIPI_RX_PHY_BASE
		+ CORE_DIG_IOCTRL_RW_AFE_LANE1_CTRL_2_2 * 4,
		CORE_DIG_IOCTRL_RW_AFE_LANE1_CTRL_2_2_OA_LANE1_SEL_LANE_CFG_MASK, 0);

	mtk_spi_mask_field_write(MIPI_RX_PHY_BASE
		+ CORE_DIG_IOCTRL_RW_AFE_LANE2_CTRL_2_2 * 4,
		CORE_DIG_IOCTRL_RW_AFE_LANE2_CTRL_2_2_OA_LANE2_SEL_LANE_CFG_MASK, 1);

	mtk_spi_mask_field_write(MIPI_RX_PHY_BASE
		+ CORE_DIG_IOCTRL_RW_AFE_LANE3_CTRL_2_2 * 4,
		CORE_DIG_IOCTRL_RW_AFE_LANE3_CTRL_2_2_OA_LANE3_SEL_LANE_CFG_MASK, 0);

	mtk_spi_mask_field_write(MIPI_RX_PHY_BASE
		+ CORE_DIG_IOCTRL_RW_AFE_LANE4_CTRL_2_2 * 4,
		CORE_DIG_IOCTRL_RW_AFE_LANE4_CTRL_2_2_OA_LANE4_SEL_LANE_CFG_MASK, 0);

	mtk_spi_mask_field_write(MIPI_RX_PHY_BASE
		+ CORE_DIG_RW_COMMON_6 * 4,
		CORE_DIG_RW_COMMON_6_DESERIALIZER_EN_DEASS_COUNT_THRESH_D_MASK, 1);

	mtk_spi_mask_field_write(MIPI_RX_PHY_BASE
		+ CORE_DIG_RW_COMMON_6 * 4,
		CORE_DIG_RW_COMMON_6_DESERIALIZER_DIV_EN_DELAY_THRESH_D_MASK, 1);

	if (data_rate > 1500) {
		mtk_spi_mask_field_write(MIPI_RX_PHY_BASE
			+ CORE_DIG_IOCTRL_RW_AFE_LANE0_CTRL_2_12 * 4,
			CORE_DIG_IOCTRL_RW_AFE_LANE0_CTRL_2_12_OA_LANE0_HSRX_DPHY_DDL_BYPASS_EN_OVR_VAL_MASK, 0);

		mtk_spi_mask_field_write(MIPI_RX_PHY_BASE
			+ CORE_DIG_IOCTRL_RW_AFE_LANE1_CTRL_2_12 * 4,
			CORE_DIG_IOCTRL_RW_AFE_LANE1_CTRL_2_12_OA_LANE1_HSRX_DPHY_DDL_BYPASS_EN_OVR_VAL_MASK, 0);

		mtk_spi_mask_field_write(MIPI_RX_PHY_BASE
			+ CORE_DIG_IOCTRL_RW_AFE_LANE2_CTRL_2_12 * 4,
			CORE_DIG_IOCTRL_RW_AFE_LANE2_CTRL_2_12_OA_LANE2_HSRX_DPHY_DDL_BYPASS_EN_OVR_VAL_MASK, 0);

		mtk_spi_mask_field_write(MIPI_RX_PHY_BASE
			+ CORE_DIG_IOCTRL_RW_AFE_LANE3_CTRL_2_12 * 4,
			CORE_DIG_IOCTRL_RW_AFE_LANE3_CTRL_2_12_OA_LANE3_HSRX_DPHY_DDL_BYPASS_EN_OVR_VAL_MASK, 0);

		mtk_spi_mask_field_write(MIPI_RX_PHY_BASE
			+ CORE_DIG_IOCTRL_RW_AFE_LANE4_CTRL_2_12 * 4,
			CORE_DIG_IOCTRL_RW_AFE_LANE4_CTRL_2_12_OA_LANE4_HSRX_DPHY_DDL_BYPASS_EN_OVR_VAL_MASK, 0);

		mtk_spi_mask_field_write(MIPI_RX_PHY_BASE
			+ CORE_DIG_IOCTRL_RW_AFE_LANE0_CTRL_2_13 * 4,
			CORE_DIG_IOCTRL_RW_AFE_LANE0_CTRL_2_13_OA_LANE0_HSRX_DPHY_DDL_BYPASS_EN_OVR_EN_MASK, 0);

		mtk_spi_mask_field_write(MIPI_RX_PHY_BASE
			+ CORE_DIG_IOCTRL_RW_AFE_LANE1_CTRL_2_13 * 4,
			CORE_DIG_IOCTRL_RW_AFE_LANE1_CTRL_2_13_OA_LANE1_HSRX_DPHY_DDL_BYPASS_EN_OVR_EN_MASK, 0);

		mtk_spi_mask_field_write(MIPI_RX_PHY_BASE
			+ CORE_DIG_IOCTRL_RW_AFE_LANE2_CTRL_2_13 * 4,
			CORE_DIG_IOCTRL_RW_AFE_LANE2_CTRL_2_13_OA_LANE2_HSRX_DPHY_DDL_BYPASS_EN_OVR_EN_MASK, 0);

		mtk_spi_mask_field_write(MIPI_RX_PHY_BASE
			+ CORE_DIG_IOCTRL_RW_AFE_LANE3_CTRL_2_13 * 4,
			CORE_DIG_IOCTRL_RW_AFE_LANE3_CTRL_2_13_OA_LANE3_HSRX_DPHY_DDL_BYPASS_EN_OVR_EN_MASK, 0);

		mtk_spi_mask_field_write(MIPI_RX_PHY_BASE
			+ CORE_DIG_IOCTRL_RW_AFE_LANE4_CTRL_2_13 * 4,
			CORE_DIG_IOCTRL_RW_AFE_LANE4_CTRL_2_13_OA_LANE4_HSRX_DPHY_DDL_BYPASS_EN_OVR_EN_MASK, 0);
	} else {
		mtk_spi_mask_field_write(MIPI_RX_PHY_BASE
			+ CORE_DIG_IOCTRL_RW_AFE_LANE0_CTRL_2_12 * 4,
			CORE_DIG_IOCTRL_RW_AFE_LANE0_CTRL_2_12_OA_LANE0_HSRX_DPHY_DDL_BYPASS_EN_OVR_VAL_MASK, 1);

		mtk_spi_mask_field_write(MIPI_RX_PHY_BASE
			+ CORE_DIG_IOCTRL_RW_AFE_LANE1_CTRL_2_12 * 4,
			CORE_DIG_IOCTRL_RW_AFE_LANE1_CTRL_2_12_OA_LANE1_HSRX_DPHY_DDL_BYPASS_EN_OVR_VAL_MASK, 1);

		mtk_spi_mask_field_write(MIPI_RX_PHY_BASE
			+ CORE_DIG_IOCTRL_RW_AFE_LANE2_CTRL_2_12 * 4,
			CORE_DIG_IOCTRL_RW_AFE_LANE2_CTRL_2_12_OA_LANE2_HSRX_DPHY_DDL_BYPASS_EN_OVR_VAL_MASK, 1);

		mtk_spi_mask_field_write(MIPI_RX_PHY_BASE
			+ CORE_DIG_IOCTRL_RW_AFE_LANE3_CTRL_2_12 * 4,
			CORE_DIG_IOCTRL_RW_AFE_LANE3_CTRL_2_12_OA_LANE3_HSRX_DPHY_DDL_BYPASS_EN_OVR_VAL_MASK, 1);

		mtk_spi_mask_field_write(MIPI_RX_PHY_BASE
			+ CORE_DIG_IOCTRL_RW_AFE_LANE4_CTRL_2_12 * 4,
			CORE_DIG_IOCTRL_RW_AFE_LANE4_CTRL_2_12_OA_LANE4_HSRX_DPHY_DDL_BYPASS_EN_OVR_VAL_MASK, 1);

		mtk_spi_mask_field_write(MIPI_RX_PHY_BASE
			+ CORE_DIG_IOCTRL_RW_AFE_LANE0_CTRL_2_13 * 4,
			CORE_DIG_IOCTRL_RW_AFE_LANE0_CTRL_2_13_OA_LANE0_HSRX_DPHY_DDL_BYPASS_EN_OVR_EN_MASK, 1);

		mtk_spi_mask_field_write(MIPI_RX_PHY_BASE
			+ CORE_DIG_IOCTRL_RW_AFE_LANE1_CTRL_2_13 * 4,
			CORE_DIG_IOCTRL_RW_AFE_LANE1_CTRL_2_13_OA_LANE1_HSRX_DPHY_DDL_BYPASS_EN_OVR_EN_MASK, 1);

		mtk_spi_mask_field_write(MIPI_RX_PHY_BASE
			+ CORE_DIG_IOCTRL_RW_AFE_LANE2_CTRL_2_13 * 4,
			CORE_DIG_IOCTRL_RW_AFE_LANE2_CTRL_2_13_OA_LANE2_HSRX_DPHY_DDL_BYPASS_EN_OVR_EN_MASK, 1);

		mtk_spi_mask_field_write(MIPI_RX_PHY_BASE
			+ CORE_DIG_IOCTRL_RW_AFE_LANE3_CTRL_2_13 * 4,
			CORE_DIG_IOCTRL_RW_AFE_LANE3_CTRL_2_13_OA_LANE3_HSRX_DPHY_DDL_BYPASS_EN_OVR_EN_MASK, 1);

		mtk_spi_mask_field_write(MIPI_RX_PHY_BASE
			+ CORE_DIG_IOCTRL_RW_AFE_LANE4_CTRL_2_13 * 4,
			CORE_DIG_IOCTRL_RW_AFE_LANE4_CTRL_2_13_OA_LANE4_HSRX_DPHY_DDL_BYPASS_EN_OVR_EN_MASK, 1);
	}

	mtk_spi_mask_field_write(MIPI_RX_PHY_BASE
		+ CORE_DIG_IOCTRL_RW_AFE_LANE2_CTRL_2_9 * 4,
		CORE_DIG_IOCTRL_RW_AFE_LANE2_CTRL_2_9_OA_LANE2_HSRX_HS_CLK_DIV_MASK,
		hsrx_clk_div);

	mtk_spi_mask_field_write(MIPI_RX_PHY_BASE
		+ CORE_DIG_DLANE_CLK_RW_HS_RX_0 * 4,
		CORE_DIG_DLANE_CLK_RW_HS_RX_0_HS_RX_0_TCLKSETTLE_REG_MASK, 28);

	mtk_spi_mask_field_write(MIPI_RX_PHY_BASE
		+ CORE_DIG_DLANE_CLK_RW_HS_RX_7 * 4,
		CORE_DIG_DLANE_CLK_RW_HS_RX_7_HS_RX_7_TCLKMISS_REG_MASK, 6);

	mtk_spi_mask_field_write(MIPI_RX_PHY_BASE
		+ CORE_DIG_DLANE_0_RW_HS_RX_0 * 4,
		CORE_DIG_DLANE_0_RW_HS_RX_0_HS_RX_0_THSSETTLE_REG_MASK,
		hs_thssettle);

	mtk_spi_mask_field_write(MIPI_RX_PHY_BASE
		+ CORE_DIG_DLANE_1_RW_HS_RX_0 * 4,
		CORE_DIG_DLANE_1_RW_HS_RX_0_HS_RX_0_THSSETTLE_REG_MASK,
		hs_thssettle);

	mtk_spi_mask_field_write(MIPI_RX_PHY_BASE
		+ CORE_DIG_DLANE_2_RW_HS_RX_0 * 4,
		CORE_DIG_DLANE_2_RW_HS_RX_0_HS_RX_0_THSSETTLE_REG_MASK,
		hs_thssettle);

	mtk_spi_mask_field_write(MIPI_RX_PHY_BASE
		+ CORE_DIG_DLANE_3_RW_HS_RX_0 * 4,
		CORE_DIG_DLANE_3_RW_HS_RX_0_HS_RX_0_THSSETTLE_REG_MASK,
		hs_thssettle);

	if (data_rate > 1500) {
		mtk_spi_mask_field_write(MIPI_RX_PHY_BASE
			+ CORE_DIG_DLANE_0_RW_CFG_1 * 4,
			CORE_DIG_DLANE_0_RW_CFG_1_CFG_1_DESKEW_SUPPORTED_REG_MASK,
			1);

		mtk_spi_mask_field_write(MIPI_RX_PHY_BASE
			+ CORE_DIG_DLANE_1_RW_CFG_1 * 4,
			CORE_DIG_DLANE_1_RW_CFG_1_CFG_1_DESKEW_SUPPORTED_REG_MASK,
			1);

		mtk_spi_mask_field_write(MIPI_RX_PHY_BASE
			+ CORE_DIG_DLANE_2_RW_CFG_1 * 4,
			CORE_DIG_DLANE_2_RW_CFG_1_CFG_1_DESKEW_SUPPORTED_REG_MASK,
			1);

		mtk_spi_mask_field_write(MIPI_RX_PHY_BASE
			+ CORE_DIG_DLANE_3_RW_CFG_1 * 4,
			CORE_DIG_DLANE_3_RW_CFG_1_CFG_1_DESKEW_SUPPORTED_REG_MASK,
			1);

		mtk_spi_mask_field_write(MIPI_RX_PHY_BASE
			+ CORE_DIG_DLANE_0_RW_CFG_1 * 4,
			CORE_DIG_DLANE_0_RW_CFG_1_CFG_1_SOT_DETECTION_REG_MASK,
			0);

		mtk_spi_mask_field_write(MIPI_RX_PHY_BASE
			+ CORE_DIG_DLANE_1_RW_CFG_1 * 4,
			CORE_DIG_DLANE_1_RW_CFG_1_CFG_1_SOT_DETECTION_REG_MASK,
			0);

		mtk_spi_mask_field_write(MIPI_RX_PHY_BASE
			+ CORE_DIG_DLANE_2_RW_CFG_1 * 4,
			CORE_DIG_DLANE_2_RW_CFG_1_CFG_1_SOT_DETECTION_REG_MASK,
			0);

		mtk_spi_mask_field_write(MIPI_RX_PHY_BASE
			+ CORE_DIG_DLANE_3_RW_CFG_1 * 4,
			CORE_DIG_DLANE_3_RW_CFG_1_CFG_1_SOT_DETECTION_REG_MASK,
			0);
	} else {
		mtk_spi_mask_field_write(MIPI_RX_PHY_BASE
			+ CORE_DIG_DLANE_0_RW_CFG_1 * 4,
			CORE_DIG_DLANE_0_RW_CFG_1_CFG_1_DESKEW_SUPPORTED_REG_MASK,
			0);

		mtk_spi_mask_field_write(MIPI_RX_PHY_BASE
			+ CORE_DIG_DLANE_1_RW_CFG_1 * 4,
			CORE_DIG_DLANE_1_RW_CFG_1_CFG_1_DESKEW_SUPPORTED_REG_MASK,
			0);

		mtk_spi_mask_field_write(MIPI_RX_PHY_BASE
			+ CORE_DIG_DLANE_2_RW_CFG_1 * 4,
			CORE_DIG_DLANE_2_RW_CFG_1_CFG_1_DESKEW_SUPPORTED_REG_MASK,
			0);

		mtk_spi_mask_field_write(MIPI_RX_PHY_BASE
			+ CORE_DIG_DLANE_3_RW_CFG_1 * 4,
			CORE_DIG_DLANE_3_RW_CFG_1_CFG_1_DESKEW_SUPPORTED_REG_MASK,
			0);

		mtk_spi_mask_field_write(MIPI_RX_PHY_BASE
			+ CORE_DIG_DLANE_0_RW_CFG_1 * 4,
			CORE_DIG_DLANE_0_RW_CFG_1_CFG_1_SOT_DETECTION_REG_MASK,
			1);

		mtk_spi_mask_field_write(MIPI_RX_PHY_BASE
			+ CORE_DIG_DLANE_1_RW_CFG_1 * 4,
			CORE_DIG_DLANE_1_RW_CFG_1_CFG_1_SOT_DETECTION_REG_MASK,
			1);

		mtk_spi_mask_field_write(MIPI_RX_PHY_BASE
			+ CORE_DIG_DLANE_2_RW_CFG_1 * 4,
			CORE_DIG_DLANE_2_RW_CFG_1_CFG_1_SOT_DETECTION_REG_MASK,
			1);

		mtk_spi_mask_field_write(MIPI_RX_PHY_BASE
			+ CORE_DIG_DLANE_3_RW_CFG_1 * 4,
			CORE_DIG_DLANE_3_RW_CFG_1_CFG_1_SOT_DETECTION_REG_MASK,
			1);
	}

	if (data_rate > 2500) {
		mtk_spi_mask_field_write(MIPI_RX_PHY_BASE
			+ CORE_DIG_DLANE_0_RW_HS_RX_2 * 4,
			CORE_DIG_DLANE_0_RW_HS_RX_2_HS_RX_2_IGNORE_ALTERNCAL_REG_MASK,
			0);

		mtk_spi_mask_field_write(MIPI_RX_PHY_BASE
			+ CORE_DIG_DLANE_1_RW_HS_RX_2 * 4,
			CORE_DIG_DLANE_1_RW_HS_RX_2_HS_RX_2_IGNORE_ALTERNCAL_REG_MASK,
			0);

		mtk_spi_mask_field_write(MIPI_RX_PHY_BASE
			+ CORE_DIG_DLANE_2_RW_HS_RX_2 * 4,
			CORE_DIG_DLANE_2_RW_HS_RX_2_HS_RX_2_IGNORE_ALTERNCAL_REG_MASK,
			0);

		mtk_spi_mask_field_write(MIPI_RX_PHY_BASE
			+ CORE_DIG_DLANE_3_RW_HS_RX_2 * 4,
			CORE_DIG_DLANE_3_RW_HS_RX_2_HS_RX_2_IGNORE_ALTERNCAL_REG_MASK,
			0);

		mtk_spi_mask_field_write(MIPI_RX_PHY_BASE
			+ CORE_DIG_DLANE_CLK_RW_HS_RX_2 * 4,
			CORE_DIG_DLANE_CLK_RW_HS_RX_2_HS_RX_2_IGNORE_ALTERNCAL_REG_MASK,
			0);
	} else {
		mtk_spi_mask_field_write(MIPI_RX_PHY_BASE
			+ CORE_DIG_DLANE_0_RW_HS_RX_2 * 4,
			CORE_DIG_DLANE_0_RW_HS_RX_2_HS_RX_2_IGNORE_ALTERNCAL_REG_MASK,
			1);

		mtk_spi_mask_field_write(MIPI_RX_PHY_BASE
			+ CORE_DIG_DLANE_1_RW_HS_RX_2 * 4,
			CORE_DIG_DLANE_1_RW_HS_RX_2_HS_RX_2_IGNORE_ALTERNCAL_REG_MASK,
			1);

		mtk_spi_mask_field_write(MIPI_RX_PHY_BASE
			+ CORE_DIG_DLANE_2_RW_HS_RX_2 * 4,
			CORE_DIG_DLANE_2_RW_HS_RX_2_HS_RX_2_IGNORE_ALTERNCAL_REG_MASK,
			1);

		mtk_spi_mask_field_write(MIPI_RX_PHY_BASE
			+ CORE_DIG_DLANE_3_RW_HS_RX_2 * 4,
			CORE_DIG_DLANE_3_RW_HS_RX_2_HS_RX_2_IGNORE_ALTERNCAL_REG_MASK,
			1);

		mtk_spi_mask_field_write(MIPI_RX_PHY_BASE
			+ CORE_DIG_DLANE_CLK_RW_HS_RX_2 * 4,
			CORE_DIG_DLANE_CLK_RW_HS_RX_2_HS_RX_2_IGNORE_ALTERNCAL_REG_MASK,
			1);
	}

	mtk_spi_mask_field_write(MIPI_RX_PHY_BASE
		+ CORE_DIG_DLANE_0_RW_HS_RX_2 * 4,
		CORE_DIG_DLANE_0_RW_HS_RX_2_HS_RX_2_UPDATE_SETTINGS_DESKEW_REG_MASK,
		1);

	mtk_spi_mask_field_write(MIPI_RX_PHY_BASE
		+ CORE_DIG_DLANE_1_RW_HS_RX_2 * 4,
		CORE_DIG_DLANE_1_RW_HS_RX_2_HS_RX_2_UPDATE_SETTINGS_DESKEW_REG_MASK,
		1);

	mtk_spi_mask_field_write(MIPI_RX_PHY_BASE
		+ CORE_DIG_DLANE_2_RW_HS_RX_2 * 4,
		CORE_DIG_DLANE_2_RW_HS_RX_2_HS_RX_2_UPDATE_SETTINGS_DESKEW_REG_MASK,
		1);

	mtk_spi_mask_field_write(MIPI_RX_PHY_BASE
		+ CORE_DIG_DLANE_3_RW_HS_RX_2 * 4,
		CORE_DIG_DLANE_3_RW_HS_RX_2_HS_RX_2_UPDATE_SETTINGS_DESKEW_REG_MASK,
		1);

	mtk_spi_mask_field_write(MIPI_RX_PHY_BASE
		+ CORE_DIG_DLANE_CLK_RW_HS_RX_2 * 4,
		CORE_DIG_DLANE_CLK_RW_HS_RX_2_HS_RX_2_UPDATE_SETTINGS_DESKEW_REG_MASK,
		1);

	mtk_spi_mask_field_write(MIPI_RX_PHY_BASE
		+ CORE_DIG_DLANE_0_RW_HS_RX_1 * 4,
		CORE_DIG_DLANE_0_RW_HS_RX_1_HS_RX_1_FILTER_SIZE_DESKEW_REG_MASK,
		16);

	mtk_spi_mask_field_write(MIPI_RX_PHY_BASE
		+ CORE_DIG_DLANE_1_RW_HS_RX_1 * 4,
		CORE_DIG_DLANE_1_RW_HS_RX_1_HS_RX_1_FILTER_SIZE_DESKEW_REG_MASK,
		16);

	mtk_spi_mask_field_write(MIPI_RX_PHY_BASE
		+ CORE_DIG_DLANE_2_RW_HS_RX_1 * 4,
		CORE_DIG_DLANE_2_RW_HS_RX_1_HS_RX_1_FILTER_SIZE_DESKEW_REG_MASK,
		16);

	mtk_spi_mask_field_write(MIPI_RX_PHY_BASE +
		CORE_DIG_DLANE_3_RW_HS_RX_1 * 4,
		CORE_DIG_DLANE_3_RW_HS_RX_1_HS_RX_1_FILTER_SIZE_DESKEW_REG_MASK,
		16);

	mtk_spi_mask_field_write(MIPI_RX_PHY_BASE +
		CORE_DIG_DLANE_0_RW_HS_RX_2 * 4,
		CORE_DIG_DLANE_0_RW_HS_RX_2_HS_RX_2_WINDOW_SIZE_DESKEW_REG_MASK,
		3);

	mtk_spi_mask_field_write(MIPI_RX_PHY_BASE +
		CORE_DIG_DLANE_1_RW_HS_RX_2 * 4,
		CORE_DIG_DLANE_1_RW_HS_RX_2_HS_RX_2_WINDOW_SIZE_DESKEW_REG_MASK,
		3);

	mtk_spi_mask_field_write(MIPI_RX_PHY_BASE +
		CORE_DIG_DLANE_2_RW_HS_RX_2 * 4,
		CORE_DIG_DLANE_2_RW_HS_RX_2_HS_RX_2_WINDOW_SIZE_DESKEW_REG_MASK,
		3);

	mtk_spi_mask_field_write(MIPI_RX_PHY_BASE +
		CORE_DIG_DLANE_3_RW_HS_RX_2 * 4,
		CORE_DIG_DLANE_3_RW_HS_RX_2_HS_RX_2_WINDOW_SIZE_DESKEW_REG_MASK,
		3);

	mtk_spi_mask_field_write(MIPI_RX_PHY_BASE +
		CORE_DIG_DLANE_CLK_RW_HS_RX_2 * 4,
		CORE_DIG_DLANE_CLK_RW_HS_RX_2_HS_RX_2_WINDOW_SIZE_DESKEW_REG_MASK,
		3);

	mtk_spi_mask_field_write(MIPI_RX_PHY_BASE +
		CORE_DIG_DLANE_0_RW_HS_RX_3 * 4,
		CORE_DIG_DLANE_0_RW_HS_RX_3_HS_RX_3_STEP_SIZE_DESKEW_REG_MASK,
		1);

	mtk_spi_mask_field_write(MIPI_RX_PHY_BASE +
		CORE_DIG_DLANE_1_RW_HS_RX_3 * 4,
		CORE_DIG_DLANE_1_RW_HS_RX_3_HS_RX_3_STEP_SIZE_DESKEW_REG_MASK,
		1);

	mtk_spi_mask_field_write(MIPI_RX_PHY_BASE +
		CORE_DIG_DLANE_2_RW_HS_RX_3 * 4,
		CORE_DIG_DLANE_2_RW_HS_RX_3_HS_RX_3_STEP_SIZE_DESKEW_REG_MASK,
		1);

	mtk_spi_mask_field_write(MIPI_RX_PHY_BASE +
		CORE_DIG_DLANE_3_RW_HS_RX_3 * 4,
		CORE_DIG_DLANE_3_RW_HS_RX_3_HS_RX_3_STEP_SIZE_DESKEW_REG_MASK,
		1);

	mtk_spi_mask_field_write(MIPI_RX_PHY_BASE +
		CORE_DIG_DLANE_0_RW_HS_RX_4 * 4,
		CORE_DIG_DLANE_0_RW_HS_RX_4_HS_RX_4_MAX_ITERATIONS_DESKEW_REG_MASK,
		150);

	mtk_spi_mask_field_write(MIPI_RX_PHY_BASE +
		CORE_DIG_DLANE_1_RW_HS_RX_4 * 4,
		CORE_DIG_DLANE_1_RW_HS_RX_4_HS_RX_4_MAX_ITERATIONS_DESKEW_REG_MASK,
		150);

	mtk_spi_mask_field_write(MIPI_RX_PHY_BASE +
		CORE_DIG_DLANE_2_RW_HS_RX_4 * 4,
		CORE_DIG_DLANE_2_RW_HS_RX_4_HS_RX_4_MAX_ITERATIONS_DESKEW_REG_MASK,
		150);

	mtk_spi_mask_field_write(MIPI_RX_PHY_BASE +
		CORE_DIG_DLANE_3_RW_HS_RX_4 * 4,
		CORE_DIG_DLANE_3_RW_HS_RX_4_HS_RX_4_MAX_ITERATIONS_DESKEW_REG_MASK,
		150);

	mtk_spi_mask_field_write(MIPI_RX_PHY_BASE +
		CORE_DIG_DLANE_0_RW_HS_RX_5 * 4,
		CORE_DIG_DLANE_0_RW_HS_RX_5_HS_RX_5_DDL_LEFT_INIT_REG_MASK,
		0);

	mtk_spi_mask_field_write(MIPI_RX_PHY_BASE +
		CORE_DIG_DLANE_1_RW_HS_RX_5 * 4,
		CORE_DIG_DLANE_1_RW_HS_RX_5_HS_RX_5_DDL_LEFT_INIT_REG_MASK,
		0);

	mtk_spi_mask_field_write(MIPI_RX_PHY_BASE +
		CORE_DIG_DLANE_2_RW_HS_RX_5 * 4,
		CORE_DIG_DLANE_2_RW_HS_RX_5_HS_RX_5_DDL_LEFT_INIT_REG_MASK,
		0);

	mtk_spi_mask_field_write(MIPI_RX_PHY_BASE +
		CORE_DIG_DLANE_3_RW_HS_RX_5 * 4,
		CORE_DIG_DLANE_3_RW_HS_RX_5_HS_RX_5_DDL_LEFT_INIT_REG_MASK,
		0);

	mtk_spi_mask_field_write(MIPI_RX_PHY_BASE +
		CORE_DIG_DLANE_0_RW_HS_RX_5 * 4,
		CORE_DIG_DLANE_0_RW_HS_RX_5_HS_RX_5_DDL_MID_INIT_REG_MASK,
		1);

	mtk_spi_mask_field_write(MIPI_RX_PHY_BASE +
		CORE_DIG_DLANE_1_RW_HS_RX_5 * 4,
		CORE_DIG_DLANE_1_RW_HS_RX_5_HS_RX_5_DDL_MID_INIT_REG_MASK,
		1);

	mtk_spi_mask_field_write(MIPI_RX_PHY_BASE +
		CORE_DIG_DLANE_2_RW_HS_RX_5 * 4,
		CORE_DIG_DLANE_2_RW_HS_RX_5_HS_RX_5_DDL_MID_INIT_REG_MASK,
		1);

	mtk_spi_mask_field_write(MIPI_RX_PHY_BASE +
		CORE_DIG_DLANE_3_RW_HS_RX_5 * 4,
		CORE_DIG_DLANE_3_RW_HS_RX_5_HS_RX_5_DDL_MID_INIT_REG_MASK,
		1);

	mtk_spi_mask_field_write(MIPI_RX_PHY_BASE +
		CORE_DIG_DLANE_0_RW_HS_RX_6 * 4,
		CORE_DIG_DLANE_0_RW_HS_RX_6_HS_RX_6_DDL_RIGHT_INIT_REG_MASK,
		2);

	mtk_spi_mask_field_write(MIPI_RX_PHY_BASE +
		CORE_DIG_DLANE_1_RW_HS_RX_6 * 4,
		CORE_DIG_DLANE_1_RW_HS_RX_6_HS_RX_6_DDL_RIGHT_INIT_REG_MASK,
		2);

	mtk_spi_mask_field_write(MIPI_RX_PHY_BASE +
		CORE_DIG_DLANE_2_RW_HS_RX_6 * 4,
		CORE_DIG_DLANE_2_RW_HS_RX_6_HS_RX_6_DDL_RIGHT_INIT_REG_MASK,
		2);

	mtk_spi_mask_field_write(MIPI_RX_PHY_BASE +
		CORE_DIG_DLANE_3_RW_HS_RX_6 * 4,
		CORE_DIG_DLANE_3_RW_HS_RX_6_HS_RX_6_DDL_RIGHT_INIT_REG_MASK,
		2);

	mtk_spi_mask_field_write(MIPI_RX_PHY_BASE +
		CORE_DIG_DLANE_0_RW_HS_RX_7 * 4,
		CORE_DIG_DLANE_0_RW_HS_RX_7_HS_RX_7_DESKEW_AUTO_ALGO_SEL_REG_MASK,
		1);

	mtk_spi_mask_field_write(MIPI_RX_PHY_BASE +
		CORE_DIG_DLANE_1_RW_HS_RX_7 * 4,
		CORE_DIG_DLANE_1_RW_HS_RX_7_HS_RX_7_DESKEW_AUTO_ALGO_SEL_REG_MASK,
		1);

	mtk_spi_mask_field_write(MIPI_RX_PHY_BASE +
		CORE_DIG_DLANE_2_RW_HS_RX_7 * 4,
		CORE_DIG_DLANE_2_RW_HS_RX_7_HS_RX_7_DESKEW_AUTO_ALGO_SEL_REG_MASK,
		1);

	mtk_spi_mask_field_write(MIPI_RX_PHY_BASE +
		CORE_DIG_DLANE_3_RW_HS_RX_7 * 4,
		CORE_DIG_DLANE_3_RW_HS_RX_7_HS_RX_7_DESKEW_AUTO_ALGO_SEL_REG_MASK,
		1);

	mtk_spi_mask_field_write(MIPI_RX_PHY_BASE +
		CORE_DIG_DLANE_0_RW_HS_RX_3 * 4,
		CORE_DIG_DLANE_0_RW_HS_RX_3_HS_RX_3_FJUMP_DESKEW_REG_MASK,
		fjump_deskew_reg);

	mtk_spi_mask_field_write(MIPI_RX_PHY_BASE +
		CORE_DIG_DLANE_1_RW_HS_RX_3 * 4,
		CORE_DIG_DLANE_1_RW_HS_RX_3_HS_RX_3_FJUMP_DESKEW_REG_MASK,
		fjump_deskew_reg);

	mtk_spi_mask_field_write(MIPI_RX_PHY_BASE +
		CORE_DIG_DLANE_2_RW_HS_RX_3 * 4,
		CORE_DIG_DLANE_2_RW_HS_RX_3_HS_RX_3_FJUMP_DESKEW_REG_MASK,
		fjump_deskew_reg);

	mtk_spi_mask_field_write(MIPI_RX_PHY_BASE +
		CORE_DIG_DLANE_3_RW_HS_RX_3 * 4,
		CORE_DIG_DLANE_3_RW_HS_RX_3_HS_RX_3_FJUMP_DESKEW_REG_MASK,
		fjump_deskew_reg);

	mtk_spi_mask_field_write(MIPI_RX_PHY_BASE +
		CORE_DIG_DLANE_0_RW_HS_RX_6 * 4,
		CORE_DIG_DLANE_0_RW_HS_RX_6_HS_RX_6_MIN_EYE_OPENING_DESKEW_REG_MASK,
		eye_open_deskew_reg);

	mtk_spi_mask_field_write(MIPI_RX_PHY_BASE +
		CORE_DIG_DLANE_1_RW_HS_RX_6 * 4,
		CORE_DIG_DLANE_1_RW_HS_RX_6_HS_RX_6_MIN_EYE_OPENING_DESKEW_REG_MASK,
		eye_open_deskew_reg);

	mtk_spi_mask_field_write(MIPI_RX_PHY_BASE +
		CORE_DIG_DLANE_2_RW_HS_RX_6 * 4,
		CORE_DIG_DLANE_2_RW_HS_RX_6_HS_RX_6_MIN_EYE_OPENING_DESKEW_REG_MASK,
		eye_open_deskew_reg);

	mtk_spi_mask_field_write(MIPI_RX_PHY_BASE +
		CORE_DIG_DLANE_3_RW_HS_RX_6 * 4,
		CORE_DIG_DLANE_3_RW_HS_RX_6_HS_RX_6_MIN_EYE_OPENING_DESKEW_REG_MASK,
		eye_open_deskew_reg);

	mtk_spi_write(MIPI_RX_PHY_BASE +
		CORE_DIG_COMMON_RW_DESKEW_FINE_MEM * 4,
		0x0404);

	mtk_spi_write(MIPI_RX_PHY_BASE +
		CORE_DIG_COMMON_RW_DESKEW_FINE_MEM * 4,
		0x040C);

	mtk_spi_write(MIPI_RX_PHY_BASE +
		CORE_DIG_COMMON_RW_DESKEW_FINE_MEM * 4, 0x0414);

	mtk_spi_write(MIPI_RX_PHY_BASE +
		CORE_DIG_COMMON_RW_DESKEW_FINE_MEM * 4, 0x041C);

	mtk_spi_write(MIPI_RX_PHY_BASE +
		CORE_DIG_COMMON_RW_DESKEW_FINE_MEM * 4, 0x0423);

	mtk_spi_write(MIPI_RX_PHY_BASE +
		CORE_DIG_COMMON_RW_DESKEW_FINE_MEM * 4, 0x0429);

	mtk_spi_write(MIPI_RX_PHY_BASE +
		CORE_DIG_COMMON_RW_DESKEW_FINE_MEM * 4, 0x0430);

	mtk_spi_write(MIPI_RX_PHY_BASE +
		CORE_DIG_COMMON_RW_DESKEW_FINE_MEM * 4, 0x043A);

	mtk_spi_write(MIPI_RX_PHY_BASE +
		CORE_DIG_COMMON_RW_DESKEW_FINE_MEM * 4, 0x0445);

	mtk_spi_write(MIPI_RX_PHY_BASE +
		CORE_DIG_COMMON_RW_DESKEW_FINE_MEM * 4, 0x044A);

	mtk_spi_write(MIPI_RX_PHY_BASE +
		CORE_DIG_COMMON_RW_DESKEW_FINE_MEM * 4, 0x0450);

	mtk_spi_write(MIPI_RX_PHY_BASE +
		CORE_DIG_COMMON_RW_DESKEW_FINE_MEM * 4, 0x045A);

	mtk_spi_write(MIPI_RX_PHY_BASE +
		CORE_DIG_COMMON_RW_DESKEW_FINE_MEM * 4, 0x0465);

	mtk_spi_write(MIPI_RX_PHY_BASE +
		CORE_DIG_COMMON_RW_DESKEW_FINE_MEM * 4, 0x0469);

	mtk_spi_write(MIPI_RX_PHY_BASE +
		CORE_DIG_COMMON_RW_DESKEW_FINE_MEM * 4, 0x0472);

	mtk_spi_write(MIPI_RX_PHY_BASE +
		CORE_DIG_COMMON_RW_DESKEW_FINE_MEM * 4, 0x047A);

	mtk_spi_write(MIPI_RX_PHY_BASE +
		CORE_DIG_COMMON_RW_DESKEW_FINE_MEM * 4, 0x0485);

	mtk_spi_write(MIPI_RX_PHY_BASE +
		CORE_DIG_COMMON_RW_DESKEW_FINE_MEM * 4, 0x0489);

	mtk_spi_write(MIPI_RX_PHY_BASE +
		CORE_DIG_COMMON_RW_DESKEW_FINE_MEM * 4, 0x0490);

	mtk_spi_write(MIPI_RX_PHY_BASE +
		CORE_DIG_COMMON_RW_DESKEW_FINE_MEM * 4, 0x049A);

	mtk_spi_write(MIPI_RX_PHY_BASE +
		CORE_DIG_COMMON_RW_DESKEW_FINE_MEM * 4, 0x04A4);

	mtk_spi_write(MIPI_RX_PHY_BASE +
		CORE_DIG_COMMON_RW_DESKEW_FINE_MEM * 4, 0x04AC);

	mtk_spi_write(MIPI_RX_PHY_BASE +
		CORE_DIG_COMMON_RW_DESKEW_FINE_MEM * 4, 0x04B4);

	mtk_spi_write(MIPI_RX_PHY_BASE +
		CORE_DIG_COMMON_RW_DESKEW_FINE_MEM * 4, 0x04BC);

	mtk_spi_write(MIPI_RX_PHY_BASE +
		CORE_DIG_COMMON_RW_DESKEW_FINE_MEM * 4, 0x04C4);

	mtk_spi_write(MIPI_RX_PHY_BASE +
		CORE_DIG_COMMON_RW_DESKEW_FINE_MEM * 4, 0x04CC);

	mtk_spi_write(MIPI_RX_PHY_BASE +
		CORE_DIG_COMMON_RW_DESKEW_FINE_MEM * 4, 0x04D4);

	mtk_spi_write(MIPI_RX_PHY_BASE +
		CORE_DIG_COMMON_RW_DESKEW_FINE_MEM * 4, 0x04DC);

	mtk_spi_write(MIPI_RX_PHY_BASE +
		CORE_DIG_COMMON_RW_DESKEW_FINE_MEM * 4, 0x04E4);

	mtk_spi_write(MIPI_RX_PHY_BASE +
		CORE_DIG_COMMON_RW_DESKEW_FINE_MEM * 4, 0x04EC);

	mtk_spi_write(MIPI_RX_PHY_BASE +
		CORE_DIG_COMMON_RW_DESKEW_FINE_MEM * 4, 0x04F4);

	mtk_spi_write(MIPI_RX_PHY_BASE +
		CORE_DIG_COMMON_RW_DESKEW_FINE_MEM * 4, 0x04FC);

	mtk_spi_write(MIPI_RX_PHY_BASE +
		CORE_DIG_COMMON_RW_DESKEW_FINE_MEM * 4, 0x0504);

	mtk_spi_write(MIPI_RX_PHY_BASE +
		CORE_DIG_COMMON_RW_DESKEW_FINE_MEM * 4, 0x050C);

	mtk_spi_write(MIPI_RX_PHY_BASE +
		CORE_DIG_COMMON_RW_DESKEW_FINE_MEM * 4, 0x0514);

	mtk_spi_write(MIPI_RX_PHY_BASE +
		CORE_DIG_COMMON_RW_DESKEW_FINE_MEM * 4, 0x051C);

	mtk_spi_write(MIPI_RX_PHY_BASE +
		CORE_DIG_COMMON_RW_DESKEW_FINE_MEM * 4, 0x0523);

	mtk_spi_write(MIPI_RX_PHY_BASE +
		CORE_DIG_COMMON_RW_DESKEW_FINE_MEM * 4, 0x0529);

	mtk_spi_write(MIPI_RX_PHY_BASE +
		CORE_DIG_COMMON_RW_DESKEW_FINE_MEM * 4, 0x0530);

	mtk_spi_write(MIPI_RX_PHY_BASE +
		CORE_DIG_COMMON_RW_DESKEW_FINE_MEM * 4, 0x053A);

	mtk_spi_write(MIPI_RX_PHY_BASE +
		CORE_DIG_COMMON_RW_DESKEW_FINE_MEM * 4, 0x0545);

	mtk_spi_write(MIPI_RX_PHY_BASE +
		CORE_DIG_COMMON_RW_DESKEW_FINE_MEM * 4, 0x054A);

	mtk_spi_write(MIPI_RX_PHY_BASE +
		CORE_DIG_COMMON_RW_DESKEW_FINE_MEM * 4, 0x0550);

	mtk_spi_write(MIPI_RX_PHY_BASE +
		CORE_DIG_COMMON_RW_DESKEW_FINE_MEM * 4, 0x055A);

	mtk_spi_write(MIPI_RX_PHY_BASE +
		CORE_DIG_COMMON_RW_DESKEW_FINE_MEM * 4, 0x0565);

	mtk_spi_write(MIPI_RX_PHY_BASE +
		CORE_DIG_COMMON_RW_DESKEW_FINE_MEM * 4, 0x0569);

	mtk_spi_write(MIPI_RX_PHY_BASE +
		CORE_DIG_COMMON_RW_DESKEW_FINE_MEM * 4, 0x0572);

	mtk_spi_write(MIPI_RX_PHY_BASE +
		CORE_DIG_COMMON_RW_DESKEW_FINE_MEM * 4, 0x057A);

	mtk_spi_write(MIPI_RX_PHY_BASE +
		CORE_DIG_COMMON_RW_DESKEW_FINE_MEM * 4, 0x0585);

	mtk_spi_write(MIPI_RX_PHY_BASE +
		CORE_DIG_COMMON_RW_DESKEW_FINE_MEM * 4, 0x0589);

	mtk_spi_write(MIPI_RX_PHY_BASE +
		CORE_DIG_COMMON_RW_DESKEW_FINE_MEM * 4, 0x0590);

	mtk_spi_write(MIPI_RX_PHY_BASE +
		CORE_DIG_COMMON_RW_DESKEW_FINE_MEM * 4, 0x059A);

	mtk_spi_write(MIPI_RX_PHY_BASE +
		CORE_DIG_COMMON_RW_DESKEW_FINE_MEM * 4, 0x05A4);

	mtk_spi_write(MIPI_RX_PHY_BASE +
		CORE_DIG_COMMON_RW_DESKEW_FINE_MEM * 4, 0x05AC);

	mtk_spi_write(MIPI_RX_PHY_BASE +
		CORE_DIG_COMMON_RW_DESKEW_FINE_MEM * 4, 0x05B4);

	mtk_spi_write(MIPI_RX_PHY_BASE +
		CORE_DIG_COMMON_RW_DESKEW_FINE_MEM * 4, 0x05BC);

	mtk_spi_write(MIPI_RX_PHY_BASE +
		CORE_DIG_COMMON_RW_DESKEW_FINE_MEM * 4, 0x05C4);

	mtk_spi_write(MIPI_RX_PHY_BASE +
		CORE_DIG_COMMON_RW_DESKEW_FINE_MEM * 4, 0x05CC);

	mtk_spi_write(MIPI_RX_PHY_BASE +
		CORE_DIG_COMMON_RW_DESKEW_FINE_MEM * 4, 0x05D4);

	mtk_spi_write(MIPI_RX_PHY_BASE +
		CORE_DIG_COMMON_RW_DESKEW_FINE_MEM * 4, 0x05DC);

	mtk_spi_write(MIPI_RX_PHY_BASE +
		CORE_DIG_COMMON_RW_DESKEW_FINE_MEM * 4, 0x05E4);

	mtk_spi_write(MIPI_RX_PHY_BASE +
		CORE_DIG_COMMON_RW_DESKEW_FINE_MEM * 4, 0x05EC);

	mtk_spi_write(MIPI_RX_PHY_BASE +
		CORE_DIG_COMMON_RW_DESKEW_FINE_MEM * 4, 0x05F4);

	mtk_spi_write(MIPI_RX_PHY_BASE +
		CORE_DIG_COMMON_RW_DESKEW_FINE_MEM * 4, 0x05FC);

	mtk_spi_write(MIPI_RX_PHY_BASE +
		CORE_DIG_COMMON_RW_DESKEW_FINE_MEM * 4, 0x0604);

	mtk_spi_write(MIPI_RX_PHY_BASE +
		CORE_DIG_COMMON_RW_DESKEW_FINE_MEM * 4, 0x060C);

	mtk_spi_write(MIPI_RX_PHY_BASE +
		CORE_DIG_COMMON_RW_DESKEW_FINE_MEM * 4, 0x0614);

	mtk_spi_write(MIPI_RX_PHY_BASE +
		CORE_DIG_COMMON_RW_DESKEW_FINE_MEM * 4, 0x061C);

	mtk_spi_write(MIPI_RX_PHY_BASE +
		CORE_DIG_COMMON_RW_DESKEW_FINE_MEM * 4, 0x0623);

	mtk_spi_write(MIPI_RX_PHY_BASE +
		CORE_DIG_COMMON_RW_DESKEW_FINE_MEM * 4, 0x0629);

	mtk_spi_write(MIPI_RX_PHY_BASE +
		CORE_DIG_COMMON_RW_DESKEW_FINE_MEM * 4, 0x0632);

	mtk_spi_write(MIPI_RX_PHY_BASE +
		CORE_DIG_COMMON_RW_DESKEW_FINE_MEM * 4, 0x063A);

	mtk_spi_write(MIPI_RX_PHY_BASE +
		CORE_DIG_COMMON_RW_DESKEW_FINE_MEM * 4, 0x0645);

	mtk_spi_write(MIPI_RX_PHY_BASE +
		CORE_DIG_COMMON_RW_DESKEW_FINE_MEM * 4, 0x064A);

	mtk_spi_write(MIPI_RX_PHY_BASE +
		CORE_DIG_COMMON_RW_DESKEW_FINE_MEM * 4, 0x0650);

	mtk_spi_write(MIPI_RX_PHY_BASE +
		CORE_DIG_COMMON_RW_DESKEW_FINE_MEM * 4, 0x065A);

	mtk_spi_write(MIPI_RX_PHY_BASE +
		CORE_DIG_COMMON_RW_DESKEW_FINE_MEM * 4, 0x0665);

	mtk_spi_write(MIPI_RX_PHY_BASE +
		CORE_DIG_COMMON_RW_DESKEW_FINE_MEM * 4, 0x0669);

	mtk_spi_write(MIPI_RX_PHY_BASE +
		CORE_DIG_COMMON_RW_DESKEW_FINE_MEM * 4, 0x0672);

	mtk_spi_write(MIPI_RX_PHY_BASE +
		CORE_DIG_COMMON_RW_DESKEW_FINE_MEM * 4, 0x067A);

	mtk_spi_write(MIPI_RX_PHY_BASE +
		CORE_DIG_COMMON_RW_DESKEW_FINE_MEM * 4, 0x0685);

	mtk_spi_write(MIPI_RX_PHY_BASE +
		CORE_DIG_COMMON_RW_DESKEW_FINE_MEM * 4, 0x0689);

	mtk_spi_write(MIPI_RX_PHY_BASE +
		CORE_DIG_COMMON_RW_DESKEW_FINE_MEM * 4, 0x0690);

	mtk_spi_write(MIPI_RX_PHY_BASE +
		CORE_DIG_COMMON_RW_DESKEW_FINE_MEM * 4, 0x069A);

	mtk_spi_write(MIPI_RX_PHY_BASE +
		CORE_DIG_COMMON_RW_DESKEW_FINE_MEM * 4, 0x06A4);

	mtk_spi_write(MIPI_RX_PHY_BASE +
		CORE_DIG_COMMON_RW_DESKEW_FINE_MEM * 4, 0x06AC);

	mtk_spi_write(MIPI_RX_PHY_BASE +
		CORE_DIG_COMMON_RW_DESKEW_FINE_MEM * 4, 0x06B4);

	mtk_spi_write(MIPI_RX_PHY_BASE +
		CORE_DIG_COMMON_RW_DESKEW_FINE_MEM * 4, 0x06BC);

	mtk_spi_write(MIPI_RX_PHY_BASE +
		CORE_DIG_COMMON_RW_DESKEW_FINE_MEM * 4, 0x06C4);

	mtk_spi_write(MIPI_RX_PHY_BASE +
		CORE_DIG_COMMON_RW_DESKEW_FINE_MEM * 4, 0x06CC);

	mtk_spi_write(MIPI_RX_PHY_BASE +
		CORE_DIG_COMMON_RW_DESKEW_FINE_MEM * 4, 0x06D4);

	mtk_spi_write(MIPI_RX_PHY_BASE +
		CORE_DIG_COMMON_RW_DESKEW_FINE_MEM * 4, 0x06DC);

	mtk_spi_write(MIPI_RX_PHY_BASE +
		CORE_DIG_COMMON_RW_DESKEW_FINE_MEM * 4, 0x06E4);

	mtk_spi_write(MIPI_RX_PHY_BASE +
		CORE_DIG_COMMON_RW_DESKEW_FINE_MEM * 4, 0x06EC);

	mtk_spi_write(MIPI_RX_PHY_BASE +
		CORE_DIG_COMMON_RW_DESKEW_FINE_MEM * 4, 0x06F4);

	mtk_spi_write(MIPI_RX_PHY_BASE +
		CORE_DIG_COMMON_RW_DESKEW_FINE_MEM * 4, 0x06FC);

	mtk_spi_write(MIPI_RX_PHY_BASE +
		CORE_DIG_COMMON_RW_DESKEW_FINE_MEM * 4, 0x0704);

	mtk_spi_write(MIPI_RX_PHY_BASE +
		CORE_DIG_COMMON_RW_DESKEW_FINE_MEM * 4, 0x070C);

	mtk_spi_write(MIPI_RX_PHY_BASE +
		CORE_DIG_COMMON_RW_DESKEW_FINE_MEM * 4, 0x0714);

	mtk_spi_write(MIPI_RX_PHY_BASE +
		CORE_DIG_COMMON_RW_DESKEW_FINE_MEM * 4, 0x071C);

	mtk_spi_write(MIPI_RX_PHY_BASE +
		CORE_DIG_COMMON_RW_DESKEW_FINE_MEM * 4, 0x0723);

	mtk_spi_write(MIPI_RX_PHY_BASE +
		CORE_DIG_COMMON_RW_DESKEW_FINE_MEM * 4, 0x072A);

	mtk_spi_write(MIPI_RX_PHY_BASE +
		CORE_DIG_COMMON_RW_DESKEW_FINE_MEM * 4, 0x0730);

	mtk_spi_write(MIPI_RX_PHY_BASE +
		CORE_DIG_COMMON_RW_DESKEW_FINE_MEM * 4, 0x073A);

	mtk_spi_write(MIPI_RX_PHY_BASE +
		CORE_DIG_COMMON_RW_DESKEW_FINE_MEM * 4, 0x0745);

	mtk_spi_write(MIPI_RX_PHY_BASE +
		CORE_DIG_COMMON_RW_DESKEW_FINE_MEM * 4, 0x074A);

	mtk_spi_write(MIPI_RX_PHY_BASE +
		CORE_DIG_COMMON_RW_DESKEW_FINE_MEM * 4, 0x0750);

	mtk_spi_write(MIPI_RX_PHY_BASE +
		CORE_DIG_COMMON_RW_DESKEW_FINE_MEM * 4, 0x075A);

	mtk_spi_write(MIPI_RX_PHY_BASE +
		CORE_DIG_COMMON_RW_DESKEW_FINE_MEM * 4, 0x0765);

	mtk_spi_write(MIPI_RX_PHY_BASE +
		CORE_DIG_COMMON_RW_DESKEW_FINE_MEM * 4, 0x0769);

	mtk_spi_write(MIPI_RX_PHY_BASE +
		CORE_DIG_COMMON_RW_DESKEW_FINE_MEM * 4, 0x0772);

	mtk_spi_write(MIPI_RX_PHY_BASE +
		CORE_DIG_COMMON_RW_DESKEW_FINE_MEM * 4, 0x077A);

	mtk_spi_write(MIPI_RX_PHY_BASE +
		CORE_DIG_COMMON_RW_DESKEW_FINE_MEM * 4, 0x0785);

	mtk_spi_write(MIPI_RX_PHY_BASE +
		CORE_DIG_COMMON_RW_DESKEW_FINE_MEM * 4, 0x0789);

	mtk_spi_write(MIPI_RX_PHY_BASE +
		CORE_DIG_COMMON_RW_DESKEW_FINE_MEM * 4, 0x0790);

	mtk_spi_write(MIPI_RX_PHY_BASE +
		CORE_DIG_COMMON_RW_DESKEW_FINE_MEM * 4, 0x079A);

	mtk_spi_write(MIPI_RX_PHY_BASE +
		CORE_DIG_COMMON_RW_DESKEW_FINE_MEM * 4, 0x07A4);

	mtk_spi_write(MIPI_RX_PHY_BASE +
		CORE_DIG_COMMON_RW_DESKEW_FINE_MEM * 4, 0x07AC);

	mtk_spi_write(MIPI_RX_PHY_BASE +
		CORE_DIG_COMMON_RW_DESKEW_FINE_MEM * 4, 0x07B4);

	mtk_spi_write(MIPI_RX_PHY_BASE +
		CORE_DIG_COMMON_RW_DESKEW_FINE_MEM * 4, 0x07BC);

	mtk_spi_write(MIPI_RX_PHY_BASE +
		CORE_DIG_COMMON_RW_DESKEW_FINE_MEM * 4, 0x07C4);

	mtk_spi_write(MIPI_RX_PHY_BASE +
		CORE_DIG_COMMON_RW_DESKEW_FINE_MEM * 4, 0x07CC);

	mtk_spi_write(MIPI_RX_PHY_BASE +
		CORE_DIG_COMMON_RW_DESKEW_FINE_MEM * 4, 0x07D4);

	mtk_spi_write(MIPI_RX_PHY_BASE +
		CORE_DIG_COMMON_RW_DESKEW_FINE_MEM * 4, 0x07DC);

	mtk_spi_write(MIPI_RX_PHY_BASE +
		CORE_DIG_COMMON_RW_DESKEW_FINE_MEM * 4, 0x07E4);

	mtk_spi_write(MIPI_RX_PHY_BASE +
		CORE_DIG_COMMON_RW_DESKEW_FINE_MEM * 4, 0x07EC);

	mtk_spi_write(MIPI_RX_PHY_BASE +
		CORE_DIG_COMMON_RW_DESKEW_FINE_MEM * 4, 0x07F4);

	mtk_spi_write(MIPI_RX_PHY_BASE +
		CORE_DIG_COMMON_RW_DESKEW_FINE_MEM * 4, 0x07FC);

	DISPFUNCEND();
}

/* for debug use */
void output_debug_signal(void)
{
	/*Mutex thread 0 remove mod_sof[1]*/
	mtk_spi_write(0x00025030, 0x0000001D);

	/*Mutex thread 1 use IPI_VSYNC falling and mod_sof[1]*/
	mtk_spi_write(0x00025050, 0x00000002);
	mtk_spi_write(0x0002504C, 0x0000004a);
	mtk_spi_write(0x00025040, 0x00000001);

	/*DSI DBG Setting*/
	mtk_spi_write(0x00021170, 0x00001001);

	/*MM DBG Setting*/
	mtk_spi_write(0x00023300, 0x00000003);
	mtk_spi_write(0x000231a8, 0x00000021);

	/*DBGSYS Setting*/
	mtk_spi_write(0x000076d0, 0x00000001);

	/*GPIO Mode*/
	mtk_spi_write(0x00007310, 0x17711111);
	mtk_spi_write(0x00007300, 0x77701111);
}

int bdg_common_init(enum DISP_BDG_ENUM module,
			disp_ddp_path_config *config, void *cmdq)
{
	int ret = 0;
	LCM_DSI_PARAMS *tx_params;

	DISPSYS_REG = (struct BDG_DISPSYS_CONFIG_REGS *)
		DISPSYS_BDG_MMSYS_CONFIG_BASE;
	DSI2_REG = (struct BDG_MIPIDSI2_REGS *)
		DISPSYS_BDG_MIPIDSI2_DEVICE_BASE;
	SYS_REG = (struct BDG_SYSREG_CTRL_REGS *)
		DISPSYS_BDG_SYSREG_CTRL_BASE;
	RDMA_REG = (struct BDG_RDMA0_REGS *)
		DISPSYS_BDG_RDMA0_REGS_BASE;
	MUTEX_REG = (struct BDG_MUTEX_REGS *)
		DISPSYS_BDG_MUTEX_REGS_BASE;
	OCLA_REG = (struct BDG_OCLA_REGS *)
		DISPSYS_BDG_OCLA_BASE;
	TX_REG[0] = (struct BDG_TX_REGS *)
		DISPSYS_BDG_TX_DSI0_BASE;
	MIPI_TX_REG = (struct BDG_MIPI_TX_REGS *)
		DISPSYS_BDG_MIPI_TX_BASE;
	DSC_REG = (struct BDG_DISP_DSC_REGS *)
		DISPSYS_BDG_DISP_DSC_BASE;
	APMIXEDSYS = (struct BDG_APMIXEDSYS_REGS *)DISPSYS_BDG_APMIXEDSYS_BASE;
	TOPCKGEN = (struct BDG_TOPCKGEN_REGS *)DISPSYS_BDG_TOPCKGEN_BASE;
	GCE_REG = (struct BDG_GCE_REGS *)DISPSYS_BDG_GCE_BASE;
	EFUSE = (struct BDG_EFUSE_REGS *)DISPSYS_BDG_EFUSE_BASE;
	GPIO = (struct BDG_GPIO_REGS *)DISPSYS_BDG_GPIO_BASE;
	TX_CMDQ_REG[0] =
		(struct DSI_TX_CMDQ_REGS *)(DISPSYS_BDG_TX_DSI0_BASE + 0xd00);

	DISPFUNCSTART();
	bdg_tx_pull_6382_reset_pin();
	bdg_is_bdg_connected();
	spislv_init();
	spislv_switch_speed_hz(SPI_TX_LOW_SPEED_HZ,
			SPI_RX_LOW_SPEED_HZ);
	set_LDO_on(cmdq);
	set_mtcmos_on(cmdq);
	ana_macro_on(cmdq);
	set_subsys_on(cmdq);

	spislv_switch_speed_hz(SPI_TX_MAX_SPEED_HZ, SPI_RX_MAX_SPEED_HZ);

	/* Disable reset sequential de-glitch circuit */
	DSI_OUTREG32(cmdq, SYS_REG->RST_DG_CTRL, 0);
	/* Set GPIO to active IRQ */
	mtk_spi_mask_write((u32)(DISPSYS_BDG_GPIO_BASE +
		REG_GPIO_MODE1),
		GPIO_MODE1_FLD_GPIO12, 1);

	tx_params = &(config->dsi_config);

	/* clear CG */
	DSI_OUTREG32(cmdq, DISPSYS_REG->MMSYS_CG_CON0, 0);
	/* for release rx mac reset */
	DSI_OUTREG32(cmdq, SYS_REG->DISP_MISC0, 1);
	DSI_OUTREG32(cmdq, SYS_REG->DISP_MISC0, 0);
	/* switch DDI(cmd mode) or IPI(vdo mode) path */
	DSI_OUTREG32(cmdq, DISPSYS_REG->MIPI_RX_POST_CTRL, 0x00001100);
	if (tx_params->mode == CMD_MODE) {
		mtk_spi_mask_write(
			(u32)(DISPSYS_BDG_GPIO_BASE + REG_GPIO_MODE1),
			REG_GPIO_MODE1_FIELD_RSV_24, 0x31);
		mtk_spi_mask_write((u32)(DISPSYS_BDG_MMSYS_CONFIG_BASE +
			REG_MIPI_RX_POST_CTRL),
			MIPI_RX_POST_CTRL_FLD_MIPI_RX_MODE_SEL, 0);
	}

	else
		mtk_spi_mask_write((u32)(DISPSYS_BDG_MMSYS_CONFIG_BASE +
			REG_MIPI_RX_POST_CTRL),
			MIPI_RX_POST_CTRL_FLD_MIPI_RX_MODE_SEL, 1);

	/* RDMA setting */
	DSI_OUTREG32(cmdq, RDMA_REG->DISP_RDMA_SIZE_CON_0,
			tx_params->horizontal_active_pixel);
	DSI_OUTREG32(cmdq, RDMA_REG->DISP_RDMA_SIZE_CON_1,
			tx_params->vertical_active_line);

	DSI_OUTREG32(cmdq, RDMA_REG->DISP_RDMA_FIFO_CON, 0xf30 << 16);
	DSI_OUTREG32(cmdq, RDMA_REG->DISP_RDMA_STALL_CG_CON, 0);
	mtk_spi_mask_write((u32)(DISPSYS_BDG_RDMA0_REGS_BASE +
		REG_DISP_RDMA_SRAM_SEL),
		DISP_RDMA_SRAM_SEL_FLD_RDMA_SRAM_SEL, 0);
		mtk_spi_mask_write((u32)(DISPSYS_BDG_RDMA0_REGS_BASE +
	REG_DISP_RDMA_GLOBAL_CON),
		DISP_RDMA_GLOBAL_CON_FLD_ENGINE_EN, 1);


	/* MUTEX setting */
	DSI_OUTREG32(cmdq, MUTEX_REG->BDG_TX_DISP_MUTEX_INTEN, 0x1 << 16);
	DSI_OUTREG32(cmdq, MUTEX_REG->BDG_TX_DISP_MUTEX0_MOD0, 0x1f);
	if (tx_params->mode == CMD_MODE)
		DSI_OUTREG32(cmdq, MUTEX_REG->BDG_TX_DISP_MUTEX0_CTL, 0);
	else {
		DSI_OUTREG32(cmdq, MUTEX_REG->BDG_TX_DISP_MUTEX0_CTL,
				0x1 << 0 | 0x0 << 3 | 0x1 << 6 | 0x0 << 9);
		mtk_spi_mask_write((u32)(DISPSYS_BDG_MUTEX_REGS_BASE +
			REG_BDG_TX_DISP_MUTEX0_EN),
			BDG_TX_DISP_MUTEX0_EN_FLD_MUTEX0_EN, 1);

	}

	/* DSI-TX setting */
	bdg_tx_init(module, config, NULL);
	DSI_OUTREG32(cmdq, TX_REG[0]->DSI_RESYNC_CON, 0x50007);

	/* DSC setting */
	if (dsc_en) {
		bdg_dsc_init(module, cmdq, tx_params);
		mtk_spi_mask_write((u32)(DISPSYS_BDG_DISP_DSC_BASE +
			REG_DISP_DSC_CON),
			DISP_DSC_CON_FLD_DSC_PT_MEM_EN, 1);
		mtk_spi_mask_write((u32)(DISPSYS_BDG_DISP_DSC_BASE +
			REG_DISP_DSC_CON),
		DISP_DSC_CON_FLD_DSC_EN, 1);

	} else {
	#ifdef DSC_RELAY_MODE_EN
		bdg_dsc_init(module, cmdq, tx_params);
		mtk_spi_mask_write((u32)(DISPSYS_BDG_DISP_DSC_BASE +
			REG_DISP_DSC_CON),
			DISP_DSC_CON_FLD_DSC_PT_MEM_EN, 1);
		mtk_spi_mask_write((u32)(DISPSYS_BDG_DISP_DSC_BASE +
			REG_DISP_DSC_CON),
			DISP_DSC_CON_FLD_DSC_RELAY, 1);
		mtk_spi_mask_write((u32)(DISPSYS_BDG_DISP_DSC_BASE +
			REG_DISP_DSC_CON),
			DISP_DSC_CON_FLD_DSC_EN, 1);

	#else
		mtk_spi_mask_write((u32)(DISPSYS_BDG_DISP_DSC_BASE +
			REG_DISP_DSC_CON),
			DISP_DSC_CON_FLD_DSC_ALL_BYPASS, 1);
	#endif
	}

	calculate_datarate_cfgs_rx(ap_tx_data_rate);
	startup_seq_common(cmdq);

	startup_seq_dphy_specific(ap_tx_data_rate);

	/*TODO: Fix TARGET line*/
	DSI_OUTREG32(cmdq, TX_REG[0]->DSI_TARGET_NL, 0x7d0);
	DSI_OUTREG32(cmdq, DISPSYS_REG->TE_OUT_CON, 0x2e);

	DISPFUNCEND();

	return ret;
}
int bdg_common_deinit(enum DISP_BDG_ENUM module, void *cmdq)
{
	int ret = 0;

	DISPFUNCSTART();

	spislv_switch_speed_hz(SPI_TX_LOW_SPEED_HZ, SPI_RX_LOW_SPEED_HZ);
	set_ana_mipi_dsi_off(cmdq);
	ana_macro_off(cmdq);
	set_mtcmos_off(cmdq);
	set_LDO_off(cmdq);
	clk_buf_disp_ctrl(false);
	mt6382_init = 0;

	DISPFUNCEND();

	return ret;
}
