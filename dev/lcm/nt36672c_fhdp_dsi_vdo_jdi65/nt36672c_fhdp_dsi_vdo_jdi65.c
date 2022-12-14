/* Copyright Statement:
*
* This software/firmware and related documentation ("MediaTek Software") are
* protected under relevant copyright laws. The information contained herein
* is confidential and proprietary to MediaTek Inc. and/or its licensors.
* Without the prior written permission of MediaTek inc. and/or its licensors,
* any reproduction, modification, use or disclosure of MediaTek Software,
* and information contained herein, in whole or in part, shall be strictly prohibited.
*/
/* MediaTek Inc. (C) 2015. All rights reserved.
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

#ifndef BUILD_LK
#include <linux/string.h>
#include <linux/kernel.h>
#endif

#include "lcm_drv.h"


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
#define LCM_LOGI(string, args...)  dprintf(CRITICAL, "[LK/"LOG_TAG"]"string, ##args)
#define LCM_LOGD(string, args...)  dprintf(INFO, "[LK/"LOG_TAG"]"string, ##args)
#else
#define LCM_LOGI(fmt, args...)  pr_notice("[KERNEL/"LOG_TAG"]"fmt, ##args)
#define LCM_LOGD(fmt, args...)  pr_debug("[KERNEL/"LOG_TAG"]"fmt, ##args)
#endif

#define LCM_ID_NT36672C_JDI (0xd7) // TO DO

static const unsigned int BL_MIN_LEVEL = 20;
static LCM_UTIL_FUNCS lcm_util;

#define SET_RESET_PIN(v)    (lcm_util.set_reset_pin((v)))
#define MDELAY(n)       (lcm_util.mdelay(n))
#define UDELAY(n)       (lcm_util.udelay(n))


/* --------------------------------------------------------------------------- */
/* Local Functions */
/* --------------------------------------------------------------------------- */

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
/* --------------------------------------------------------------------------- */
/* Local Constants */
/* --------------------------------------------------------------------------- */
#define LCM_DSI_CMD_MODE                                    0
#define FRAME_WIDTH                                     (1080)
#define FRAME_HEIGHT                                    (2400)

#define REGFLAG_DELAY       0xFFFC
#define REGFLAG_UDELAY  0xFFFB
#define REGFLAG_END_OF_TABLE    0xFFFD
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
	unsigned char para_list[64];
};

static struct LCM_setting_table lcm_suspend_setting[] = {
	{0x28, 0, {} },
	{0x10, 0, {} },
	{REGFLAG_DELAY, 120, {} },
	{0x4F, 1, {0x01} },
	{REGFLAG_DELAY, 120, {} }
};

static struct LCM_setting_table init_setting[] = {
{0xFF,1,{0x10}},
{0xFB,1,{0x01}},
{0xB0,1,{0x00}},
{0xC0,1,{0x00}},
{0xC2,2,{0x1B,0xA0}},

{0xFF,1,{0x20}},
{0xFB,1,{0x01}},
{0x01,1,{0x66}},
{0x06,1,{0x40}},
{0x07,1,{0x38}},
{0x18,1,{0x66}},
{0x1B,1,{0x01}},
{0x2F,1,{0x83}},
{0x5C,1,{0x90}},
{0x5E,1,{0xAA}},
{0x69,1,{0x91}},
{0x95,1,{0xD1}},
{0x96,1,{0xD1}},
{0xF2,1,{0x65}},
{0xF3,1,{0x64}},
{0xF4,1,{0x65}},
{0xF5,1,{0x64}},
{0xF6,1,{0x65}},
{0xF7,1,{0x64}},
{0xF8,1,{0x65}},
{0xF9,1,{0x64}},

{0xFF,1,{0x24}},
{0xFB,1,{0x01}},
{0x01,1,{0x0F}},
{0x03,1,{0x0C}},
{0x05,1,{0x1D}},
{0x08,1,{0x2F}},
{0x09,1,{0x2E}},
{0x0A,1,{0x2D}},
{0x0B,1,{0x2C}},
{0x11,1,{0x17}},
{0x12,1,{0x13}},
{0x13,1,{0x15}},
{0x15,1,{0x14}},
{0x16,1,{0x16}},
{0x17,1,{0x18}},
{0x1B,1,{0x01}},
{0x1D,1,{0x1D}},
{0x20,1,{0x2F}},
{0x21,1,{0x2E}},
{0x22,1,{0x2D}},
{0x23,1,{0x2C}},
{0x29,1,{0x17}},
{0x2A,1,{0x13}},
{0x2B,1,{0x15}},
{0x2F,1,{0x14}},
{0x30,1,{0x16}},
{0x31,1,{0x18}},
{0x32,1,{0x04}},
{0x34,1,{0x10}},
{0x35,1,{0x1F}},
{0x36,1,{0x1F}},
{0x37,1,{0x20}},
{0x4D,1,{0x1B}},
{0x4E,1,{0x4B}},
{0x4F,1,{0x4B}},
{0x53,1,{0x4B}},
{0x71,1,{0x30}},
{0x79,1,{0x11}},
{0x7A,1,{0x82}},
{0x7B,1,{0x96}},
{0x7D,1,{0x04}},
{0x80,1,{0x04}},
{0x81,1,{0x04}},
{0x82,1,{0x13}},
{0x84,1,{0x31}},
{0x85,1,{0x00}},
{0x86,1,{0x00}},
{0x87,1,{0x00}},
{0x90,1,{0x13}},
{0x92,1,{0x31}},
{0x93,1,{0x00}},
{0x94,1,{0x00}},
{0x95,1,{0x00}},
{0x9C,1,{0xF4}},
{0x9D,1,{0x01}},
{0xA0,1,{0x16}},
{0xA2,1,{0x16}},
{0xA3,1,{0x02}},
{0xA4,1,{0x04}},
{0xA5,1,{0x04}},
{0xC6,1,{0xC0}},
{0xC9,1,{0x00}},
{0xD9,1,{0x80}},
{0xE9,1,{0x02}},

{0xFF,1,{0x25}},
{0xFB,1,{0x01}},
{0x0F,1,{0x1B}},
{0x18,1,{0x21}},
{0x19,1,{0xE4}},
{0x21,1,{0x40}},
{0x66,1,{0xD8}},
{0x68,1,{0x50}},
{0x69,1,{0x10}},
{0x6B,1,{0x00}},
{0x6D,1,{0x0D}},
{0x6E,1,{0x48}},
{0x72,1,{0x41}},
{0x73,1,{0x4A}},
{0x74,1,{0xD0}},
{0x77,1,{0x62}},
{0x79,1,{0x77}},
{0x7D,1,{0x40}},
{0x7E,1,{0x1D}},
{0x7F,1,{0x00}},
{0x80,1,{0x04}},
{0x84,1,{0x0D}},
{0xCF,1,{0x80}},
{0xD6,1,{0x80}},
{0xD7,1,{0x80}},
{0xEF,1,{0x20}},
{0xF0,1,{0x84}},

{0xFF,1,{0x26}},
{0xFB,1,{0x01}},
{0x15,1,{0x04}},
{0x81,1,{0x16}},
{0x83,1,{0x02}},
{0x84,1,{0x03}},
{0x85,1,{0x01}},
{0x86,1,{0x03}},
{0x87,1,{0x01}},
{0x88,1,{0x00}},
{0x8A,1,{0x1A}},
{0x8B,1,{0x11}},
{0x8C,1,{0x24}},
{0x8E,1,{0x42}},
{0x8F,1,{0x11}},
{0x90,1,{0x11}},
{0x91,1,{0x11}},
{0x9A,1,{0x80}},
{0x9B,1,{0x04}},
{0x9C,1,{0x00}},
{0x9D,1,{0x00}},
{0x9E,1,{0x00}},

{0xFF,1,{0x27}},
{0xFB,1,{0x01}},
{0x01,1,{0x60}},
{0x20,1,{0x81}},
{0x21,1,{0xE7}},
{0x25,1,{0x82}},
{0x26,1,{0x1F}},
{0x6E,1,{0x00}},
{0x6F,1,{0x00}},
{0x70,1,{0x00}},
{0x71,1,{0x00}},
{0x72,1,{0x00}},
{0x75,1,{0x00}},
{0x76,1,{0x00}},
{0x77,1,{0x00}},
{0x7D,1,{0x09}},
{0x7E,1,{0x5F}},
{0x80,1,{0x23}},
{0x82,1,{0x09}},
{0x83,1,{0x5F}},
{0x88,1,{0x01}},
{0x89,1,{0x10}},
{0xA5,1,{0x10}},
{0xA6,1,{0x23}},
{0xA7,1,{0x01}},
{0xB6,1,{0x40}},
{0xE3,1,{0x02}},
{0xE4,1,{0xDA}},
{0xE5,1,{0x01}},
{0xE6,1,{0x6D}},
{0xE9,1,{0x03}},
{0xEA,1,{0x2F}},
{0xEB,1,{0x01}},
{0xEC,1,{0x98}},

{0xFF,1,{0x2A}},
{0xFB,1,{0x01}},
{0x00,1,{0x91}},
{0x03,1,{0x20}},
{0x07,1,{0x52}},
{0x0A,1,{0x70}},
{0x0D,1,{0x40}},
{0x0E,1,{0x02}},
{0x11,1,{0xF0}},
{0x15,1,{0x0E}},
{0x16,1,{0xB6}},
{0x19,1,{0x0E}},
{0x1A,1,{0x8A}},
{0x1B,1,{0x14}},
{0x1D,1,{0x36}},
{0x1E,1,{0x4F}},
{0x1F,1,{0x51}},
{0x20,1,{0x4F}},
{0x28,1,{0xEC}},
{0x29,1,{0x0C}},
{0x2A,1,{0x05}},
{0x2D,1,{0x06}},
{0x2F,1,{0x02}},
{0x30,1,{0x4A}},
{0x33,1,{0x0E}},
{0x34,1,{0xEE}},
{0x35,1,{0x30}},
{0x36,1,{0x06}},
{0x37,1,{0xE9}},
{0x38,1,{0x34}},
{0x39,1,{0x02}},
{0x3A,1,{0x4A}},
{0x45,1,{0x0E}},
{0x46,1,{0x40}},
{0x47,1,{0x03}},
{0x4A,1,{0xA0}},
{0x4E,1,{0x0E}},
{0x4F,1,{0xB6}},
{0x52,1,{0x0E}},
{0x53,1,{0x8A}},
{0x54,1,{0x14}},
{0x56,1,{0x36}},
{0x57,1,{0x76}},
{0x58,1,{0x76}},
{0x59,1,{0x76}},
{0x60,1,{0x80}},
{0x61,1,{0xA0}},
{0x62,1,{0x04}},
{0x63,1,{0x33}},
{0x65,1,{0x04}},
{0x66,1,{0x01}},
{0x67,1,{0x05}},
{0x68,1,{0x40}},
{0x6A,1,{0x4C}},
{0x6B,1,{0xA2}},
{0x6C,1,{0x21}},
{0x6D,1,{0xBB}},
{0x6E,1,{0x9F}},
{0x6F,1,{0x23}},
{0x70,1,{0xB9}},
{0x71,1,{0x05}},
{0x7A,1,{0x07}},
{0x7B,1,{0x40}},
{0x7D,1,{0x01}},
{0x7F,1,{0x2C}},
{0x83,1,{0x0E}},
{0x84,1,{0xB6}},
{0x87,1,{0x0E}},
{0x88,1,{0x8A}},
{0x89,1,{0x14}},
{0x8B,1,{0x36}},
{0x8C,1,{0x3A}},
{0x8D,1,{0x3A}},
{0x8E,1,{0x3A}},
{0x95,1,{0x80}},
{0x96,1,{0xFD}},
{0x97,1,{0x14}},
{0x98,1,{0x17}},
{0x99,1,{0x01}},
{0x9A,1,{0x08}},
{0x9B,1,{0x02}},
{0x9C,1,{0x4C}},
{0x9D,1,{0xAF}},
{0x9F,1,{0x6B}},
{0xA0,1,{0xFF}},
{0xA2,1,{0x40}},
{0xA3,1,{0x6F}},
{0xA4,1,{0xF9}},
{0xA5,1,{0x45}},
{0xA6,1,{0x6A}},
{0xA7,1,{0x4C}},

{0xFF,1,{0x2C}},
{0xFB,1,{0x01}},
{0x00,1,{0x02}},
{0x01,1,{0x02}},
{0x02,1,{0x02}},
{0x03,1,{0x16}},
{0x04,1,{0x16}},
{0x05,1,{0x16}},
{0x0D,1,{0x1F}},
{0x0E,1,{0x1F}},
{0x16,1,{0x1B}},
{0x17,1,{0x4B}},
{0x18,1,{0x4B}},
{0x19,1,{0x4B}},
{0x2A,1,{0x03}},
{0x4D,1,{0x16}},
{0x4E,1,{0x02}},
{0x4F,1,{0x27}},
{0x53,1,{0x02}},
{0x54,1,{0x02}},
{0x55,1,{0x02}},
{0x56,1,{0x0E}},
{0x58,1,{0x0E}},
{0x59,1,{0x0E}},
{0x61,1,{0x1F}},
{0x62,1,{0x1F}},
{0x6A,1,{0x14}},
{0x6B,1,{0x34}},
{0x6C,1,{0x34}},
{0x6D,1,{0x34}},
{0x7E,1,{0x03}},
{0x9D,1,{0x0E}},
{0x9E,1,{0x02}},
{0x9F,1,{0x03}},

{0xFF,1,{0xE0}},
{0xFB,1,{0x01}},
{0x35,1,{0x82}},

{0xFF,1,{0xF0}},
{0xFB,1,{0x01}},
{0x1C,1,{0x01}},
{0x33,1,{0x01}},
{0x5A,1,{0x00}},
{0xD2,1,{0x52}},

{0xFF,1,{0xD0}},
{0xFB,1,{0x01}},
{0x53,1,{0x22}},
{0x54,1,{0x02}},

{0xFF,1,{0xC0}},
{0xFB,1,{0x01}},
{0x9C,1,{0x11}},
{0x9D,1,{0x11}},

{0xFF,1,{0x2B}},
{0xFB,1,{0x01}},
{0xB7,1,{0x08}},
{0xB8,1,{0x0B}},

{0xFF,1,{0x10}},
{0x35,1,{0x00}},
{0x11, 0, {} },
{REGFLAG_DELAY, 120, {} },
{0x29, 0, {} },
};

#ifdef LCM_SET_DISPLAY_ON_DELAY
/* to reduce init time, we move 120ms delay to lcm_set_display_on() !! */
static struct LCM_setting_table set_display_on[] = {
		{0x29, 0, {} },
};
#endif

static struct LCM_setting_table bl_level[] = {
	{0x51, 1, {0xFF} },
	{REGFLAG_END_OF_TABLE, 0x00, {} }
};

static void push_table(struct LCM_setting_table *table, unsigned int count, unsigned char force_update)
{
	unsigned int i;

	for (i = 0; i < count; i++) {
		unsigned cmd;
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
				dsi_set_cmdq_V2(cmd, table[i].count, table[i].para_list, force_update);
				break;
		}
	}
}

/* --------------------------------------------------------------------------- */
/* LCM Driver Implementations */
/* --------------------------------------------------------------------------- */

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
	params->dsi.vertical_frontporch = 54;
	params->dsi.vertical_frontporch_for_low_power = 54;
	params->dsi.vertical_active_line = FRAME_HEIGHT;

	params->dsi.horizontal_sync_active = 12;
	params->dsi.horizontal_backporch = 56;
	params->dsi.horizontal_frontporch = 76;
	params->dsi.horizontal_active_pixel = FRAME_WIDTH;
	params->dsi.ssc_disable = 1;

#if (LCM_DSI_CMD_MODE)
	params->dsi.PLL_CLOCK = 480;	/* this value must be in MTK suggested table */
#else
	params->dsi.PLL_CLOCK = 680;//274;	/* this value must be in MTK suggested table */
#endif
	params->dsi.data_rate = 1200;//548;

	params->dsi.clk_lp_per_line_enable = 0;
	params->dsi.esd_check_enable = 1;
	params->dsi.customization_esd_check_enable = 1;
	params->dsi.lcm_esd_check_table[0].cmd = 0x0A;
	params->dsi.lcm_esd_check_table[0].count = 1;
	params->dsi.lcm_esd_check_table[0].para_list[0] = 0x9C;

	//params->dsi.lane_swap_en = 0;
#ifdef MTK_RUNTIME_SWITCH_FPS_SUPPORT
	params->dsi.fps = 60;
#endif

#if 0//MTK_ROUND_CORNER_SUPPORT
	params->round_corner_params.round_corner_en = 1;
	params->round_corner_params.full_content = 0;
	params->round_corner_params.h = ROUND_CORNER_H_TOP;
	params->round_corner_params.h_bot = ROUND_CORNER_H_BOT;
	params->round_corner_params.tp_size = sizeof(top_rc_pattern);
	params->round_corner_params.lt_addr = (void *)top_rc_pattern;
#endif
}

#ifdef LCM_SET_DISPLAY_ON_DELAY
static U32 lcm_init_tick = 0;
static int is_display_on = 0;
#endif

static void lcm_init(void)
{
	int ret = 0;

	SET_RESET_PIN(0);
	MDELAY(100);
	display_bias_enable_v(5500);
	
	SET_RESET_PIN(0);

    MDELAY(15);
	SET_RESET_PIN(1);
	MDELAY(10);
	SET_RESET_PIN(0);
	MDELAY(200);

	SET_RESET_PIN(1);
	MDELAY(200);

	push_table(init_setting, sizeof(init_setting) / sizeof(struct LCM_setting_table), 1);

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

	while(!gpt4_timeout_tick(lcm_init_tick, timeout_tick)) {
		i++;
		if (i % 1000 == 0) {
			LCM_LOGI("nt35695B %s error: i=%u,lcm_init_tick=%u,cur_tick=%u\n", __func__,
				i, lcm_init_tick, gpt4_get_current_tick());
		}
	}

	push_table(set_display_on, sizeof(set_display_on) / sizeof(struct LCM_setting_table), 1);

	is_display_on = 1;
	return 0;
}
#endif

static void lcm_suspend(void)
{
	int ret;

	push_table(lcm_suspend_setting, sizeof(lcm_suspend_setting) / sizeof(struct LCM_setting_table), 1);

	display_bias_disable();

	MDELAY(5);

	SET_RESET_PIN(0);
	MDELAY(10);
}

static void lcm_resume(void)
{
	lcm_init();
}

#if LCM_DSI_CMD_MODE == 1
static void lcm_update(unsigned int x, unsigned int y, unsigned int width, unsigned int height)
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
	data_array[1] = (x1_MSB << 24) | (x0_LSB << 16) | (x0_MSB << 8) | 0x2a;
	data_array[2] = (x1_LSB);
	dsi_set_cmdq(data_array, 3, 1);

	data_array[0] = 0x00053902;
	data_array[1] = (y1_MSB << 24) | (y0_LSB << 16) | (y0_MSB << 8) | 0x2b;
	data_array[2] = (y1_LSB);
	dsi_set_cmdq(data_array, 3, 1);

	data_array[0] = 0x002c3909;
	dsi_set_cmdq(data_array, 1, 0);
}
#else /* not LCM_DSI_CMD_MODE */

static void lcm_update(unsigned int x, unsigned int y, unsigned int width, unsigned int height)
{
#ifdef LCM_SET_DISPLAY_ON_DELAY
	lcm_set_display_on();
#endif
}
#endif

//static struct LCM_setting_table set_page[] = {
//		{0xff, 1, {0x21} },
//};
static unsigned int lcm_compare_id(void)
{

	unsigned char buffer[8];
	unsigned int array[16];

	LCM_LOGI("%S: enter\n", __func__);

	SET_RESET_PIN(1);
	MDELAY(10);
	SET_RESET_PIN(0);
	MDELAY(10);
	SET_RESET_PIN(1);
	MDELAY(10);
	
	array[0] = 0x00033700;	/* read id return two byte,version and id */
	dsi_set_cmdq(array, 1, 1);
	read_reg_v2(0xA1, buffer, 3);
	LCM_LOGI("%s,nt3672C_id_jdi=%x %x %x\n", __func__, buffer[0],buffer[1],buffer[2]);

	//array[0] = 0x00013700;	/* read id return two byte,version and id */
	//dsi_set_cmdq(array, 1, 1);
	//read_reg_v2(0xDA, &buffer[0], 1);
	//read_reg_v2(0xDB, &buffer[1], 1);
	//read_reg_v2(0xDC, &buffer[2], 1);
	//LCM_LOGI("%s,nt3672C_id_jdi=%x %x %x\n", __func__, buffer[0],buffer[1],buffer[2]);

	//array[0] = 0x00083902;	/* read id return two byte,version and id */
	//array[1] = 0x000021ff;	/* read id return two byte,version and id */
	//dsi_set_cmdq(array, 2, 1);
	//push_table(set_page, sizeof(set_page) / sizeof(struct LCM_setting_table), 1);
	//array[0] = 0x00083700;	/* read id return two byte,version and id */
	//dsi_set_cmdq(array, 1, 1);
	//read_reg_v2(0xF1, buffer, 8);
	//LCM_LOGI("%s,nt3672C_id_jdi=%x %x %x %x %x %x %x %x\n", __func__, buffer[0],buffer[1],buffer[2],buffer[3],buffer[4],buffer[5],buffer[6],buffer[7]);

	if ((buffer[0] == 0x36)&&(buffer[1] == 0x72)&&(buffer[2] == 0x0e))
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
	LCM_LOGI("ATA check size = 0x%x,0x%x,0x%x,0x%x\n", x0_MSB, x0_LSB, x1_MSB, x1_LSB);
	data_array[0] = 0x0005390A; /* HS packet */
	data_array[1] = (x1_MSB << 24) | (x0_LSB << 16) | (x0_MSB << 8) | 0x2a;
	data_array[2] = (x1_LSB);
	dsi_set_cmdq(data_array, 3, 1);

	data_array[0] = 0x00043700; /* read id return two byte,version and id */
	dsi_set_cmdq(data_array, 1, 1);

	read_reg_v2(0x2A, read_buf, 4);

	if ((read_buf[0] == x0_MSB) && (read_buf[1] == x0_LSB)
	        && (read_buf[2] == x1_MSB) && (read_buf[3] == x1_LSB))
		ret = 1;
	else
		ret = 0;
	LCM_LOGI("ATA check size = 0x%x,0x%x,0x%x,0x%x\n", read_buf[0], read_buf[1], read_buf[2], read_buf[3]);

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

	LCM_LOGI("%s,nt36672c shenchao backlight: level = %d\n", __func__, level);

	bl_level[0].para_list[0] = level;

	push_table(bl_level, sizeof(bl_level) / sizeof(struct LCM_setting_table), 1);
}

static void lcm_setbacklight(unsigned int level)
{
	LCM_LOGI("%s,nt36672c backlight: level = %d\n", __func__, level);

	bl_level[0].para_list[0] = level;

	push_table(bl_level, sizeof(bl_level) / sizeof(struct LCM_setting_table), 1);
}

static void *lcm_switch_mode(int mode)
{
	return NULL;
}


LCM_DRIVER nt36672c_fhdp_dsi_vdo_jdi65_lcm_drv = {
	.name = "nt36672c_fhdp_dsi_vdo_jdi65_lcm_drv",
		#if defined(CONFIG_PRIZE_HARDWARE_INFO) && !defined (BUILD_LK)
	.lcm_info = {
		.chip	= "NT36672C",
		.vendor	= "mizan",
		.id		= "0x36720e",
		.more	= "lcm_2400*1080",
	},
	#endif
	.set_util_funcs = lcm_set_util_funcs,
	.get_params = lcm_get_params,
	.init = lcm_init,
	.suspend = lcm_suspend,
	.resume = lcm_resume,
	.compare_id = lcm_compare_id,
	.esd_check = lcm_esd_check,
	//.set_backlight = lcm_setbacklight,
	.ata_check = lcm_ata_check,
	.update = lcm_update,
	.switch_mode = lcm_switch_mode,
};
