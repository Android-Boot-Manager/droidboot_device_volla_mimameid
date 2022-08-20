#ifndef BUILD_LK
#include <linux/string.h>
#endif
#include "lcm_drv.h"

#ifdef BUILD_LK
#include <platform/mt_gpio.h>
#include <platform/mt_pmic.h>
#elif defined(BUILD_UBOOT)
#include <asm/arch/mt_gpio.h>
#else
//#include <mach/mt_gpio.h>
//#include <linux/xlog.h>
//#include <mach/mt_pm_ldo.h>
#endif


// ---------------------------------------------------------------------------
//  Local Constants
// ---------------------------------------------------------------------------

#define FRAME_WIDTH  										(720)
#define FRAME_HEIGHT 										(1440)

#define REGFLAG_DELAY             							0XFE
#define REGFLAG_END_OF_TABLE      							0xFFF   // END OF REGISTERS MARKER

#define LCM_ID_ILI9881C	0x98811C

#define LCM_DSI_CMD_MODE									0

#ifndef TRUE
    #define   TRUE     1
#endif
 
#ifndef FALSE
    #define   FALSE    0
#endif

// ---------------------------------------------------------------------------
//  Local Variables
// ---------------------------------------------------------------------------

static LCM_UTIL_FUNCS lcm_util = {0};

#define SET_RESET_PIN(v)    								(lcm_util.set_reset_pin((v)))

#define UDELAY(n) 											(lcm_util.udelay(n))
#define MDELAY(n) 											(lcm_util.mdelay(n))


// ---------------------------------------------------------------------------
//  Local Functions
// ---------------------------------------------------------------------------

#define dsi_set_cmdq_V2(cmd, count, ppara, force_update)	lcm_util.dsi_set_cmdq_V2(cmd, count, ppara, force_update)
#define dsi_set_cmdq(pdata, queue_size, force_update)		lcm_util.dsi_set_cmdq(pdata, queue_size, force_update)
#define wrtie_cmd(cmd)										lcm_util.dsi_write_cmd(cmd)
#define write_regs(addr, pdata, byte_nums)					lcm_util.dsi_write_regs(addr, pdata, byte_nums)
#define read_reg											lcm_util.dsi_read_reg()
#define read_reg_v2(cmd, buffer, buffer_size)				lcm_util.dsi_dcs_read_lcm_reg_v2(cmd, buffer, buffer_size)

 struct LCM_setting_table {
    unsigned cmd;
    unsigned char count;
    unsigned char para_list[64];
};


static struct LCM_setting_table lcm_initialization_setting[] = {
	
{0xE0,2,{0xAB,0xBA}},
{0xE1,2,{0xBA,0xAB}},
{0xB1,4,{0x10,0x01,0x47,0xFF}},
{0xB2,6,{0x0C,0x14,0x04,0x50,0x50,0x14}},
{0xB3,3,{0x56,0x52,0xA0}},
{0xB4,3,{0x33,0x30,0x04}},
{0xB6,7,{0xB0,0x00,0x00,0x10,0x00,0x10,0x00}},
{0xB8,5,{0x05,0x12,0x29,0x49,0x48}},  
{0xB9,38,{0x7C,0x37,0x29,0x1E,0x1D,0x10,0x19,0x06,0x23,0x25,0x27,0x47,0x38,0x42,0x35,0x34,0x26,0x15,0x06,0x7C,0x37,0x29,0x1E,0x1D,0x10,0x19,0x06,0x23,0x25,0x27,0x47,0x38,0x42,0x35,0x34,0x26,0x15,0x06}},
{0xC0,16,{0x54,0x56,0x12,0x34,0x44,0x44,0x33,0x33,0x90,0x04,0x70,0x04,0x3F,0x00,0x00,0xC1}},
{0xC1,10,{0x14,0x94,0x02,0x89,0x90,0x04,0x70,0x04,0x54,0x00}},        
{0xC2,12,{0x37,0x09,0x08,0x89,0x08,0x11,0x22,0x20,0x44,0xBB,0x18,0x00}},
{0xC3,22,{0x92,0x52,0x10,0x10,0x0E,0x0E,0x0C,0x0C,0x04,0x02,0x00,0x00,0x02,0x1C,0x06,0x1D,0x02,0x02,0x02,0x02,0x02,0x02}},
{0xC4,22,{0x13,0x13,0x11,0x11,0x0F,0x0F,0x0D,0x0D,0x05,0x02,0x00,0x00,0x02,0x1C,0x07,0x1D,0x02,0x02,0x02,0x02,0x02,0x02}},
{0xC6,2,{0x2A,0x2A}},                          
{0xC8,6,{0x11,0x00,0x31,0x42,0x34,0x16}},      
{0xC9,5,{0xA1,0x22,0xFF,0xC4,0x23}},           
{0xCA,2,{0xCB,0x43}},                          
{0xCD,8,{0x0E,0x4F,0x4F,0x30,0x14,0x6B,0x06,0xB3}},
{0xD2,4,{0xE3,0x2B,0x38,0x00}},                
{0xD4,11,{0x00,0x01,0x00,0x0E,0x04,0x44,0x08,0x10,0x00,0x00,0x00}},
{0xE6,8,{0x00,0x01,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF}},
{0xF0,5,{0x12,0x03,0x20,0x00,0xFF}},
{0xF3,1,{0x00}},

		 //sleep out 
		{0x11,1,{0x00}},       
		{REGFLAG_DELAY, 150, {}},
		// Display ON            
		{0x29, 1, {0x00}},       
	{REGFLAG_END_OF_TABLE, 0x00, {}}
};

/*

static struct LCM_setting_table lcm_backlight_level_setting[] = {
	{0x51, 1, {0xFF}},
	{REGFLAG_END_OF_TABLE, 0x00, {}}
};
*/

static void push_table(struct LCM_setting_table *table, unsigned int count, unsigned char force_update)
{
	unsigned int i;

    for(i = 0; i < count; i++) {
		
        unsigned cmd;
        cmd = table[i].cmd;
		
        switch (cmd) {
			
            case REGFLAG_DELAY :
                MDELAY(table[i].count);
                break;
				
            case REGFLAG_END_OF_TABLE :
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
	
		params->type   = LCM_TYPE_DSI;

		params->width  = FRAME_WIDTH;
		params->height = FRAME_HEIGHT;
	//	params->density = 320;//LCM_DENSITY;

#if LCM_DSI_CMD_MODE
		params->dsi.mode   = CMD_MODE;
#else
		params->dsi.mode   = BURST_VDO_MODE;
#endif
		params->dsi.vertical_sync_active = 4;
		params->dsi.vertical_backporch = 12;
		params->dsi.vertical_frontporch = 20;
		params->dsi.vertical_active_line = FRAME_HEIGHT; 

		params->dsi.horizontal_sync_active				= 20;	
		params->dsi.horizontal_backporch				= 80;
		params->dsi.horizontal_frontporch				= 80;  //80
		params->dsi.horizontal_active_pixel				= FRAME_WIDTH;

		params->dsi.LANE_NUM = LCM_FOUR_LANE;
		
		params->dbi.te_mode 				= LCM_DBI_TE_MODE_VSYNC_ONLY;
		params->dbi.te_edge_polarity		= LCM_POLARITY_RISING;
		params->dsi.data_format.color_order = LCM_COLOR_ORDER_RGB;
		params->dsi.data_format.trans_seq   = LCM_DSI_TRANS_SEQ_MSB_FIRST;
		params->dsi.data_format.padding     = LCM_DSI_PADDING_ON_LSB;
		params->dsi.data_format.format      = LCM_DSI_FORMAT_RGB888;
		
		
		params->dsi.packet_size=256;
		params->dsi.PS=LCM_PACKED_PS_24BIT_RGB888;
		// Video mode setting		
		params->dsi.intermediat_buffer_num = 2;


		// params->dsi.HS_TRAIL=20; 
		params->dsi.PLL_CLOCK = 251;  //240=4db  230=1.5db
/***********************    esd  check   ***************************/
#ifndef BUILD_LK
		params->dsi.esd_check_enable = 0;
		params->dsi.customization_esd_check_enable = 0;
		params->dsi.lcm_esd_check_table[0].cmd          = 0x0a;
		params->dsi.lcm_esd_check_table[0].count        = 1;
		params->dsi.lcm_esd_check_table[0].para_list[0] = 0x1C;
#endif

}


#define AUXADC_COMPARE_ID          0
#define AUXADC_LCM_VOLTAGE_CHANNEL (2)
#define MIN_VOLTAGE (200)
#define MAX_VOLTAGE (600)

static unsigned int lcm_compare_id(void)
{
     int res = 0;
	int i=0;
    int lcm_vol = 0;
    int rawdata = 0;
    int data[4] = {0,0,0,0};
	int avg = 3;

	res = IMM_GetOneChannelValue(AUXADC_LCM_VOLTAGE_CHANNEL, data, &rawdata);


	printf("zgj:[cpu-adc_uboot]: lcm_vol = %d==XXXXX",lcm_vol);

	
    if(res < 0)
    {
#ifdef BUILD_LK
        printf("cjx:[adc_uboot]: res = %d, get data error\n",res);
#else
        printk("cjx:[adc_kernel]: get data error\n");
#endif
        return 0;
    }

	 lcm_vol = data[0]*1000+data[1]*10;
#ifdef BUILD_LK
    printf("zgj:[adc_uboot]: lcm_vol= %d , file : %s, line : %d\n", lcm_vol, __FILE__, __LINE__);
#else
    printk("zgj:[adc_kernel]: lcm_vol= %d , file : %s, line : %d\n", lcm_vol, __FILE__, __LINE__);
#endif

    if (lcm_vol >= MIN_VOLTAGE && lcm_vol <= MAX_VOLTAGE)
    {
		printf("zgj:lcm->er68577_hd1440_dsi_vdo_hsd_hz\n");
        return 1;
    }
    return 0;
}

static void lcm_init(void)
{
	//display_ldo28_enable(1);
	//display_ldo18_enable(1);
	//display_bias_enable();
	
    SET_RESET_PIN(1);
    MDELAY(10);
    SET_RESET_PIN(0);
    MDELAY(20);
    SET_RESET_PIN(1);
    MDELAY(120);

	push_table(lcm_initialization_setting, sizeof(lcm_initialization_setting) / sizeof(struct LCM_setting_table), 1);
}


static void lcm_suspend(void)
{

//	push_table(lcm_deep_sleep_mode_in_setting, sizeof(lcm_deep_sleep_mode_in_setting) / sizeof(struct LCM_setting_table), 1);
	SET_RESET_PIN(1);
	MDELAY(1);
	SET_RESET_PIN(0);
	MDELAY(1);
	SET_RESET_PIN(1);
	MDELAY(120);
	
	//display_bias_disable();
	//display_ldo18_enable(0);
	//`nnnndisplay_ldo28_enable(0);
}


static void lcm_resume(void)
{
//	lcm_initialization_setting[141].para_list[0] = lcm_initialization_setting[141].para_list[0] + 1; 
	lcm_init();
	
//	push_table(lcm_sleep_out_setting, sizeof(lcm_sleep_out_setting) / sizeof(struct LCM_setting_table), 1);
}

/*
static void lcm_update(unsigned int x, unsigned int y,
                       unsigned int width, unsigned int height)
{
	unsigned int x0 = x;
	unsigned int y0 = y;
	unsigned int x1 = x0 + width - 1;
	unsigned int y1 = y0 + height - 1;

	unsigned char x0_MSB = ((x0>>8)&0xFF);
	unsigned char x0_LSB = (x0&0xFF);
	unsigned char x1_MSB = ((x1>>8)&0xFF);
	unsigned char x1_LSB = (x1&0xFF);
	unsigned char y0_MSB = ((y0>>8)&0xFF);
	unsigned char y0_LSB = (y0&0xFF);
	unsigned char y1_MSB = ((y1>>8)&0xFF);
	unsigned char y1_LSB = (y1&0xFF);

	unsigned int data_array[16];

	data_array[0]= 0x00053902;
	data_array[1]= (x1_MSB<<24)|(x0_LSB<<16)|(x0_MSB<<8)|0x2a;
	data_array[2]= (x1_LSB);
	data_array[3]= 0x00053902;
	data_array[4]= (y1_MSB<<24)|(y0_LSB<<16)|(y0_MSB<<8)|0x2b;
	data_array[5]= (y1_LSB);
	data_array[6]= 0x002c3909;

	dsi_set_cmdq(data_array, 7, 0);

}


static void lcm_setbacklight(unsigned int level)
{
	unsigned int default_level = 145;
	unsigned int mapped_level = 0;

	//for LGE backlight IC mapping table
	if(level > 255) 
			level = 255;

	if(level >0) 
			mapped_level = default_level+(level)*(255-default_level)/(255);
	else
			mapped_level=0;

	// Refresh value of backlight level.
	lcm_backlight_level_setting[0].para_list[0] = mapped_level;

	push_table(lcm_backlight_level_setting, sizeof(lcm_backlight_level_setting) / sizeof(struct LCM_setting_table), 1);
}

*/


#ifndef BUILD_LK
extern atomic_t ESDCheck_byCPU;
#endif


//static unsigned int ili_lcm_compare_id(void)
//{

 //return 1;
//}
//prize-chenxiaowen-20161013-compatibility-(ili9881c_hd720_dsi_vdo_yin vs ili9881c_hd720_dsi_vdo_cmi)-end


LCM_DRIVER er68577_hdplus_dsi_vdo_cmi_lcm_drv = {
	.name		= "er68577_hdplus_dsi_vdo_cmi",
    	//prize-lixuefeng-20150512-start
	#if defined(CONFIG_PRIZE_HARDWARE_INFO) && !defined (BUILD_LK)
	.lcm_info = {
		.chip	= "er68577",
		.vendor	= "mizan",
		.id		= "0xe8",
		.more	= "lcm_1440*720",
	},
	#endif
	//prize-lixuefeng-20150512-end	
	.set_util_funcs = lcm_set_util_funcs,
	.get_params     = lcm_get_params,
	.init           = lcm_init,
	.suspend        = lcm_suspend,
	.resume         = lcm_resume,
	//prize-chenxiaowen-20161013-compatibility-(ili9881c_hd720_dsi_vdo_yin vs ili9881c_hd720_dsi_vdo_cmi)-start
  //#if defined(ILI9881C_HD720_DSI_VDO_YIN) && defined(ILI9881C_HD720_DSI_VDO_CMI)
 // .compare_id = ili_lcm_compare_id,
 // #else
  .compare_id = lcm_compare_id,
 // #endif
	//prize-chenxiaowen-20161013-compatibility-(ili9881c_hd720_dsi_vdo_yin vs ili9881c_hd720_dsi_vdo_cmi)-end
#if (LCM_DSI_CMD_MODE)
	.set_backlight	= lcm_setbacklight,
    .update         = lcm_update,
#endif
};

