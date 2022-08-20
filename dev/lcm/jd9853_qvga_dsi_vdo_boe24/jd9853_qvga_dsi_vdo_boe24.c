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
#include "lcm_util.h"


#ifdef BUILD_LK
	#include <platform/upmu_common.h>
	#include <platform/mt_gpio.h>
	#include <platform/mt_i2c.h>
	#include <platform/mt_pmic.h>
	#include <platform/mt_gpt.h>
	#include <string.h>
#ifndef MACH_FPGA
	#include <lcm_pmic.h>
#endif
#elif defined(BUILD_UBOOT)
	#include <asm/arch/mt_gpio.h>
#else
	#include <mach/mt_pm_ldo.h>
	#include <mach/mt_gpio.h>
#endif

#include <platform/mt_i2c.h>
#include <libfdt.h>

#ifdef BUILD_LK
	#define LCM_LOGI(string, args...)  dprintf(0, "[LK/"LOG_TAG"]"string, ##args)
	#define LCM_LOGD(string, args...)  dprintf(1, "[LK/"LOG_TAG"]"string, ##args)
#else
	#define LCM_LOGI(fmt, args...)  pr_debug("[KERNEL/"LOG_TAG"]"fmt, ##args)
	#define LCM_LOGD(fmt, args...)  pr_debug("[KERNEL/"LOG_TAG"]"fmt, ##args)
#endif


static LCM_UTIL_FUNCS lcm_util;

#define SET_RESET_PIN(v)	(lcm_util.set_reset_pin((v)))
#define MDELAY(n)		(lcm_util.mdelay(n))
#define UDELAY(n)		(lcm_util.udelay(n))

extern U32 gpt4_time2tick_ms(U32 time_ms);

#define dsi_set_cmdq_V2(cmd, count, ppara, force_update) \
		lcm_util.dsi_set_cmdq_V2(cmd, count, ppara, force_update)
#define dsi_set_cmdq(pdata, queue_size, force_update) \
		lcm_util.dsi_set_cmdq(pdata, queue_size, force_update)
#define wrtie_cmd(cmd) lcm_util.dsi_write_cmd(cmd)
#define write_regs(addr, pdata, byte_nums) \
		lcm_util.dsi_write_regs(addr, pdata, byte_nums)
#define read_reg(cmd)	lcm_util.dsi_dcs_read_lcm_reg(cmd)
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
	#include <linux/uaccess.h>
	#include <linux/interrupt.h>
	#include <linux/io.h>
	#include <linux/platform_device.h>
#endif

#define FRAME_WIDTH			(240)
#define FRAME_HEIGHT			(320)
#define HFP (40)
#define HSA (4)
#define HBP (20)
#define VFP (8)
#define VSA (2)
#define VBP (6)

#define REGFLAG_DELAY			0xFFFC
#define REGFLAG_UDELAY			0xFFFB
#define REGFLAG_END_OF_TABLE		0xFFFD
#define REGFLAG_RESET_LOW		0xFFFE
#define REGFLAG_RESET_HIGH		0xFFFF

#ifndef TRUE
#define TRUE 1
#endif

#ifndef FALSE
#define FALSE 0
#endif

/* i2c control end */

struct LCM_setting_table {
	unsigned int cmd;
	unsigned char count;
	unsigned char para_list[200];
};

static struct LCM_setting_table lcm_suspend_setting[] = {
	{0x28, 0, {} },
	{REGFLAG_DELAY, 50, {} },
	{0x10, 0, {} },
	{REGFLAG_DELAY, 150, {} },
};

static struct LCM_setting_table init_setting_vdo[] = {
	{0xDF, 0x3, {0x98,0x51,0xE9}},
	//{0x21, 0x0, {}},
	{0xDE, 0x1, {0x0}},
	{0xB7, 0x4, {0x13,0x76,0x13,0x3A}},
	{0xC8, 0x20, {0x3F,0x31,0x26,0x20,0x23,0x28,0x23,0x24,0x23,
				  0x21,0x20,0x15,0x11,0x0F,0x0C,0x0E,0x3F,0x31,
				  0x26,0x20,0x23,0x28,0x23,0x24,0x23,0x21,0x20,
				  0x15,0x11,0x0F,0x0C,0x0E}},
	{0xB9, 0x3, {0x33,0x08,0xCC}},
	{0xBB, 0x8, {0x47,0x7A,0x40,0x30,0x7C,0x60,0x70,0x70}},
	{0xBC, 0x2, {0x38,0x3C}},
	
	{0xC0, 0x2, {0x26,0x20}},
	{0xC1, 0x1, {0x12}},
	{0xC3, 0x9, {0x08,0x00,0x0A,0x10,0x08,0x54,0x45,0x71,0x2C}},
	{0xC4, 0x11, {0x00,0xA0,0x79,0x0E,0x0A,0x16,0x79,0x0E,0x0A,
				 0x16,0x79,0x0E,0x0A,0x16,0x82,0x00,0x03}},
	{0xD0, 0x6, {0x04,0x0C,0x6B,0x0F,0x07,0x03}},
	{0xD7, 0x2, {0x13,0x00}},
	
	{0xDE, 0x1, {0x02}},
	{0xB8, 0x5, {0x19,0xA0,0x2F,0x04,0x33}},
	{0xB2, 0x4, {0x30,0xFA,0x93,0x6E}},
	{0xC1, 0x4, {0x10,0x66,0x66,0x01}},
	{0xDE, 0x1, {0x00}},
	{0x11, 0, {}},
	{REGFLAG_DELAY, 120, {} }, //Delay 200ms
	{0x29, 0, {}},
	{REGFLAG_DELAY, 50, {} }, //Delay 100ms
    {REGFLAG_END_OF_TABLE, 0x00, {}}
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
			dsi_set_cmdq_V2(cmd, table[i].count, table[i].para_list,
					force_update);
			break;
		}
	}
}

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

	params->dsi.mode = SYNC_EVENT_VDO_MODE;
	params->dsi.switch_mode = CMD_MODE;
	params->dsi.switch_mode_enable = 0;

	/* DSI */
	/* Command mode setting */
	params->dsi.LANE_NUM = LCM_ONE_LANE;
	/* The following defined the fomat for data coming from LCD engine. */
	params->dsi.data_format.color_order = LCM_COLOR_ORDER_RGB;
	params->dsi.data_format.trans_seq = LCM_DSI_TRANS_SEQ_MSB_FIRST;
	params->dsi.data_format.padding = LCM_DSI_PADDING_ON_LSB;
	params->dsi.data_format.format = LCM_DSI_FORMAT_RGB888;

	/* Highly depends on LCD driver capability. */
	params->dsi.packet_size = 256;
	/* video mode timing */

	params->dsi.PS = LCM_PACKED_PS_24BIT_RGB888;

	params->dsi.vertical_sync_active = VSA;
	params->dsi.vertical_backporch = VBP;
	params->dsi.vertical_frontporch = VFP;
	params->dsi.vertical_frontporch_for_low_power = 750;
	params->dsi.vertical_active_line = FRAME_HEIGHT;

	params->dsi.horizontal_sync_active = HSA;
	params->dsi.horizontal_backporch = HBP;
	params->dsi.horizontal_frontporch = HFP;
	params->dsi.horizontal_active_pixel = FRAME_WIDTH;
	params->dsi.ssc_disable = 1;
#ifndef MACH_FPGA
	/* this value must be in MTK suggested table */
	params->dsi.PLL_CLOCK = 74;
#else
	params->dsi.pll_div1 = 0;
	params->dsi.pll_div2 = 0;
	params->dsi.fbk_div = 0x1;
#endif
	params->dsi.clk_lp_per_line_enable = 0;
	params->dsi.esd_check_enable = 0;
	params->dsi.customization_esd_check_enable = 0;
	params->dsi.lcm_esd_check_table[0].cmd = 0x0a;
	params->dsi.lcm_esd_check_table[0].count = 1;
	params->dsi.lcm_esd_check_table[0].para_list[0] = 0x9c;
}

/* turn on gate ic & control voltage to 5.5V */
static void lcm_init_power(void)
{

}

static void lcm_suspend_power(void)
{

}

static void lcm_resume_power(void)
{

}

static void lcm_init(void)
{
//	unsigned int data_array[20]; 
//	SET_RESET_PIN(0);
	SET_RESET_PIN(1);
	MDELAY(50);
	SET_RESET_PIN(0);
	MDELAY(50);
	SET_RESET_PIN(1);
	MDELAY(155);

#if 0 //150901
	data_array[0] = 0x00042902;
	data_array[1] = 0xE95198DF;
	dsi_set_cmdq(data_array, 2, 1);		

	data_array[0] = 0x00210500;
	dsi_set_cmdq(data_array, 1, 1);
	
	data_array[0]=0x00DE1500;
	dsi_set_cmdq(data_array, 1, 1);	

	data_array[0] = 0x00052902;
	data_array[1] = 0x137613B7;
	data_array[2] = 0x0000003A;
	dsi_set_cmdq(data_array, 3, 1);
	
	//data_array[0] = 0x773A1500;
	//dsi_set_cmdq(data_array, 1, 1);
	data_array[0] = 0x00212902;
	data_array[1] = 0x26313FC8;
	data_array[2] = 0x23282320;
	data_array[3] = 0x20212324;
	data_array[4] = 0x0C0F1115;
	data_array[5] = 0x26313F0E;
	data_array[6] = 0x23282320;
	data_array[7] = 0x20212324;
	data_array[8] = 0x0C0F1115;
	data_array[9] = 0x0000000E;
	dsi_set_cmdq(data_array, 10, 1);
	
	data_array[0] = 0x00042902;
	data_array[1] = 0xCC0833B9;
	dsi_set_cmdq(data_array, 2, 1);
	data_array[0]=0x00093902;
	data_array[1]=0x407A47BB;
	data_array[2]=0x70607C30;
	data_array[3]=0x00000070;
	dsi_set_cmdq(data_array, 4, 1);	
	data_array[0]=0x00033902;
	data_array[1]=0x003C38BC;
	dsi_set_cmdq(data_array, 2, 1);		
	data_array[0]=0x00033902;
	data_array[1]=0x002026C0;
	dsi_set_cmdq(data_array, 2, 1);			
	data_array[0]=0x12C11500;
	dsi_set_cmdq(data_array, 1, 1);		
	data_array[0]=0x000A3902;
	data_array[1]=0x0A0008C3;
	data_array[2]=0x45540810;
	data_array[3]=0x00002C71;
	dsi_set_cmdq(data_array, 4, 1);		
	
	data_array[0]=0x00123902;
	data_array[1]=0x79A000C4;
	data_array[2]=0x79160A0E;
	data_array[3]=0x79160A0E;
	data_array[4]=0x82160A0E;
	data_array[5]=0x00000300;
	dsi_set_cmdq(data_array, 6, 1);		
	
	data_array[0]=0x00073902;
	data_array[1]=0x6B0C04D0;
	data_array[2]=0x0003070F;
	dsi_set_cmdq(data_array, 3, 1);		
	
	data_array[0]=0x00033902;
	data_array[1]=0x000013D7;
	dsi_set_cmdq(data_array, 2, 1);		
	
	data_array[0]=0x02DE1500;
	dsi_set_cmdq(data_array, 1, 1);		
	
	data_array[0]=0x00063902;
	data_array[1]=0x2FA019B8;
	data_array[2]=0x00003304;
	dsi_set_cmdq(data_array, 3, 1);	
	
	data_array[0]=0x00053902;
	data_array[1]=0x93FA30B2;
	data_array[2]=0x0000006E;
	dsi_set_cmdq(data_array, 3, 1);	
	
	data_array[0]=0x00053902;
	data_array[1]=0x666610C1;
	data_array[2]=0x00000001;
	dsi_set_cmdq(data_array, 3, 1);	
	
	data_array[0]=0x00DE1500;
	dsi_set_cmdq(data_array, 1, 1);	
	
	data_array[0] = 0x00110500;
	dsi_set_cmdq(data_array, 1, 1);
    MDELAY(120);
	data_array[0] = 0x00290500;
	dsi_set_cmdq(data_array, 1, 1);
	MDELAY(50);
#else
	push_table(init_setting_vdo, sizeof(init_setting_vdo) /
		   sizeof(struct LCM_setting_table), 1);
#endif
}


static void lcm_suspend(void)
{
	push_table(lcm_suspend_setting, sizeof(lcm_suspend_setting) /
		   sizeof(struct LCM_setting_table), 1);

	SET_RESET_PIN(0);
	MDELAY(10);
}

static void lcm_resume(void)
{
	lcm_init();
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

	data_array[0] = 0x00043700; /* read id return two byte,version and id */
	dsi_set_cmdq(data_array, 1, 1);

	read_reg_v2(0x2A, read_buf, 4);

	if ((read_buf[0] == x0_MSB) && (read_buf[1] == x0_LSB) &&
	    (read_buf[2] == x1_MSB) && (read_buf[3] == x1_LSB))
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
	LCM_LOGI("%s,jd9853 backlight: level = %d\n", __func__, level);
}

static void lcm_setbacklight(unsigned int level)
{
	LCM_LOGI("%s,jd9853 backlight: level = %d\n", __func__, level);
}

static unsigned int lcm_compare_id(void)
{
	unsigned int id = 0;
	unsigned char buffer[1];
	unsigned int array[16];

	SET_RESET_PIN(1);
	SET_RESET_PIN(0);
	MDELAY(1);

	SET_RESET_PIN(1);
	MDELAY(20);

	array[0] = 0x00013700;  /* read id return 1byte */
	dsi_set_cmdq(array, 1, 1);
	read_reg_v2(0x04, buffer, 1);
	id = buffer[0];

	LCM_LOGI("%s,jd9853 id = 0x%08x\n", __func__, id);
	return 1;
	#if 0
	if (id == LCM_ID_TD4330)
		return 1;
	else
		return 0;
	#endif
}

LCM_DRIVER jd9853_qvga_dsi_vdo_boe24_lcm_drv = {
	.name = "jd9853_qvga_dsi_vdo_boe24_drv",
	.set_util_funcs = lcm_set_util_funcs,
	.get_params = lcm_get_params,
	.init = lcm_init,
	.suspend = lcm_suspend,
	.resume = lcm_resume,
	.init_power = lcm_init_power,
	.compare_id = lcm_compare_id,
	.resume_power = lcm_resume_power,
	.suspend_power = lcm_suspend_power,
	.set_backlight = lcm_setbacklight,
	.ata_check = lcm_ata_check,
};

