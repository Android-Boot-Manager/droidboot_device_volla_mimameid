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

#define LOG_TAG "LCM"
#define DSC_ENABLE
#ifndef BUILD_LK
#include <linux/string.h>
#include <linux/kernel.h>
#endif

#include "lcm_drv.h"

#ifdef MTK_ROUND_CORNER_SUPPORT
#include "vdo_01.h"
#endif


#ifdef BUILD_LK
#include <platform/upmu_common.h>
#include <platform/mt_gpio.h>
#include <platform/mt_i2c.h>
#include <platform/mt_pmic.h>
#include <string.h>
#include <debug.h>
#ifndef MACH_FPGA
#include <lcm_pmic.h>
#endif
#elif defined(BUILD_UBOOT)
#include <asm/arch/mt_gpio.h>
#else
#include <mach/mt_pm_ldo.h>
#include <mach/mt_gpio.h>
#endif

#ifdef BUILD_LK
#define LCM_LOGI(string, args...) \
	dprintf(CRITICAL, "[LK/"LOG_TAG"]"string, ##args)
#define LCM_LOGD(string, args...) \
	dprintf(INFO, "[LK/"LOG_TAG"]"string, ##args)
#else
#define LCM_LOGI(fmt, args...) \
	pr_notice("[KERNEL/"LOG_TAG"]"fmt, ##args)
#define LCM_LOGD(fmt, args...) \
	pr_debug("[KERNEL/"LOG_TAG"]"fmt, ##args)
#endif

#define LCM_ID_NT36672C_JDI (0xd7)

static const unsigned int BL_MIN_LEVEL = 20;
static LCM_UTIL_FUNCS lcm_util;

#define SET_RESET_PIN(v)	(lcm_util.set_reset_pin((v)))
#define MDELAY(n)	   (lcm_util.mdelay(n))
#define UDELAY(n)	   (lcm_util.udelay(n))


/* ---------------------------------------------------------------------- */
/* Local Functions */
/* ---------------------------------------------------------------------- */

#define dsi_set_cmdq_V2(cmd, count, ppara, force_update) \
		lcm_util.dsi_set_cmdq_V2(cmd, count, ppara, force_update)
#define dsi_set_cmdq(pdata, queue_size, force_update) \
		lcm_util.dsi_set_cmdq(pdata, queue_size, force_update)
#define wrtie_cmd(cmd) lcm_util.dsi_write_cmd(cmd)
#define write_regs(addr, pdata, byte_nums) \
		lcm_util.dsi_write_regs(addr, pdata, byte_nums)
#define read_reg(cmd) \
	  lcm_util.dsi_dcs_read_lcm_reg(cmd)
#define read_reg_v2(cmd, buffer, buffer_size) \
		lcm_util.dsi_dcs_read_lcm_reg_v2(cmd, buffer, buffer_size)

#ifndef BUILD_LK
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/fs.h>
#include <linux/slab.h>
#include <linux/init.h>
#include <linux/list.h>
#include <linux/i2c.h>
#include <linux/irq.h>
/* #include <linux/jiffies.h> */
/* #include <linux/delay.h> */
#include <linux/uaccess.h>
#include <linux/interrupt.h>
#include <linux/io.h>
#include <linux/platform_device.h>
#endif

/* static unsigned char lcd_id_pins_value = 0xFF; */
static const unsigned char LCD_MODULE_ID = 0x01;
/* ---------------------------------------------------------------------- */
/* Local Constants */
/* ---------------------------------------------------------------------- */
#define LCM_DSI_CMD_MODE	0
#define FRAME_WIDTH	(1080)
#define FRAME_HEIGHT	(2400)

#ifndef MACH_FPGA
#define GPIO_65132_EN GPIO_LCD_BIAS_ENP_PIN
#endif

#define REGFLAG_DELAY	   0xFFFC
#define REGFLAG_UDELAY  0xFFFB
#define REGFLAG_END_OF_TABLE	0xFFFD
#define REGFLAG_RESET_LOW   0xFFFE
#define REGFLAG_RESET_HIGH  0xFFFF

#ifndef TRUE
#define TRUE 1
#endif

#ifndef FALSE
#define FALSE 0
#endif

struct LCM_setting_table {
	unsigned int cmd;
	unsigned char count;
	unsigned char para_list[500];
};

static struct LCM_setting_table lcm_suspend_setting[] = {
	{0x28, 0, {} },
	{0x10, 0, {} },
	{REGFLAG_DELAY, 120, {} },
	{0x4F, 1, {0x01} },
	{REGFLAG_DELAY, 120, {} }
};

static struct LCM_setting_table init_setting[] = {
		{0XFF, 1, {0X10} },
		/*REGR 0XFE, 1, {0X10} },*/
		{0XFB, 1, {0X01} },
		{0XB0, 1, {0X00} },
		{0xC0, 1, {0x03} },
		{0xC1, 16, {0x89, 0x28, 0x00, 0x08, 0x00,
			0xAA, 0x02, 0x0E, 0x00, 0x2B, 0x00,
			0x07, 0x0D, 0xB7, 0x0C, 0xB7} },
		{0xC2, 2, {0x1B, 0xA0} },

		{0XFF, 1, {0X20} },
		/*REGR 0XFE, 1, {0X20} },*/
		{0XFB, 1, {0X01} },
		{0X01, 1, {0X66} },
		{0X06, 1, {0X50} },
		{0X07, 1, {0X28} },
		{0X0E, 1, {0X00} },
		{0X17, 1, {0X66} },
		{0X1B, 1, {0X01} },
		{0X5C, 1, {0X90} },
		{0X5E, 1, {0XA0} },
		{0X69, 1, {0XD0} },

		{0X19, 1, {0X55} },
		{0X32, 1, {0X3D} },
		{0X69, 1, {0XAA} },
		{0X95, 1, {0XD1} },
		{0X96, 1, {0XD1} },
		{0XF2, 1, {0X66} },
		{0XF3, 1, {0X44} },
		{0XF4, 1, {0X66} },
		{0XF3, 1, {0X44} },
		{0XF6, 1, {0X66} },
		{0XF3, 1, {0X44} },
		{0XF8, 1, {0X66} },
		{0XF3, 1, {0X44} },

		{0XFF, 1, {0X21} },
		/*REGR 0XFE, 1, {0X10} },*/
		{0XFB, 1, {0X01} },

		{0XFF, 1, {0X24} },
		/*REGR 0XFE, 1, {0X24} },*/
		{0XFB, 1, {0X01} },
		{0X00, 1, {0X1C} },
		{0X01, 1, {0X01} },
		{0X04, 1, {0X2C} },
		{0X05, 1, {0X2D} },
		{0X06, 1, {0X2E} },
		{0X07, 1, {0X2F} },
		{0X08, 1, {0X30} },
		{0X09, 1, {0X0F} },
		{0X0A, 1, {0X11} },
		{0X0C, 1, {0X14} },
		{0X0D, 1, {0X16} },
		{0X0E, 1, {0X18} },
		{0X0F, 1, {0X13} },
		{0X10, 1, {0X15} },
		{0X11, 1, {0X17} },
		{0X13, 1, {0X10} },
		{0X14, 1, {0X22} },
		{0X15, 1, {0X22} },
		{0X18, 1, {0X1C} },
		{0X19, 1, {0X01} },
		{0X1C, 1, {0X2C} },
		{0X1D, 1, {0X2D} },
		{0X1E, 1, {0X2E} },
		{0X1F, 1, {0X2F} },
		{0X20, 1, {0X30} },
		{0X21, 1, {0X0F} },
		{0X22, 1, {0X11} },
		{0X24, 1, {0X14} },
		{0X25, 1, {0X16} },
		{0X26, 1, {0X18} },
		{0X27, 1, {0X13} },
		{0X28, 1, {0X15} },
		{0X29, 1, {0X17} },
		{0X2B, 1, {0X10} },
		{0X2D, 1, {0X22} },
		{0X2F, 1, {0X22} },
		{0X32, 1, {0X44} },
		{0X33, 1, {0X00} },
		{0X34, 1, {0X00} },
		{0X35, 1, {0X01} },
		{0X36, 1, {0X3F} },
		{0X37, 1, {0X00} },
		{0X38, 1, {0X00} },
		{0X4D, 1, {0X02} },
		{0X4E, 1, {0X3A} },
		{0X4F, 1, {0X3A} },
		{0X53, 1, {0X3A} },
		{0X7A, 1, {0X83} },
		{0X7B, 1, {0X90} },
		{0X7D, 1, {0X03} },
		{0X80, 1, {0X03} },
		{0X81, 1, {0X03} },
		{0X82, 1, {0X13} },
		{0X84, 1, {0X31} },
		{0X85, 1, {0X00} },
		{0X86, 1, {0X00} },
		{0X87, 1, {0X00} },
		{0X90, 1, {0X13} },
		{0X92, 1, {0X31} },
		{0X93, 1, {0X00} },
		{0X94, 1, {0X00} },
		{0X95, 1, {0X00} },
		{0X9C, 1, {0XF4} },
		{0X9D, 1, {0X01} },
		{0XA0, 1, {0X10} },
		{0XA2, 1, {0X10} },
		{0XA3, 1, {0X03} },
		{0XA4, 1, {0X03} },
		{0XA5, 1, {0X03} },
		{0XC4, 1, {0X80} },
		{0XC6, 1, {0XC0} },
		{0XC9, 1, {0X00} },
		{0XD1, 1, {0X34} },
		{0XD9, 1, {0X80} },
		{0XE9, 1, {0X03} },

		{0XFF, 1, {0X25} },
		/*REGR 0XFE, 1, {0X25} },*/
		{0XFB, 1, {0X01} },
		{0X0F, 1, {0X1B} },
		{0X18, 1, {0X21} },
		{0X19, 1, {0XE4} },
		{0X21, 1, {0X40} },
		{0X68, 1, {0X58} },
		{0X69, 1, {0X10} },
		{0X6B, 1, {0X00} },
		{0X6C, 1, {0X1D} },
		{0X71, 1, {0X1D} },
		{0X77, 1, {0X72} },
		{0X7F, 1, {0X00} },
		{0X81, 1, {0X00} },
		{0X84, 1, {0X6D} },
		{0X86, 1, {0X2D} },
		{0X8D, 1, {0X00} },
		{0X8E, 1, {0X14} },
		{0X8F, 1, {0X04} },
		{0XC0, 1, {0X03} },
		{0XC1, 1, {0X19} },
		{0XC3, 1, {0X03} },
		{0XC4, 1, {0X11} },
		{0XC6, 1, {0X00} },
		{0XEF, 1, {0X00} },
		{0XF1, 1, {0X04} },

		{0XFF, 1, {0X26} },
		/*REGR 0XFE, 1, {0X26} },*/
		{0XFB, 1, {0X01} },
		{0X00, 1, {0X10} },
		{0X01, 1, {0XEB} },
		{0X03, 1, {0X01} },
		{0X04, 1, {0X9A} },
		{0X06, 1, {0X11} },
		{0X08, 1, {0X96} },
		{0X14, 1, {0X02} },
		{0X15, 1, {0X01} },
		{0X74, 1, {0XAF} },
		{0X81, 1, {0X10} },
		{0X83, 1, {0X03} },
		{0X84, 1, {0X02} },
		{0X85, 1, {0X01} },
		{0X86, 1, {0X02} },
		{0X87, 1, {0X01} },
		{0X88, 1, {0X05} },
		{0X8A, 1, {0X1A} },
		{0X8B, 1, {0X11} },
		{0X8C, 1, {0X24} },
		{0X8E, 1, {0X42} },
		{0X8F, 1, {0X11} },
		{0X90, 1, {0X11} },
		{0X91, 1, {0X11} },
		{0X9A, 1, {0X80} },
		{0X9B, 1, {0X04} },
		{0X9C, 1, {0X00} },
		{0X9D, 1, {0X00} },
		{0X9E, 1, {0X00} },

		{0XFF, 1, {0X27} },
		/*REGR 0XFE, 1, {0X27} },*/
		{0XFB, 1, {0X01} },
		{0X01, 1, {0X60} },
		{0X20, 1, {0X81} },
		{0X21, 1, {0X71} },
		{0X25, 1, {0X81} },
		{0X26, 1, {0X99} },
		{0X6E, 1, {0X00} },
		{0X6F, 1, {0X00} },
		{0X70, 1, {0X00} },
		{0X71, 1, {0X00} },
		{0X72, 1, {0X00} },
		{0X75, 1, {0X04} },
		{0X76, 1, {0X00} },
		{0X77, 1, {0X00} },
		{0X7D, 1, {0X09} },
		{0X7E, 1, {0X63} },
		{0X80, 1, {0X24} },
		{0X82, 1, {0X09} },
		{0X83, 1, {0X63} },
		{0XE3, 1, {0X01} },
		{0XE4, 1, {0XEC} },
		{0XE5, 1, {0X00} },
		{0XE6, 1, {0X7B} },
		{0XE9, 1, {0X02} },
		{0XEA, 1, {0X22} },
		{0XEB, 1, {0X00} },
		{0XEC, 1, {0X7B} },

		{0XFF, 1, {0X2A} },
		/*REGR 0XFE, 1, {0X10} },*/
		{0XFB, 1, {0X01} },
		{0X00, 1, {0X91} },
		{0X03, 1, {0X20} },
		{0X07, 1, {0X64} },
		{0X0A, 1, {0X60} },
		{0X0C, 1, {0X04} },
		{0X0D, 1, {0X40} },
		{0X0F, 1, {0X01} },
		{0X11, 1, {0XE0} },
		{0X15, 1, {0X0E} },
		{0X16, 1, {0XB6} },
		{0X19, 1, {0X0E} },
		{0X1A, 1, {0X8A} },
		{0X1F, 1, {0X40} },
		{0X28, 1, {0XFD} },
		{0X29, 1, {0X1F} },
		{0X2A, 1, {0XFF} },
		{0X2D, 1, {0X0A} },
		{0X30, 1, {0X4F} },
		{0X31, 1, {0XE7} },
		{0X33, 1, {0X73} },
		{0X34, 1, {0XFF} },
		{0X35, 1, {0X3B} },
		{0X36, 1, {0XE6} },
		{0X37, 1, {0XF9} },
		{0X38, 1, {0X40} },
		{0X39, 1, {0XE1} },
		{0X3A, 1, {0X4F} },
		{0X45, 1, {0X06} },
		{0X46, 1, {0X40} },
		{0X47, 1, {0X02} },
		{0X48, 1, {0X01} },
		{0X4A, 1, {0X56} },
		{0X4E, 1, {0X0E} },
		{0X4F, 1, {0XB6} },
		{0X52, 1, {0X0E} },
		{0X53, 1, {0X8A} },
		{0X57, 1, {0X55} },
		{0X58, 1, {0X55} },
		{0X59, 1, {0X55} },
		{0X60, 1, {0X80} },
		{0X61, 1, {0XFD} },
		{0X62, 1, {0X0D} },
		{0X63, 1, {0XC2} },
		{0X64, 1, {0X06} },
		{0X65, 1, {0X08} },
		{0X66, 1, {0X01} },
		{0X67, 1, {0X48} },
		{0X68, 1, {0X17} },
		{0X6A, 1, {0X8D} },
		{0X6B, 1, {0XFF} },
		{0X6C, 1, {0X2D} },
		{0X6D, 1, {0X8C} },
		{0X6E, 1, {0XFA} },
		{0X6F, 1, {0X31} },
		{0X70, 1, {0X88} },
		{0X71, 1, {0X48} },
		{0X7A, 1, {0X13} },
		{0X7B, 1, {0X90} },
		{0X7F, 1, {0X75} },
		{0X83, 1, {0X07} },
		{0X84, 1, {0XF9} },
		{0X87, 1, {0X07} },
		{0X88, 1, {0X77} },

		{0XFF, 1, {0X2C} },
		/*REGR 0XFE, 1, {0X10} },*/
		{0XFB, 1, {0X01} },
		{0X03, 1, {0X17} },
		{0X04, 1, {0X17} },
		{0X05, 1, {0X17} },
		{0X0D, 1, {0X01} },
		{0X0E, 1, {0X54} },
		{0X17, 1, {0X4B} },
		{0X18, 1, {0X4B} },
		{0X19, 1, {0X4B} },
		{0X2D, 1, {0XAF} },

		{0X2F, 1, {0X10} },
		{0X30, 1, {0XEB} },
		{0X32, 1, {0X01} },
		{0X33, 1, {0X9A} },
		{0X35, 1, {0X16} },
		{0X37, 1, {0X96} },
		{0X4D, 1, {0X17} },
		{0X4E, 1, {0X04} },
		{0X4F, 1, {0X04} },
		{0X61, 1, {0X04} },
		{0X62, 1, {0X68} },
		{0X6B, 1, {0X71} },
		{0X6C, 1, {0X71} },
		{0X6D, 1, {0X71} },
		{0X81, 1, {0X11} },
		{0X82, 1, {0X84} },
		{0X84, 1, {0X01} },
		{0X85, 1, {0X9A} },
		{0X87, 1, {0X29} },
		{0X89, 1, {0X96} },
		{0X9D, 1, {0X1B} },
		{0X9E, 1, {0X08} },
		{0X9F, 1, {0X10} },

		{0XFF, 1, {0XE0} },
		/*REGR 0XFE, 1, {0XE0} },*/
		{0XFB, 1, {0X01} },
		{0X35, 1, {0X82} },
		{0X25, 1, {0X00} },
		{0X4E, 1, {0X00} },

		{0XFF, 1, {0XF0} },
		/*REGR 0XFE, 1, {0X10} },*/
		{0XFB, 1, {0X01} },
		{0X1C, 1, {0X01} },
		{0X33, 1, {0X01} },
		{0X5A, 1, {0X00} },

		{0XFF, 1, {0XD0} },
		/*REGR 0XFE, 1, {0XD0} },*/
		{0XFB, 1, {0X01} },
		{0X53, 1, {0X22} },
		{0X54, 1, {0X02} },
		{0XFF, 1, {0XC0} },
		{0XFB, 1, {0X01} },
		{0X9C, 1, {0X11} },
		{0X9D, 1, {0X11} },

		{0XFF, 1, {0X2B} },
		{0XFB, 1, {0X01} },
		{0XB7, 1, {0X13} },
		{0XB8, 1, {0X0F} },
		{0XC0, 1, {0X03} },

		{0XFF, 1, {0X10} },
		{0x11, 0, {} },
		{REGFLAG_DELAY, 120, {} },
		/* Display On*/
		{0x29, 0, {} },
};

/* #ifdef LCM_SET_DISPLAY_ON_DELAY */
/* to reduce init time, we move 120ms delay to lcm_set_display_on() !! */
static struct LCM_setting_table set_display_on[] = {
		{0x29, 0, {} },
};
/* #endif */

static struct LCM_setting_table bl_level[] = {
	{0x51, 1, {0xFF} },
	{REGFLAG_END_OF_TABLE, 0x00, {} }
};

static void push_table(struct LCM_setting_table *table, unsigned int count,
			   unsigned char force_update)
{
	unsigned int i;
	unsigned int cmd;

	for (i = 0; i < count; i++) {
		cmd = table[i].cmd;

		switch (cmd) {
		case REGFLAG_DELAY:
			if (table[i].count <= 10)
				MDELAY(table[i].count);
			else
				MDELAY(table[i].count);
			break;

		case REGFLAG_UDELAY:
			UDELAY(table[i].count);
			break;

		case REGFLAG_END_OF_TABLE:
			break;

		default:
			dsi_set_cmdq_V2(cmd, table[i].count,
				table[i].para_list, force_update);
			if (table[i].count > 1)
				UDELAY(100);
			break;
		}
	}
}

/* ---------------------------------------------------------------------- */
/* LCM Driver Implementations */
/* ---------------------------------------------------------------------- */

static void lcm_set_util_funcs(const LCM_UTIL_FUNCS *util)
{
	memcpy(&lcm_util, util, sizeof(LCM_UTIL_FUNCS));
}

static void lcm_get_params(LCM_PARAMS *params)
{
	memset(params, 0, sizeof(LCM_PARAMS));

	params->type = LCM_TYPE_DSI;

	params->width = FRAME_WIDTH;
	params->height = FRAME_HEIGHT;

#if (LCM_DSI_CMD_MODE)
	params->dsi.mode = CMD_MODE;
	params->dsi.switch_mode = SYNC_PULSE_VDO_MODE;
#else
	params->dsi.mode = SYNC_PULSE_VDO_MODE;
	params->dsi.switch_mode = CMD_MODE;
#endif
	params->dsi.switch_mode_enable = 0;

	/* DSI */
	/* Command mode setting */
	params->dsi.LANE_NUM = LCM_FOUR_LANE;
	/* The following defined the fomat for data coming from LCD engine. */
	params->dsi.data_format.color_order = LCM_COLOR_ORDER_RGB;
	params->dsi.data_format.trans_seq = LCM_DSI_TRANS_SEQ_MSB_FIRST;
	params->dsi.data_format.padding = LCM_DSI_PADDING_ON_LSB;
	params->dsi.data_format.format = LCM_DSI_FORMAT_RGB888;

	/* Highly depends on LCD driver capability. */
	params->dsi.packet_size = 256;
	/* video mode timing */

	params->dsi.PS = LCM_PACKED_PS_24BIT_RGB888;

	params->dsi.vertical_sync_active = 10;
	params->dsi.vertical_backporch = 10;
	params->dsi.vertical_frontporch = 46;
	/*params->dsi.vertical_frontporch_for_low_power = 750;*/
	params->dsi.vertical_active_line = FRAME_HEIGHT;

	params->dsi.horizontal_sync_active = 22;
	params->dsi.horizontal_backporch = 22;
	params->dsi.horizontal_frontporch = 165;
	params->dsi.horizontal_active_pixel = FRAME_WIDTH;
	params->dsi.ssc_disable = 1;
	params->dsi.dsc_enable = 0;
#ifdef DSC_ENABLE
	params->dsi.bdg_dsc_enable = 1;
/*with dsc*/
	params->dsi.PLL_CLOCK = 380;
	params->dsi.data_rate = 760;
#else
	params->dsi.bdg_dsc_enable = 0;
/*without dsc*/
	params->dsi.PLL_CLOCK = 500;
	params->dsi.data_rate = 1000;
#endif

	params->dsi.clk_lp_per_line_enable = 0;
	params->dsi.esd_check_enable = 0;
	params->dsi.customization_esd_check_enable = 0;
	params->dsi.lcm_esd_check_table[0].cmd = 0x0A;
	params->dsi.lcm_esd_check_table[0].count = 1;
	params->dsi.lcm_esd_check_table[0].para_list[0] = 0x9d;

	params->dsi.lane_swap_en = 0;
#ifdef MTK_RUNTIME_SWITCH_FPS_SUPPORT
	params->dsi.fps = 60;
#endif

#ifdef MTK_ROUND_CORNER_SUPPORT
	params->round_corner_params.round_corner_en = 1;
	params->round_corner_params.full_content = 0;
	params->round_corner_params.h = ROUND_CORNER_H_TOP;
	params->round_corner_params.h_bot = ROUND_CORNER_H_BOT;
	params->round_corner_params.tp_size = sizeof(top_rc_pattern);
	params->round_corner_params.lt_addr = (void *)top_rc_pattern;
#endif

}

static int i2c_w8(int bus, uint8_t flags, uint16_t address,
	uint8_t reg, uint8_t val)
{
	int ret;
	struct mt_i2c_t i2c;
	uint8_t data[4];

	memset(&i2c, 0x0, sizeof(i2c));
	i2c.id = bus;
	i2c.addr = address;
	i2c.mode = ST_MODE;
	i2c.speed = 100;
	i2c.dma_en = 0;
	data[0] = reg;
	data[1] = val;
	ret = i2c_write(&i2c, data, 2);
	return ret;
}

static void lcm_init_power(void)
{
		SET_RESET_PIN(0);
		MDELAY(100);
	if (lcm_util.set_gpio_lcd_enp_bias) {
		lcm_util.set_gpio_lcd_enp_bias(1);

		LCM_LOGI("[zsq] -----lcm_init_power------\n");

	} else
		LCM_LOGI("[zsq] set_gpio_lcd_enp_bias not defined\n");
}

static void lcm_suspend_power(void)
{

}

static void lcm_resume_power(void)
{

}

#ifdef LCM_SET_DISPLAY_ON_DELAY
static U32 lcm_init_tick;
static int is_display_on;
#endif

static void lcm_init(void)
{
	int ret = 0;

	SET_RESET_PIN(0);


#ifndef MACH_FPGA
#if 0
	/*config rt5081 register 0xB2[7:6]=0x3, that is set db_delay=4ms.*/
	ret = PMU_REG_MASK(0xB2, (0x3 << 6), (0x3 << 6));

	/* set AVDD 5.4v, (4v+28*0.05v) */
	/*ret = RT5081_write_byte(0xB3, (1 << 6) | 28);*/
	ret = PMU_REG_MASK(0xB3, 28, (0x3F << 0));
	if (ret < 0)
		LCM_LOGI("nt35695 tps6132 cmd=%0x--i2c write error\n",
			0xB3);
	else
		LCM_LOGI("nt35695 tps6132 cmd=%0x--i2c write success\n",
			0xB3);

	/* set AVEE */
	/*ret = RT5081_write_byte(0xB4, (1 << 6) | 28);*/
	ret = PMU_REG_MASK(0xB4, 28, (0x3F << 0));
	if (ret < 0)
		LCM_LOGI("nt35695 tps6132 cmd=%0x--i2c write error\n",
			0xB4);
	else
		LCM_LOGI("nt35695 tps6132 cmd=%0x--i2c write success\n",
			0xB4);

	/* enable AVDD & AVEE */
	/* 0x12--default value; bit3--Vneg; bit6--Vpos; */
	/*ret = RT5081_write_byte(0xB1, 0x12 | (1<<3) | (1<<6));*/
	ret = PMU_REG_MASK(0xB1, (1<<3) | (1<<6), (1<<3) | (1<<6));
	if (ret < 0)
		LCM_LOGI("nt35695 tps6132 cmd=%0x--i2c write error\n",
			0xB1);
	else
		LCM_LOGI("nt35695 tps6132 cmd=%0x--i2c write success\n",
			0xB1);

	MDELAY(15);
#endif
#endif

	MDELAY(15);
	SET_RESET_PIN(1);
	MDELAY(10);
	SET_RESET_PIN(0);
	MDELAY(10);

	SET_RESET_PIN(1);
	MDELAY(10);

	push_table(init_setting, sizeof(init_setting) /
		sizeof(struct LCM_setting_table), 1);

#ifdef LCM_SET_DISPLAY_ON_DELAY
	lcm_init_tick = gpt4_get_current_tick();
	is_display_on = 0;
#endif
}

#ifdef LCM_SET_DISPLAY_ON_DELAY
static int lcm_set_display_on(void)
{
	U32 timeout_tick, i = 0;

	if (is_display_on)
		return 0;

	/* we need to wait 120ms after lcm init to set display on */
	timeout_tick = gpt4_time2tick_ms(120);

	while (!gpt4_timeout_tick(lcm_init_tick, timeout_tick)) {
		i++;
		if (i % 1000 == 0) {
			LCM_LOGI("nt36672c %s error: i=%u,lcm_init_tick=%u,cur_tick=%u\n",
				 __func__, i, lcm_init_tick,
				 gpt4_get_current_tick());
		}
	}

	push_table(set_display_on, sizeof(set_display_on) /
		   sizeof(struct LCM_setting_table), 1);

	is_display_on = 1;
	return 0;
}
#endif

static void lcm_suspend(void)
{
	int ret;

	push_table(lcm_suspend_setting, sizeof(lcm_suspend_setting) /
		sizeof(struct LCM_setting_table), 1);
#ifndef MACH_FPGA
	/* enable AVDD & AVEE */
	/* 0x12--default value; bit3--Vneg; bit6--Vpos; */
	/*ret = RT5081_write_byte(0xB1, 0x12);*/
	ret = PMU_REG_MASK(0xB1, (0<<3) | (0<<6), (1<<3) | (1<<6));
	if (ret < 0)
		LCM_LOGI("nt35695 tps6132 cmd=%0x--i2c write error\n", 0xB1);
	else
		LCM_LOGI("nt35695 tps6132 cmd=%0x--i2c write success\n", 0xB1);

	MDELAY(5);

#endif
	SET_RESET_PIN(0);
	MDELAY(10);
}

static void lcm_resume(void)
{
	lcm_init();
}

#if LCM_DSI_CMD_MODE == 1
static void lcm_update(unsigned int x, unsigned int y,
	unsigned int width, unsigned int height)
{
	unsigned int x0 = x;
	unsigned int y0 = y;
	unsigned int x1 = x0 + width - 1;
	unsigned int y1 = y0 + height - 1;

	unsigned char x0_MSB = ((x0 >> 8) & 0xFF);
	unsigned char x0_LSB = (x0 & 0xFF);
	unsigned char x1_MSB = ((x1 >> 8) & 0xFF);
	unsigned char x1_LSB = (x1 & 0xFF);
	unsigned char y0_MSB = ((y0 >> 8) & 0xFF);
	unsigned char y0_LSB = (y0 & 0xFF);
	unsigned char y1_MSB = ((y1 >> 8) & 0xFF);
	unsigned char y1_LSB = (y1 & 0xFF);

	unsigned int data_array[16];

#ifdef LCM_SET_DISPLAY_ON_DELAY
	lcm_set_display_on();
#endif

	data_array[0] = 0x00053902;
	data_array[1] = (x1_MSB << 24) | (x0_LSB << 16) |
		(x0_MSB << 8) | 0x2a;
	data_array[2] = (x1_LSB);
	dsi_set_cmdq(data_array, 3, 1);

	data_array[0] = 0x00053902;
	data_array[1] = (y1_MSB << 24) | (y0_LSB << 16) |
		(y0_MSB << 8) | 0x2b;
	data_array[2] = (y1_LSB);
	dsi_set_cmdq(data_array, 3, 1);

	data_array[0] = 0x002c3909;
	dsi_set_cmdq(data_array, 1, 0);
}
#else /* not LCM_DSI_CMD_MODE */

static void lcm_update(unsigned int x, unsigned int y,
	unsigned int width, unsigned int height)
{
#ifdef LCM_SET_DISPLAY_ON_DELAY
	lcm_set_display_on();
#endif
}
#endif

static unsigned int lcm_compare_id(void)
{

	unsigned int id = 0;
	unsigned char buffer[2];
	unsigned int array[16];

	LCM_LOGI("%S: enter\n", __func__);

	SET_RESET_PIN(1);
	MDELAY(10);
	SET_RESET_PIN(0);
	MDELAY(10);
	SET_RESET_PIN(1);
	MDELAY(10);

	array[0] = 0x00013700;	/* read id return two byte,version and id */
	dsi_set_cmdq(array, 1, 1);

	read_reg_v2(0xDA, buffer, 1);
	id = buffer[0];	 /* we only need ID */

	LCM_LOGI("%s,nt3672C_id_jdi=0x%08x\n", __func__, id);

	if (id == LCM_ID_NT36672C_JDI)
		return 1;
	else
		return 0;

}


/* return TRUE: need recovery */
/* return FALSE: No need recovery */
static unsigned int lcm_esd_check(void)
{
#ifndef BUILD_LK
	char buffer[3];
	int array[4];

	array[0] = 0x00013700;
	dsi_set_cmdq(array, 1, 1);

	read_reg_v2(0x0A, buffer, 1);

	if (buffer[0] != 0x9C) {
		LCM_LOGI("[LCM ERROR] [0x0A]=0x%02x\n", buffer[0]);
		return TRUE;
	}
	LCM_LOGI("[LCM NORMAL] [0x0A]=0x%02x\n", buffer[0]);
	return FALSE;
#else
	return FALSE;
#endif

}

static unsigned int lcm_ata_check(unsigned char *buffer)
{
#ifndef BUILD_LK
	unsigned int ret = 0;
	unsigned int x0 = FRAME_WIDTH / 4;
	unsigned int x1 = FRAME_WIDTH * 3 / 4;

	unsigned char x0_MSB = ((x0 >> 8) & 0xFF);
	unsigned char x0_LSB = (x0 & 0xFF);
	unsigned char x1_MSB = ((x1 >> 8) & 0xFF);
	unsigned char x1_LSB = (x1 & 0xFF);

	unsigned int data_array[3];
	unsigned char read_buf[4];

	LCM_LOGI("ATA check size = 0x%x,0x%x,0x%x,0x%x\n",
		x0_MSB, x0_LSB, x1_MSB, x1_LSB);
	data_array[0] = 0x0005390A; /* HS packet */
	data_array[1] = (x1_MSB << 24) | (x0_LSB << 16) | (x0_MSB << 8) | 0x2a;
	data_array[2] = (x1_LSB);
	dsi_set_cmdq(data_array, 3, 1);

/* read id return two byte,version and id */
	data_array[0] = 0x00043700;
	dsi_set_cmdq(data_array, 1, 1);

	read_reg_v2(0x2A, read_buf, 4);

	if ((read_buf[0] == x0_MSB) && (read_buf[1] == x0_LSB)
			&& (read_buf[2] == x1_MSB) && (read_buf[3] == x1_LSB))
		ret = 1;
	else
		ret = 0;

	x0 = 0;
	x1 = FRAME_WIDTH - 1;

	x0_MSB = ((x0 >> 8) & 0xFF);
	x0_LSB = (x0 & 0xFF);
	x1_MSB = ((x1 >> 8) & 0xFF);
	x1_LSB = (x1 & 0xFF);

	data_array[0] = 0x0005390A; /* HS packet */
	data_array[1] = (x1_MSB << 24) | (x0_LSB << 16) | (x0_MSB << 8) | 0x2a;
	data_array[2] = (x1_LSB);
	dsi_set_cmdq(data_array, 3, 1);
	return ret;
#else
	return 0;
#endif
}

static void lcm_setbacklight_cmdq(void *handle, unsigned int level)
{

	LCM_LOGI("%s,nt36672c shenchao backlight: level = %d\n",
		__func__, level);

	bl_level[0].para_list[0] = level;

	push_table(bl_level, sizeof(bl_level) /
		sizeof(struct LCM_setting_table), 1);
}

static void lcm_setbacklight(unsigned int level)
{
	LCM_LOGI("%s,nt36672c backlight: level = %d\n",
		__func__, level);

	bl_level[0].para_list[0] = level;

	push_table(bl_level, sizeof(bl_level) /
		sizeof(struct LCM_setting_table), 1);
}

static void *lcm_switch_mode(int mode)
{
	return NULL;
}


LCM_DRIVER nt36672c_fhdp_dsi_vdo_90hz_shenchao_6382_lcm_drv = {
	.name = "nt36672c_fhdp_dsi_vdo_90hz_shenchao_6382_lcm_drv",
	.set_util_funcs = lcm_set_util_funcs,
	.get_params = lcm_get_params,
	.init = lcm_init,
	.suspend = lcm_suspend,
	.resume = lcm_resume,
	.compare_id = lcm_compare_id,
	.init_power = lcm_init_power,
	.resume_power = lcm_resume_power,
	.suspend_power = lcm_suspend_power,
	.esd_check = lcm_esd_check,
	.set_backlight = lcm_setbacklight,
	.ata_check = lcm_ata_check,
	.update = lcm_update,
};
