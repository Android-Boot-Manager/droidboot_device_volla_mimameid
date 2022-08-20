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
#elif defined(BUILD_UBOOT)
#include <asm/arch/mt_gpio.h>
#else
#include <mach/mt_pm_ldo.h>
#include <mach/mt_gpio.h>
#endif

#ifdef BUILD_LK
#define LCM_LOGI(string, args...)  dprintf(0, "[LK/"LOG_TAG"]"string, ##args)
#define LCM_LOGD(string, args...)  dprintf(1, "[LK/"LOG_TAG"]"string, ##args)
#else
#define LCM_LOGI(fmt, args...)  pr_debug("[KERNEL/"LOG_TAG"]"fmt, ##args)
#define LCM_LOGD(fmt, args...)  pr_debug("[KERNEL/"LOG_TAG"]"fmt, ##args)
#endif

static LCM_UTIL_FUNCS lcm_util;

#define SET_RESET_PIN(v)			(lcm_util.set_reset_pin((v)))
#define MDELAY(n)					(lcm_util.mdelay(n))

/* --------------------------------------------------------------------------- */
/* Local Functions */
/* --------------------------------------------------------------------------- */
#define dsi_set_cmdq_V2(cmd, count, ppara, force_update) \
	lcm_util.dsi_set_cmdq_V2(cmd, count, ppara, force_update)
#define dsi_set_cmdq(pdata, queue_size, force_update) \
	lcm_util.dsi_set_cmdq(pdata, queue_size, force_update)
#define wrtie_cmd(cmd) \
	lcm_util.dsi_write_cmd(cmd)
#define write_regs(addr, pdata, byte_nums) \
	lcm_util.dsi_write_regs(addr, pdata, byte_nums)
#define read_reg(cmd) \
	lcm_util.dsi_dcs_read_lcm_reg(cmd)
#define read_reg_v2(cmd, buffer, buffer_size) \
	lcm_util.dsi_dcs_read_lcm_reg_v2(cmd, buffer, buffer_size)
#define set_gpio_lcd_enp(cmd) \
		lcm_util.set_gpio_lcd_enp_bias(cmd)
		

static const unsigned char LCD_MODULE_ID = 0x01;
/* --------------------------------------------------------------------------- */
/* Local Constants */
/* --------------------------------------------------------------------------- */
#define LCM_DSI_CMD_MODE	0
#define FRAME_WIDTH  										 (720)
#define FRAME_HEIGHT 										 (1600)

#define REGFLAG_DELAY             							 0xFFFA
#define REGFLAG_UDELAY             							 0xFFFB
#define REGFLAG_PORT_SWAP									 0xFFFC
#define REGFLAG_END_OF_TABLE      							 0xFFFD   // END OF REGISTERS MARKER

#ifndef TRUE
#define TRUE 1
#endif

#ifndef FALSE
#define FALSE 0
#endif

/* --------------------------------------------------------------------------- */
/* Local Variables */
/* --------------------------------------------------------------------------- */

struct LCM_setting_table
{
    unsigned int cmd;
    unsigned char count;
    unsigned char para_list[64];
};

static struct LCM_setting_table lcm_suspend_setting[] =
{
    {0x28, 1, {0x00}},
    {REGFLAG_DELAY, 10, {}},
    {0x10, 1, {0x00}},
    {REGFLAG_DELAY, 60, {} },
};

static struct LCM_setting_table lcm_initialization_setting[] =
{

{0xFF, 03,{0x98, 0x82, 0x01}}, 
{0x00, 01,{0x45}},    
{0x01, 01,{0x13}},    
{0x02, 01,{0x00}},    
{0x03, 01,{0x20}},    
{0x04, 01,{0x01}},    
{0x05, 01,{0x13}},    
{0x06, 01,{0x00}},    
{0x07, 01,{0x20}},    
{0x08, 01,{0x81}},    
{0x09, 01,{0x0F}},    
{0x16, 01,{0x81}},    
{0x17, 01,{0x0F}},    
{0x0A, 01,{0xF2}},    
{0x0B, 01,{0x00}},
{0x18, 01,{0xF2}},    
{0x19, 01,{0x00}},
{0x0C, 01,{0x00}},          
{0x0D, 01,{0x00}},          
{0x0E, 01,{0x07}},         
{0x0F, 01,{0x07}},         
{0x1A, 01,{0x00}},       
{0x1B, 01,{0x00}},         
{0x1C, 01,{0x07}},        
{0x1D, 01,{0x07}},        
{0x10, 01,{0x08}},     
{0x12, 01,{0x03}},     
{0x1E, 01,{0x08}},     
{0x20, 01,{0x03}},     
{0x31, 01,{0x0C}},     
{0x32, 01,{0x01}},     
{0x33, 01,{0x02}},     
{0x34, 01,{0x02}},     
{0x35, 01,{0x00}},     
{0x36, 01,{0x02}},     
{0x37, 01,{0x02}},     
{0x38, 01,{0x1E}},     
{0x39, 01,{0x16}},     
{0x3A, 01,{0x1C}},     
{0x3B, 01,{0x14}},     
{0x3C, 01,{0x1A}},     
{0x3D, 01,{0x12}},     
{0x3E, 01,{0x18}},     
{0x3F, 01,{0x10}},     
{0x40, 01,{0x08}},     
{0x41, 01,{0x08}},     
{0x42, 01,{0x07}},     
{0x43, 01,{0x07}},     
{0x44, 01,{0x07}},     
{0x45, 01,{0x07}},     
{0x46, 01,{0x07}},     
{0x47, 01,{0x0D}},     
{0x48, 01,{0x01}},     
{0x49, 01,{0x02}},     
{0x4A, 01,{0x02}},     
{0x4B, 01,{0x00}},     
{0x4C, 01,{0x02}},     
{0x4D, 01,{0x02}},     
{0x4E, 01,{0x1F}},     
{0x4F, 01,{0x17}},     
{0x50, 01,{0x1D}},     
{0x51, 01,{0x15}},     
{0x52, 01,{0x1B}},     
{0x53, 01,{0x13}},     
{0x54, 01,{0x19}},     
{0x55, 01,{0x11}},     
{0x56, 01,{0x09}},     
{0x57, 01,{0x09}},     
{0x58, 01,{0x07}},     
{0x59, 01,{0x07}},     
{0x5A, 01,{0x07}},     
{0x5B, 01,{0x07}},     
{0x5C, 01,{0x07}},     
{0x61, 01,{0x09}},     
{0x62, 01,{0x01}},     
{0x63, 01,{0x02}},     
{0x64, 01,{0x02}},     
{0x65, 01,{0x00}},     
{0x66, 01,{0x02}},     
{0x67, 01,{0x02}},     
{0x68, 01,{0x11}},     
{0x69, 01,{0x19}},     
{0x6A, 01,{0x13}},     
{0x6B, 01,{0x1B}},     
{0x6C, 01,{0x15}},     
{0x6D, 01,{0x1D}},     
{0x6E, 01,{0x17}},     
{0x6F, 01,{0x1F}},     
{0x70, 01,{0x0D}},     
{0x71, 01,{0x0D}},     
{0x72, 01,{0x07}},     
{0x73, 01,{0x07}},     
{0x74, 01,{0x07}},     
{0x75, 01,{0x07}},     
{0x76, 01,{0x07}},     
{0x77, 01,{0x08}},     
{0x78, 01,{0x01}},     
{0x79, 01,{0x02}},     
{0x7A, 01,{0x02}},     
{0x7B, 01,{0x00}},     
{0x7C, 01,{0x02}},     
{0x7D, 01,{0x02}},     
{0x7E, 01,{0x10}},     
{0x7F, 01,{0x18}},     
{0x80, 01,{0x12}},     
{0x81, 01,{0x1A}},     
{0x82, 01,{0x14}},     
{0x83, 01,{0x1C}},     
{0x84, 01,{0x16}},     
{0x85, 01,{0x1E}},     
{0x86, 01,{0x0C}},     
{0x87, 01,{0x0C}},     
{0x88, 01,{0x07}},     
{0x89, 01,{0x07}},     
{0x8A, 01,{0x07}},     
{0x8B, 01,{0x07}},     
{0x8C, 01,{0x07}},     
{0xA0, 01,{0x01}},
{0xA7, 01,{0x00}},
{0xA8, 01,{0x00}},
{0xA9, 01,{0x17}},
{0xAA, 01,{0x17}},
{0xAB, 01,{0x00}},
{0xAC, 01,{0x00}},
{0xAD, 01,{0x17}},
{0xAE, 01,{0x17}},
{0xD0, 01,{0x01}},
{0xD1, 01,{0x00}},
{0xE6, 01,{0x22}},
{0xE7, 01,{0x54}},
{0xFF, 03,{0x98, 0x82, 0x02}},
{0x06, 01,{0x9C}},     
{0x0B, 01,{0xA0}},     
{0x0C, 01,{0x00}},     
{0x0D, 01,{0x18}},     
{0x0E, 01,{0x40}},     
{0xF1, 01,{0x1C}},     
{0x4B, 01,{0x5A}},   
{0x50, 01,{0xCA}},   
{0x51, 01,{0x00}},   
{0x4E, 01,{0x11}},   
{0x4D, 01,{0xCE}},   
{0x40, 01,{0x47}},   
{0xFF, 03,{0x98, 0x82, 0x05}},
{0x03, 01,{0x00}},  
{0x04, 01,{0xC0}},  
{0x58, 01,{0x61}}, 
{0x63, 01,{0x97}},  
{0x64, 01,{0x97}},  
{0x68, 01,{0xA1}},  
{0x69, 01,{0xA7}},  
{0x6A, 01,{0x79}},  
{0x6B, 01,{0x6B}},  
{0x46, 01,{0x00}},  
{0x85, 01,{0x37}},  
{0xFF, 03,{0x98, 0x82, 0x06}},
{0xD9, 01,{0x1F}},    
{0xC0, 01,{0x40}},     
{0xC1, 01,{0x16}},     
{0xFF, 03,{0x98, 0x82, 0x07}},														
{0xFF, 03,{0x98, 0x82, 0x08}},	  						
{0xE0, 27,{0x54, 0x9A, 0x0C, 0x43, 0x81, 0x95, 0xAF, 0xD2, 0xFA, 0x1A, 0xAA, 0x4B, 0x71, 0x92, 0xB1, 0xFA, 0xD0, 0xF6, 0x0E, 0x2D, 0xFF, 0x48, 0x69, 0x8F, 0xA6, 0x03, 0xAA}},								
{0xE1, 27,{0x54, 0x96, 0x0C, 0x43, 0x81, 0x95, 0xAF, 0xD2, 0xFA, 0x1A, 0xAA, 0x4B, 0x71, 0x92, 0xB1, 0xFA, 0xD0, 0xF6, 0x0E, 0x2D, 0xFF, 0x48, 0x69, 0x8F, 0xA6, 0x03, 0xAA}},													
{0xFF, 03,{0x98, 0x82, 0x0B}},
{0x9A, 01,{0x45}},
{0x9B, 01,{0x03}},
{0x9C, 01,{0x04}},
{0x9D, 01,{0x04}},
{0x9E, 01,{0x7D}},
{0x9F, 01,{0x7D}},
{0xAB, 01,{0xE0}},    
{0xFF, 03,{0x98, 0x82, 0x0C}},
{0x00, 01,{0x19}},
{0x01, 01,{0x3D}},
{0x02, 01,{0x19}},
{0x03, 01,{0x56}},
{0x04, 01,{0x18}},
{0x05, 01,{0x51}},
{0x06, 01,{0x1A}},
{0x07, 01,{0x5A}},
{0x08, 01,{0x19}},
{0x09, 01,{0x58}},
{0x0A, 01,{0x18}},
{0x0B, 01,{0x4E}},
{0x0C, 01,{0x18}},
{0x0D, 01,{0x4D}},
{0x0E, 01,{0x17}},
{0x0F, 01,{0x47}},
{0x10, 01,{0x17}},
{0x11, 01,{0x49}},
{0x12, 01,{0x17}},
{0x13, 01,{0x46}},
{0x14, 01,{0x19}},
{0x15, 01,{0x54}},
{0x16, 01,{0x17}},
{0x17, 01,{0x4A}},
{0x18, 01,{0x19}},
{0x19, 01,{0x3E}},
{0x1A, 01,{0x18}},
{0x1B, 01,{0x52}},
{0x1C, 01,{0x17}},
{0x1D, 01,{0x48}},
{0x1E, 01,{0x19}},
{0x1F, 01,{0x40}},
{0x20, 01,{0x17}},
{0x21, 01,{0x4C}},
{0x22, 01,{0x18}},
{0x23, 01,{0x4F}},
{0x24, 01,{0x19}},
{0x25, 01,{0x55}},
{0x26, 01,{0x19}},
{0x27, 01,{0x3F}},
{0x28, 01,{0x1A}},
{0x29, 01,{0x45}},
{0x2A, 01,{0x19}},
{0x2B, 01,{0x53}},
{0x2C, 01,{0x1A}},
{0x2D, 01,{0x43}},
{0x2E, 01,{0x19}},
{0x2F, 01,{0x3C}},
{0x30, 01,{0x1A}},
{0x31, 01,{0x44}},
{0x32, 01,{0x1A}},
{0x33, 01,{0x59}},
{0x34, 01,{0x19}},
{0x35, 01,{0x41}},
{0x36, 01,{0x19}},
{0x37, 01,{0x57}},
{0x38, 01,{0x17}},
{0x39, 01,{0x4B}},
{0x3A, 01,{0x19}},
{0x3B, 01,{0x42}},
{0x3C, 01,{0x18}},
{0x3D, 01,{0x50}},				
{0xFF, 03,{0x98, 0x82, 0x0E}},
{0x49, 01,{0xC8}},    
{0x11, 01,{0x10}},    
{0x13, 01,{0x10}},    
{0x45, 01,{0x0B}},    
{0x46, 01,{0xAB}},    
{0x4D, 01,{0x8D}},    
{0xD0, 01,{0x34}},    
{0x00, 01,{0xA3}},    
{0x4B, 01,{0x1E}},    
{0x07, 01,{0x21}},    
{0xD1, 01,{0x01}},    
{0xD2, 01,{0x80}},    
{0xD3, 01,{0x80}},    
{0xD4, 01,{0x80}},    
{0xD5, 01,{0x80}},    
{0xD6, 01,{0x8D}},    
{0xD7, 01,{0x8D}},    
{0xD8, 01,{0x8D}},    
{0xD9, 01,{0x8D}},    
{0xDB, 01,{0x44}},    
{0xDB, 01,{0x44}},    
{0xDC, 01,{0x44}},    
{0xDD, 01,{0x44}},    
{0xC0, 01,{0x01}},    
{0xC1, 01,{0x80}},    
{0x40, 01,{0x07}},    
{0x02, 01,{0x0F}},    
{0x05, 01,{0x00}},    
{0xE1, 01,{0x0B}},    
{0xFF, 03,{0x98, 0x82, 0x00}},
{0x35,01,{0x00}},

{0x11,1,{0x00}},
{REGFLAG_DELAY,120,{}},
{0x29,1,{0x00}},
{REGFLAG_DELAY,20,{}},
};

static void push_table(struct LCM_setting_table *table, unsigned int count, unsigned char force_update)
{
    unsigned int i;

    for (i = 0; i < count; i++)
    {
        unsigned cmd;
        cmd = table[i].cmd;

        switch (cmd)
        {
        case REGFLAG_DELAY:
            if (table[i].count <= 10)
                MDELAY(table[i].count);
            else
                MDELAY(table[i].count);
            break;

        case REGFLAG_END_OF_TABLE:
            break;

        default:
            dsi_set_cmdq_V2(cmd, table[i].count, table[i].para_list, force_update);
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

	params->type                         = LCM_TYPE_DSI;
	params->width                        = FRAME_WIDTH;
	params->height                       = FRAME_HEIGHT;

	#ifndef BUILD_LK
	params->physical_width               = 68;
	params->physical_height              = 151;
	params->physical_width_um            = 67930;
	params->physical_height_um           = 150960;
	params->density                      = 320;
	#endif

	// enable tearing-free
	params->dbi.te_mode                  = LCM_DBI_TE_MODE_VSYNC_ONLY;
	params->dbi.te_edge_polarity         = LCM_POLARITY_RISING;

	#if (LCM_DSI_CMD_MODE)
	params->dsi.mode                     = CMD_MODE;
	params->dsi.switch_mode              = SYNC_PULSE_VDO_MODE;
	#else
	params->dsi.mode                     = SYNC_PULSE_VDO_MODE;//SYNC_EVENT_VDO_MODE;//BURST_VDO_MODE;////
	#endif

	// DSI
	/* Command mode setting */
	//1 Three lane or Four lane
	params->dsi.LANE_NUM                 = LCM_FOUR_LANE;
	//The following defined the fomat for data coming from LCD engine.
	params->dsi.data_format.color_order  = LCM_COLOR_ORDER_RGB;
	params->dsi.data_format.trans_seq    = LCM_DSI_TRANS_SEQ_MSB_FIRST;
	params->dsi.data_format.padding      = LCM_DSI_PADDING_ON_LSB;
	params->dsi.data_format.format       = LCM_DSI_FORMAT_RGB888;

	params->dsi.PS                       = LCM_PACKED_PS_24BIT_RGB888;

	#if (LCM_DSI_CMD_MODE)
	params->dsi.intermediat_buffer_num   = 0;//because DSI/DPI HW design change, this parameters should be 0 when video mode in MT658X; or memory leakage
	params->dsi.word_count               = FRAME_WIDTH * 3; //DSI CMD mode need set these two bellow params, different to 6577
	#else
	params->dsi.intermediat_buffer_num   = 2;	//because DSI/DPI HW design change, this parameters should be 0 when video mode in MT658X; or memory leakage
	#endif

	// Video mode setting
	params->dsi.packet_size              = 256;
params->dsi.vertical_sync_active     = 8;
	params->dsi.vertical_backporch       = 45;
	params->dsi.vertical_frontporch      = 75;
	params->dsi.vertical_active_line     = FRAME_HEIGHT;

	params->dsi.horizontal_sync_active   = 16;
	params->dsi.horizontal_backporch     = 45;//36
	params->dsi.horizontal_frontporch    = 48;//78
	
	params->dsi.horizontal_active_pixel  = FRAME_WIDTH;
	/* params->dsi.ssc_disable = 1; */

	params->dsi.PLL_CLOCK                = 280;//288,modified by anhengxuan,20210722
	params->dsi.ssc_disable                         = 0;
	params->dsi.ssc_range                           = 4;
	params->dsi.esd_check_enable = 1;
	params->dsi.customization_esd_check_enable = 1;
	params->dsi.lcm_esd_check_table[0].cmd          = 0x0a;
	params->dsi.lcm_esd_check_table[0].count        = 1;
	params->dsi.lcm_esd_check_table[0].para_list[0] = 0x9c;
}



static unsigned int lcm_compare_id(void)
{

    int array[4];
    char buffer[4]={0,0,0,0};
	char buffer1[4]={0,0,0,0};
	char buffer2[4]={0,0,0,0};
    int id=0;
	int rawdata = 0;
	int ret = 0;

	
	struct LCM_setting_table switch_table_page6[] = {
		{ 0xFF, 0x03, {0x98, 0x82, 0x06} }
	};
//	struct LCM_setting_table switch_table_page0[] = {
//		{ 0xFF, 0x03, {0x98, 0x81, 0x00} }
//	};
	
    display_ldo18_enable(1);
    display_bias_vpos_enable(1);
    display_bias_vneg_enable(1);
    MDELAY(10);
    display_bias_vpos_set(5800);
	display_bias_vneg_set(5800);
	MDELAY(10);
	SET_RESET_PIN(1);
	MDELAY(10);
    SET_RESET_PIN(0);
    MDELAY(20);
    SET_RESET_PIN(1);
    MDELAY(120);

	push_table(switch_table_page6,sizeof(switch_table_page6) / sizeof(struct LCM_setting_table),1);

    MDELAY(5);
    array[0] = 0x00013700;
    dsi_set_cmdq(array, 1, 1);
    //MDELAY(10);
    read_reg_v2(0xF0, buffer, 1);//    NC 0x00  0x98 
	
	MDELAY(5);
    array[0] = 0x00013700;
    dsi_set_cmdq(array, 1, 1);
    //MDELAY(10);
    read_reg_v2(0xF1, buffer1, 1);//    NC 0x00  0x82 
	
	MDELAY(5);
    array[0] = 0x00013700;
    dsi_set_cmdq(array, 1, 1);
    //MDELAY(10);
    read_reg_v2(0xF2, buffer2, 1);//    NC 0x00  0x0d 
    id = (buffer[0]<<16) | (buffer1[0]<<8) |buffer2[0];
	
#ifdef BUILD_LK
	dprintf(CRITICAL, "%s, LK debug: ili9882 id = 0x%08x\n", __func__, id);
#else
	printk("%s: ili9882 id = 0x%08x \n", __func__, id);
#endif

  
    if(0x98820d == id)
	{
	    return 1;
	}
    else{
		return 0;
	}

}

static void lcm_init(void)
{
//prize-add-pengzhipeng-20191128-start
	int ret = 0;

	/*config rt5081 register 0xB2[7:6]=0x3, that is set db_delay=4ms.*/
	ret = PMU_REG_MASK(0xB2, (0x3 << 6), (0x3 << 6));

	/* set AVDD 5.4v, (4v+28*0.05v) */
	/*ret = RT5081_write_byte(0xB3, (1 << 6) | 28);*/
	ret = PMU_REG_MASK(0xB3, 28, (0x3F << 0));
	if (ret < 0)
		dprintf("nt35695----tps6132----cmd=%0x--i2c write error----\n", 0xB3);
	else
		dprintf("nt35695----tps6132----cmd=%0x--i2c write success----\n", 0xB3);

	/* set AVEE */
	/*ret = RT5081_write_byte(0xB4, (1 << 6) | 28);*/
	ret = PMU_REG_MASK(0xB4, 28, (0x3F << 0));
	if (ret < 0)
		dprintf("nt35695----tps6132----cmd=%0x--i2c write error----\n", 0xB4);
	else
		dprintf("nt35695----tps6132----cmd=%0x--i2c write success----\n", 0xB4);

	/* enable AVDD & AVEE */
	/* 0x12--default value; bit3--Vneg; bit6--Vpos; */
	/*ret = RT5081_write_byte(0xB1, 0x12 | (1<<3) | (1<<6));*/
	ret = PMU_REG_MASK(0xB1, (1<<3) | (1<<6), (1<<3) | (1<<6));
	if (ret < 0)
		dprintf("nt35695----tps6132----cmd=%0x--i2c write error----\n", 0xB1);
	else
		dprintf("nt35695----tps6132----cmd=%0x--i2c write success----\n", 0xB1);
//prize-add-pengzhipeng-20191128-end
	MDELAY(10);
    MDELAY(5);
    SET_RESET_PIN(1);
    MDELAY(5);
    SET_RESET_PIN(0);
    MDELAY(5);
    SET_RESET_PIN(1);
    MDELAY(10);
    
    push_table(lcm_initialization_setting, sizeof(lcm_initialization_setting) / sizeof(struct LCM_setting_table), 1);
}

static void lcm_suspend(void)
{
//prize-add-pengzhipeng-20191128-start
	int ret = 0;
//prize-add-pengzhipeng-20191128-end
    push_table(lcm_suspend_setting, sizeof(lcm_suspend_setting) / sizeof(struct LCM_setting_table), 1);

    SET_RESET_PIN(1);
    MDELAY(5);
//prize-add-pengzhipeng-20191128-start
	ret = PMU_REG_MASK(0xB1, (0<<3) | (0<<6), (1<<3) | (1<<6));
	if (ret < 0)
		dprintf("nt35695----tps6132----cmd=%0x--i2c write error----\n", 0xB1);
	else
		dprintf("nt35695----tps6132----cmd=%0x--i2c write success----\n", 0xB1);

	MDELAY(5);
//prize-add-pengzhipeng-20191128-end
}

static void lcm_resume(void)
{
    lcm_init();
}

LCM_DRIVER ili9882n_hdp_dsi_vdo_incell_truly_lcm_drv =
{
    .name			= "ili9882n_hdp_dsi_vdo_incell_truly",
#if defined(CONFIG_PRIZE_HARDWARE_INFO) && !defined (BUILD_LK)
	.lcm_info = {
		.chip	= "ili9882n_hdp",
		.vendor	= "truly",
		.id		= "0x82",
		.more	= "720*1600",
	},
#endif
    .set_util_funcs = lcm_set_util_funcs,
    .get_params     = lcm_get_params,
    .init           = lcm_init,
    .suspend        = lcm_suspend,
    .resume         = lcm_resume,
    .compare_id     = lcm_compare_id,
};
