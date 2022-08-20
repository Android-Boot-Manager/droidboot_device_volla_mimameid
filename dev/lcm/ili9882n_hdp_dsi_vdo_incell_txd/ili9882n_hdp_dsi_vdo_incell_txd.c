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

#ifdef MTK_ROUND_CORNER_SUPPORT
#include "data_rgba4444_roundedpattern.h"
#endif

#ifdef BUILD_LK
#include <platform/upmu_common.h>
#include <platform/mt_gpio.h>
#include <platform/mt_i2c.h>
#include <platform/mt_pmic.h>
#include <platform/mt_gpt.h>
#include <string.h>
#include <lcm_pmic.h>
#elif defined(BUILD_UBOOT)
#include <asm/arch/mt_gpio.h>
#else
#include <mach/mt_pm_ldo.h>
#include <mach/mt_gpio.h>
#endif

#ifdef BUILD_LK
#define LCM_LOGI(string, args...)  dprintf(ALWAYS, "[LK/"LOG_TAG"]"string, ##args)
#define LCM_LOGD(string, args...)  dprintf(INFO, "[LK/"LOG_TAG"]"string, ##args)
#else
#define LCM_LOGI(fmt, args...)  pr_notice("[KERNEL/"LOG_TAG"]"fmt, ##args)
#define LCM_LOGD(fmt, args...)  pr_debug("[KERNEL/"LOG_TAG"]"fmt, ##args)
#endif

#define LCM_DSI_CMD_MODE                0
#define FRAME_WIDTH			(720)
#define FRAME_HEIGHT			(1600)

//prize-tangcong modify LCD size-20200331-start
#define LCM_PHYSICAL_WIDTH                  				(63100)
#define LCM_PHYSICAL_HEIGHT                  				(123970)
//prize-tangcong modify LCD size-20200331-end

#define REGFLAG_PORT_SWAP               0xFFFA
#define REGFLAG_DELAY                   0xFFFC
#define REGFLAG_UDELAY                  0xFFFB
#define REGFLAG_END_OF_TABLE            0xFFFD


//#ifndef GPIO_LCD_BIAS_ENP_PIN
#define GPIO_LCD_BIAS_ENP (GPIO20 | 0x80000000)  //prize add 
//#endif
//#ifndef GPIO_LCD_BIAS_ENN_PIN
#define GPIO_LCD_BIAS_ENN (GPIO17  | 0x80000000)//prize add 
//#endif
//#ifndef GPIO_LCD_RESET
#define GPIO_LCD_RST (GPIO45  | 0x80000000)//prize add 
//#endif

//#define GPIO_BUS_FD_EN_PIN (GPIO42  | 0x80000000)//prize add 

#define GPIO_65132_ENP  GPIO_LCD_BIAS_ENP
#define GPIO_65132_ENN  GPIO_LCD_BIAS_ENN

// ---------------------------------------------------------------------------
//  Local Variable
// ---------------------------------------------------------------------------
static LCM_UTIL_FUNCS lcm_util;

#define SET_RESET_PIN(v)    (lcm_util.set_reset_pin((v)))
#define MDELAY(n)       (lcm_util.mdelay(n))
#define UDELAY(n)       (lcm_util.udelay(n))

extern U32 gpt4_time2tick_ms(U32 time_ms);
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

struct LCM_setting_table {
	unsigned int cmd;
	unsigned char count;
	unsigned char para_list[120];
};

static struct LCM_setting_table lcm_initialization_setting[] = {
{0xFF,3,{0x98,0x82,0x01}},
{0x00,1,{0x40}},
{0x01,1,{0x00}},
{0x02,1,{0x10}},
{0x03,1,{0x00}},
{0x08,1,{0x01}},
{0x09,1,{0x05}},
{0x0a,1,{0x40}},
{0x0b,1,{0x00}},
{0x0c,1,{0x10}},
{0x0d,1,{0x10}},
{0x0e,1,{0x00}},
{0x0f,1,{0x00}},
{0x10,1,{0x00}},
{0x11,1,{0x00}},
{0x12,1,{0x00}},
{0x31,1,{0x07}},
{0x32,1,{0x07}},
{0x33,1,{0x07}},
{0x34,1,{0x02}},
{0x35,1,{0x02}},
{0x36,1,{0x02}},
{0x37,1,{0x02}},
{0x38,1,{0x2B}},
{0x39,1,{0x2A}},
{0x3A,1,{0x14}},
{0x3B,1,{0x13}},
{0x3C,1,{0x12}},
{0x3D,1,{0x11}},
{0x3E,1,{0x10}},
{0x3F,1,{0x08}},
{0x40,1,{0x07}},
{0x41,1,{0x07}},
{0x42,1,{0x07}},
{0x43,1,{0x02}},
{0x44,1,{0x02}},
{0x45,1,{0x06}},
{0x46,1,{0x06}},
{0x47,1,{0x07}},
{0x48,1,{0x07}},
{0x49,1,{0x07}},
{0x4A,1,{0x02}},
{0x4B,1,{0x02}},
{0x4C,1,{0x02}},
{0x4D,1,{0x02}},
{0x4E,1,{0x2B}},
{0x4F,1,{0x2A}},
{0x50,1,{0x14}},
{0x51,1,{0x13}},
{0x52,1,{0x12}},
{0x53,1,{0x11}},
{0x54,1,{0x10}},
{0x55,1,{0x08}},
{0x56,1,{0x07}},
{0x57,1,{0x07}},
{0x58,1,{0x07}},
{0x59,1,{0x02}},
{0x5A,1,{0x02}},
{0x5B,1,{0x06}},
{0x5C,1,{0x06}},
{0x61,1,{0x07}},
{0x62,1,{0x07}},
{0x63,1,{0x07}},
{0x64,1,{0x02}},
{0x65,1,{0x02}},
{0x66,1,{0x02}},
{0x67,1,{0x02}},
{0x68,1,{0x2B}},
{0x69,1,{0x2A}},
{0x6A,1,{0x10}},
{0x6B,1,{0x11}},
{0x6C,1,{0x12}},
{0x6D,1,{0x13}},
{0x6E,1,{0x14}},
{0x6F,1,{0x08}},
{0x70,1,{0x07}},
{0x71,1,{0x07}},
{0x72,1,{0x07}},
{0x73,1,{0x02}},
{0x74,1,{0x02}},
{0x75,1,{0x06}},
{0x76,1,{0x06}},
{0x77,1,{0x07}},
{0x78,1,{0x07}},
{0x79,1,{0x07}},
{0x7A,1,{0x02}},
{0x7B,1,{0x02}},
{0x7C,1,{0x02}},
{0x7D,1,{0x02}},
{0x7E,1,{0x2B}},
{0x7F,1,{0x2A}},
{0x80,1,{0x10}},
{0x81,1,{0x11}},
{0x82,1,{0x12}},
{0x83,1,{0x13}},
{0x84,1,{0x14}},
{0x85,1,{0x08}},
{0x86,1,{0x07}},
{0x87,1,{0x07}},
{0x88,1,{0x07}},
{0x89,1,{0x02}},
{0x8A,1,{0x02}},
{0x8B,1,{0x06}},
{0x8C,1,{0x06}},
{0xb0,1,{0x33}},
{0xb1,1,{0x33}},
{0xb2,1,{0x00}},
{0xc3,1,{0xff}},
{0xca,1,{0x44}},
{0xd0,1,{0x01}},
{0xd1,1,{0x12}},
{0xd2,1,{0x10}},
{0xd5,1,{0x55}},
{0xd8,1,{0x01}},
{0xd9,1,{0x02}},
{0xda,1,{0x80}},
{0xdc,1,{0x10}},
{0xdd,1,{0x40}},
{0xdf,1,{0xb6}},
{0xe2,1,{0x75}},
{0xe6,1,{0x22}},
{0xe7,1,{0x54}},
{0xFF,3,{0x98,0x82,0x02}},
{0xF1,1,{0x1C}},
{0x4B,1,{0x5A}},
{0x50,1,{0xCA}},
{0x51,1,{0x00}},
{0x06,1,{0x8F}},
{0x0B,1,{0xA0}},
{0x0C,1,{0x00}},
{0x0D,1,{0x0E}},
{0x0E,1,{0xE6}},
{0x4E,1,{0x11}},
{0xFF,3,{0x98,0x82,0x05}},
{0x03,1,{0x01}},
{0x04,1,{0x17}},
{0x50,1,{0x0F}},
{0x58,1,{0x61}},
{0x63,1,{0x9C}},
{0x64,1,{0x9C}},
{0x68,1,{0x29}},
{0x69,1,{0x2F}},
{0x6A,1,{0x3D}},
{0x6B,1,{0x2F}},
{0x85,1,{0x37}},
{0x46,1,{0x00}},
{0xFF,3,{0x98,0x82,0x06}},
{0xD9,1,{0x1F}},
{0xC0,1,{0x40}},
{0xC1,1,{0x16}},
{0x48,1,{0x0F}},
{0x4D,1,{0x80}},
{0x4E,1,{0x40}},
{0x83,1,{0x00}},
{0x84,1,{0x00}},
{0xC7,1,{0x05}},
{0xFF,3,{0x98,0x82,0x08}},
{0xE0,27,{0x40,0x24,0xAE,0xE9,0x2C,0x55,0x5E,0x85,0xB2,0xD5,0xAA,0x0B,0x35,0x5B,0x80,0xEA,0xA6,0xD4,0xF0,0x14,0xFF,0x31,0x57,0x85,0xAB,0x03,0xEC}},
{0xE1,27,{0x40,0x24,0xAE,0xE9,0x2C,0x55,0x5E,0x85,0xB2,0xD5,0xAA,0x0B,0x35,0x5B,0x80,0xEA,0xA6,0xD4,0xF0,0x14,0xFF,0x31,0x57,0x85,0xAB,0x03,0xEC}},
{0xFF,3,{0x98,0x82,0x0B}},
{0x9A,1,{0x44}},
{0x9B,1,{0x84}},
{0x9C,1,{0x03}},
{0x9D,1,{0x03}},
{0x9E,1,{0x71}},
{0x9F,1,{0x71}},
{0xAB,1,{0xE0}},
{0xFF,3,{0x98,0x82,0x0E}},
{0x11,1,{0x10}},
{0x13,1,{0x10}},
{0x00,1,{0xA0}},
{0xFF,3,{0x98,0x82,0x00}},
{0x35,1,{0x00}},

{0x11,1,{0x00}},
{REGFLAG_DELAY,120,{}},
{0x29,1,{0x00}},
{REGFLAG_DELAY,20,{}},

};

static struct LCM_setting_table lcm_deep_sleep_mode_in_setting[] = {
	//{0x26, 1, {0x08}},
	// Display off sequence
	{0x28, 0, {0x00}},
	{REGFLAG_DELAY, 50, {}},
	// Sleep Mode On
	{0x10, 0, {0x00}},
	{REGFLAG_DELAY, 120, {}},
	{REGFLAG_END_OF_TABLE, 0x00, {}},
};

static void lcm_set_bias(void)
{
	unsigned char cmd = 0x00;
	unsigned char data = 0x14;
	
	int ret = 0;
	
	mt_set_gpio_mode(GPIO_65132_ENN, GPIO_MODE_00);
	mt_set_gpio_dir(GPIO_65132_ENN, GPIO_DIR_OUT);
	mt_set_gpio_out(GPIO_65132_ENN, GPIO_OUT_ONE);

	MDELAY(30);
	mt_set_gpio_mode(GPIO_65132_ENP, GPIO_MODE_00);
	mt_set_gpio_dir(GPIO_65132_ENP, GPIO_DIR_OUT);
	mt_set_gpio_out(GPIO_65132_ENP, GPIO_OUT_ONE);
	MDELAY(50);
//	dprintf(0,"lcm_set_bias===========================\n");
}

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
		}
	}
}

// ---------------------------------------------------------------------------
//  LCM Driver Implementations
// ---------------------------------------------------------------------------
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

//	params->virtual_width = VIRTUAL_WIDTH;
//	params->virtual_height = VIRTUAL_HEIGHT;

#if (LCM_DSI_CMD_MODE)
	params->dsi.mode            = CMD_MODE;
#else
	params->dsi.mode            = BURST_VDO_MODE;
#endif

	/* The following defined the fomat for data coming from LCD engine. */
	params->dsi.data_format.color_order	= LCM_COLOR_ORDER_RGB;
	params->dsi.data_format.trans_seq	= LCM_DSI_TRANS_SEQ_MSB_FIRST;
	params->dsi.data_format.padding		= LCM_DSI_PADDING_ON_LSB;
	params->dsi.data_format.format		= LCM_DSI_FORMAT_RGB888;

	/* Highly depends on LCD driver capability */
	/* video mode timing */
	params->dsi.PS                                  = LCM_PACKED_PS_24BIT_RGB888;
	params->dsi.vertical_sync_active                = 4;
	params->dsi.vertical_backporch                  = 10;
	params->dsi.vertical_frontporch                 = 230;
	params->dsi.vertical_active_line                = FRAME_HEIGHT;
	params->dsi.horizontal_sync_active              = 8;
	params->dsi.horizontal_backporch                = 20;
	params->dsi.horizontal_frontporch               = 20;
	params->dsi.horizontal_active_pixel             = FRAME_WIDTH;
	params->dsi.LANE_NUM                            = LCM_FOUR_LANE;
	/*prize-zhaopengge modify lcd fps-20201012-start*/
	params->dsi.PLL_CLOCK                           = 260;
	//params->dsi.compatibility_for_nvk 				= 0;
	/*prize-zhaopengge modify lcd fps-20201012-end*/
     //prize-tangcong modify LCD size-20200331-start
	params->physical_width = LCM_PHYSICAL_WIDTH/1000;
	params->physical_height = LCM_PHYSICAL_HEIGHT/1000;
	//prize-tangcong modify LCD size-20200331-end
	params->dsi.ssc_disable                         = 1;
	params->dsi.ssc_range                           = 4;

	params->dsi.HS_TRAIL                            = 15;
	params->dsi.noncont_clock                       = 1;
	params->dsi.noncont_clock_period                = 1;

	/* ESD check function */
	params->dsi.esd_check_enable                    = 1;
	params->dsi.customization_esd_check_enable      = 1;
	//params->dsi.clk_lp_per_line_enable              = 1;
	params->dsi.lcm_esd_check_table[0].cmd          = 0x0A;
	params->dsi.lcm_esd_check_table[0].count        = 1;
	params->dsi.lcm_esd_check_table[0].para_list[0] = 0x9C;
#ifdef MTK_ROUND_CORNER_SUPPORT
        params->round_corner_params.round_corner_en = 1;
        params->round_corner_params.full_content = 0;
        params->round_corner_params.w = ROUND_CORNER_W;
        params->round_corner_params.h = ROUND_CORNER_H;
        params->round_corner_params.lt_addr = left_top;
        params->round_corner_params.lb_addr = left_bottom;
        params->round_corner_params.rt_addr = right_top;
        params->round_corner_params.rb_addr = right_bottom;
#endif
}

static void lcm_suspend_power(void)
{
	int ret = 0;
	dprintf(CRITICAL, "%s--------------------\n", __func__);
		/* enable AVDD+ & AVDD- */
	/* 0x12--default value; bit3--Vneg; bit6--Vpos; */
	ret = PMU_REG_MASK(0xB1, (0<<3) | (0<<6), (1<<3) | (1<<6));
	if (ret < 0)
		LCM_LOGI("ili9882----cmd=%0x--i2c write error----\n", 0xB1);
	else
		LCM_LOGI("ili9882----cmd=%0x--i2c write success----\n", 0xB1);

	MDELAY(5);

	SET_RESET_PIN(0);
	MDELAY(10);
}

static void lcm_resume_power(void)
{
	dprintf(CRITICAL, "%s--------------------\n", __func__);
}

static void lcm_init(void)
{
//prize-add ldo 1.8v-pengzhipeng-20201119-start
	display_ldo18_enable(1);
	MDELAY(15);
//prize-add ldo 1.8v-pengzhipeng-20201119-end
	lcm_set_bias();
	SET_RESET_PIN(0);
	
//add by prize chenfeiwen 20201215---start
	int ret = 0;
	dprintf(CRITICAL, "%s--------------------\n", __func__);
		SET_RESET_PIN(0);
	/*config mt6371 register 0xB2[7:6]=0x3, that is set db_delay=4ms.*/
	ret = PMU_REG_MASK(0xB2, (0x3 << 6), (0x3 << 6));

	/* set AVDD+ 5.7v, (4v+32*0.05v) */
	ret = PMU_REG_MASK(0xB3, 28, (0x3F << 0));
	if (ret < 0)
		LCM_LOGI("ili9882----cmd=%0x--i2c write error----\n", 0xB3);
	else
		LCM_LOGI("ili9882----cmd=%0x--i2c write success----\n", 0xB3);

	/* set AVDD- 5.7  (4v+32*0.05v) */
	ret = PMU_REG_MASK(0xB4, 28, (0x3F << 0));
	if (ret < 0)
		LCM_LOGI("ili9882----cmd=%0x--i2c write error----\n", 0xB4);
	else
		LCM_LOGI("ili9882----cmd=%0x--i2c write success----\n", 0xB4);

	/* enable AVDD+ & AVDD- */
	/* 0x12--default value; bit3--Vneg; bit6--Vpos; */
	ret = PMU_REG_MASK(0xB1, (1<<3) | (1<<6), (1<<3) | (1<<6));
	if (ret < 0)
		LCM_LOGI("ili9882----cmd=%0x--i2c write error----\n", 0xB1);
	else
		LCM_LOGI("ili9882----cmd=%0x--i2c write success----\n", 0xB1);
//add by prize chenfeiwen 20201215---end

	MDELAY(15);

	SET_RESET_PIN(1);
	MDELAY(10);
	SET_RESET_PIN(0);
	MDELAY(10);
	SET_RESET_PIN(1);
	MDELAY(120);

	push_table(lcm_initialization_setting, sizeof(lcm_initialization_setting) / sizeof(struct LCM_setting_table), 1);
}

static void lcm_suspend(void)
{
	int ret = 0;
	push_table(lcm_deep_sleep_mode_in_setting, sizeof(lcm_deep_sleep_mode_in_setting) / sizeof(struct LCM_setting_table), 1);
	/* enable AVDD+ & AVDD- */
	/* 0x12--default value; bit3--Vneg; bit6--Vpos; */
	ret = PMU_REG_MASK(0xB1, (0<<3) | (0<<6), (1<<3) | (1<<6));
	if (ret < 0)
		LCM_LOGI("ili9882----cmd=%0x--i2c write error----\n", 0xB1);
	else
		LCM_LOGI("ili9882----cmd=%0x--i2c write success----\n", 0xB1);

	MDELAY(5);

//prize-add ldo 1.8v-pengzhipeng-20201119-start
display_ldo18_enable(0);
//prize-add ldo 1.8v-pengzhipeng-20201119-end
	SET_RESET_PIN(0);
	MDELAY(10);
}

static void lcm_resume(void)
{
	lcm_init();
}

/*prize-Solve the LCD td4321 compatible-anhengxuan-20201112-start*/
extern int IMM_GetOneChannelValue(int dwChannel, int data[4], int *rawdata);
static unsigned int lcm_compare_id_tt(void)
{
    unsigned int rawdata=0;
    int data[4] = {0, 0, 0, 0};    
    int adc_value=0;
	IMM_GetOneChannelValue(2,data,&rawdata);
	adc_value=data[0]*1000+data[1]*10;
#ifdef BUILD_LK
    LCM_LOGI("LCD_ID_td4321 adc_value = %d\n",adc_value);
#else
    LCM_LOGI("LCM_ID_td4321 adc_value = %d\n",adc_value);
#endif
	if ((adc_value>800)&&(adc_value<1200) )
	{
		return 1;
	}
	else
    {
		return 0;
    }
}
/*prize-Solve the LCD td4321 compatible-anhengxuan-20201112-end*/

static unsigned int lcm_compare_id(void)
{
	int array[4];
    char buffer[4]={0,0,0,0};
	char buffer1[4]={0,0,0,0};
	char buffer2[4]={0,0,0,0};
    unsigned int id=0;
	//int rawdata = 0;
	int ret = 0;
	
	dprintf(CRITICAL, "%s--------------------\n", __func__);

	
	struct LCM_setting_table lcm_id[] = {
		{ 0xFF, 0x03, {0x98, 0x82, 0x06} }
	};
//	struct LCM_setting_table switch_table_page0[] = {
//		{ 0xFF, 0x03, {0x98, 0x81, 0x00} }
//	};
/*	
    display_ldo18_enable(1);
    display_bias_vpos_enable(1);
    display_bias_vneg_enable(1);
    MDELAY(10);
    display_bias_vpos_set(5800);
	display_bias_vneg_set(5800);
*/

	MDELAY(10);
	SET_RESET_PIN(1);
	MDELAY(10);
    SET_RESET_PIN(0);
    MDELAY(20);
    SET_RESET_PIN(1);
    MDELAY(120);

	push_table(lcm_id, sizeof(lcm_id) / sizeof(struct LCM_setting_table), 1);
	 MDELAY(5);
	    array[0]=0x00043902;
        array[1]=0x068298ff;
        dsi_set_cmdq(array, 2, 1);
	MDELAY(5);
	array[0]=0x00023700;
	dsi_set_cmdq(array, 1, 1);
    read_reg_v2(0xF0,&buffer[0], 1);
    MDELAY(5);
    read_reg_v2(0xF1,&buffer[1], 1);
    MDELAY(5);
    read_reg_v2(0xF2,&buffer[2], 1);

    id = ( (buffer[0] << 16)|(buffer[1] << 8)| (buffer[2]) ); //
	
#ifdef BUILD_LK
	dprintf(CRITICAL, "%s, LK debug: ili9882 id = 0x%08x\n", __func__, id);
#else
	printk("%s: ili9882 id = 0x%08x \n", __func__, id);
#endif
	lcm_compare_id_tt();
	
    if(0x98820d == id)
	{
		//is_9882N_detect=1;
	    return 1;
	}
    else{
		return 0;
	}

}

static unsigned int lcm_ata_check(unsigned char *buffer)
{
#ifndef BUILD_LK
#if 1
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
	
	data_array[0] = 0x0003390A; /* HS packet */
	data_array[1] = 0x00595AF0;
	dsi_set_cmdq(data_array, 2, 1);

	data_array[0] = 0x0003390A; /* HS packet */
	data_array[1] = 0x00A6A5F1;
	dsi_set_cmdq(data_array, 2, 1);
	
	data_array[0] = 0x0004390A; /* HS packet */
	data_array[1] = (x1_LSB << 24) | (x1_MSB << 16) | (x0_LSB << 8) | 0xB9;
	dsi_set_cmdq(data_array, 2, 1);

	data_array[0] = 0x00033700; /* read id return two byte,version and id */
	dsi_set_cmdq(data_array, 1, 1);
	read_reg_v2(0x04, read_buf, 3);
	printk("%s read[0x04]= %x  %x %x %x\n", __func__, read_buf[0], read_buf[1],read_buf[2],read_buf[3]);

//	MDELAY(10);
	data_array[0] = 0x00033700; /* read id return two byte,version and id */
	dsi_set_cmdq(data_array, 1, 1);
	read_reg_v2(0xB9, read_buf, 3);
	printk("%s read[0xB9]= %x %x %x %x \n", __func__, read_buf[0],read_buf[1],read_buf[2],read_buf[3]);

	if ((read_buf[0] == x0_LSB) && (read_buf[1] == x1_MSB)
	        && (read_buf[2] == x1_LSB))
		ret = 1;
	else
		ret = 0;

	return ret;
#endif
	return 1;
#else
	return 0;
#endif
}

LCM_DRIVER ili9882n_hdp_dsi_vdo_incell_txd_lcm_drv =
{
	.name		= "ili9882n_hdp_dsi_vdo_incell_txd",
	#if defined(CONFIG_PRIZE_HARDWARE_INFO) && !defined (BUILD_LK)
	.lcm_info = {
		.chip	= "0x98820d",
		.vendor	= "ilitek",
		.more	= "720*1600",
	},
#endif
	.set_util_funcs	= lcm_set_util_funcs,
	.get_params	= lcm_get_params,
	.init		= lcm_init,
	.suspend	= lcm_suspend,
	.resume		= lcm_resume,
	.compare_id	= lcm_compare_id,
	.ata_check	= lcm_ata_check,
#ifndef BUILD_LK
	.resume_power	= lcm_resume_power,
	.suspend_power	= lcm_suspend_power,
#endif
};
