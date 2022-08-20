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

//#define SET_RESET_PIN(v)			(lcm_util.set_reset_pin((v)))
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
#define FRAME_HEIGHT 										 (1560)

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
{0xB9,3,{0x83,0x10,0x2D}},

{0xB1,11,{0x22,0x22,0x27,0x27,0x32,0x52,0x57,0x3F,0x08,0x08,0x08}},

{0xB2,14,{0x00,0x00,0x06,0x18,0x00,0x10,0xBE,0x37,0x00,0x00,0x00,0x00,0xF4,0xA0}},

{0xB4,14,{0x55,0x55,0x55,0x55,0x55,0x55,0x55,0x55,0x03,0xFF,0x01,0x20,0x00,0xFF}},

{0xCC,1,{0x02}},

{0xD3,25,{0x00,0x00,0x00,0x00,0x00,0x10,0x10,0x27,0x00,0x74,0x37,0x11,0x11,0x00,0x00,0x32,0x10,0x0B,0x00,0x0B,0x32,0x16,0x2A,0x06,0x2A}},

{0xD5,44,{0x24,0x25,0x18,0x18,0x19,0x19,0x18,0x18,0x18,0x18,0x18,0x18,0x18,0x18,0x18,0x18,0x18,0x18,0x06,0x07,0x04,0x05,0x18,0x18,0x18,0x18,0x02,0x03,0x00,0x01,0x20,0x21,0x18,0x18,0x18,0x18,0x18,0x18,0x18,0x18,0x18,0x18,0x18,0x18}},

{0xD6,44,{0x21,0x20,0x18,0x18,0x18,0x18,0x19,0x19,0x18,0x18,0x18,0x18,0x18,0x18,0x18,0x18,0x18,0x18,0x01,0x00,0x03,0x02,0x18,0x18,0x18,0x18,0x05,0x04,0x07,0x06,0x25,0x24,0x18,0x18,0x18,0x18,0x18,0x18,0x18,0x18,0x18,0x18,0x18,0x18}},

{0xE7,4,{0xFF,0x14,0x00,0x00}},

{0xBD,1,{0x01}},

{0xE7,1,{0x01}},

{0xBD,1,{0x00}},

{0xE0,46,{0x00,0x06,0x11,0x19,0x21,0x3A,0x54,0x59,0x60,0x5B,0x73,0x77,0x7B,0x89,0x87,0x90,0x9B,0xAF,0xB1,0x59,0x61,0x6D,0x7F,0x00,0x06,0x11,0x19,0x21,0x3A,0x54,0x59,0x60,0x5B,0x73,0x77,0x7B,0x89,0x87,0x90,0x9B,0xAF,0xB1,0x59,0x61,0x6D,0x7F}},

{0xBA,19,{0x70,0x23,0xA8,0x93,0xB2,0xC0,0xC0,0x01,0x10,0x00,0x00,0x00,0x0C,0x3D,0x82,0x77,0x04,0x01,0x00}},

{0xC0,6,{0x33,0x33,0x00,0x00,0x19,0x21}},

{0xBD,1,{0x01}},

{0xCB,1,{0x01}},

{0xD3,3,{0x01,0x00,0xF9}},

{0xBD,1,{0x00}},

{0xCB,5,{0x00,0x53,0x00,0x02,0xCA}},

{0xBF,7,{0xFC,0x00,0x24,0x9E,0xF6,0x00,0x5D}},

{0xBD,1,{0x02}},

{0xB4,8,{0x42,0x00,0x33,0x00,0x33,0x88,0xB3,0x00}},

{0xBD,1,{0x00}},

{0xD1,2,{0x20,0x01}},

{0xBD,1,{0x02}},

{0xB1,3,{0x7F,0x03,0xF5}},

{0xBD,1,{0x00}},

{0x51,2,{0x0F,0xFF}},//EN-CABC
{0xC9,4,{0x04,0x09,0xE0,0x00}},
{0X00,2,{0x55,0x02}},
{0X00,2,{0x53,0x24}},
{0x35,1,{0x00}},
{0x11,0,{}},
{REGFLAG_DELAY,150,{}},

{0x29,0,{}},

{REGFLAG_DELAY, 20, {}},

	{REGFLAG_END_OF_TABLE, 0x00, {}}     
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

	params->dsi.vertical_sync_active     = 2;
	params->dsi.vertical_backporch       = 12;
	params->dsi.vertical_frontporch      = 186;
	params->dsi.vertical_active_line     = FRAME_HEIGHT;

	params->dsi.horizontal_sync_active   = 9;
	params->dsi.horizontal_backporch     = 29;//36
	params->dsi.horizontal_frontporch    = 17;//78
	params->dsi.horizontal_active_pixel  = FRAME_WIDTH;
	/* params->dsi.ssc_disable = 1; */
	params->dsi.PLL_CLOCK                = 262;
	params->dsi.esd_check_enable = 1;
	params->dsi.customization_esd_check_enable = 1;
	params->dsi.lcm_esd_check_table[0].cmd          = 0x0a;
	params->dsi.lcm_esd_check_table[0].count        = 1;
	params->dsi.lcm_esd_check_table[0].para_list[0] = 0x9d;
}

static void lcm_set_gpio_output(unsigned int GPIO, unsigned int output)
{
	mt_set_gpio_mode(GPIO, GPIO_MODE_00);
	mt_set_gpio_dir(GPIO, GPIO_DIR_OUT);
	mt_set_gpio_out(GPIO, (output>0)? GPIO_OUT_ONE: GPIO_OUT_ZERO);
}

static unsigned int lcm_compare_id(void)
{
    int ret = 0;
    #define LCM_ID 0x83102d
    int array[4];
   unsigned char buffer[2],buffer1[2],buffer2[2];
    int id = 0;
   	ret = PMU_REG_MASK(0xB2, (0x3 << 6), (0x3 << 6));
	ret = PMU_REG_MASK(0xB3, 28, (0x3F << 0));
	ret = PMU_REG_MASK(0xB4, 28, (0x3F << 0));
	ret = PMU_REG_MASK(0xB1, (1<<3) | (1<<6), (1<<3) | (1<<6));
	MDELAY(10);
  lcm_set_gpio_output(GPIO45,1);  //lcd reset 1
	MDELAY(10);//100
	lcm_set_gpio_output(GPIO45,0);  //lcd reset 0
	MDELAY(10);//100
	lcm_set_gpio_output(GPIO45,1);  //lcd reset 1
    MDELAY(120);
    
	 array[0] = 0x00023700; // read id return two byte,version and id
	 dsi_set_cmdq(array, 1, 1);
	 read_reg_v2(0xDA, buffer, 1);
		
	 array[0] = 0x00023700; // read id return two byte,version and id
	 dsi_set_cmdq(array, 1, 1);
	 read_reg_v2(0xDB, buffer1, 1);
		
	 array[0] = 0x00023700; // read id return two byte,version and id
	 dsi_set_cmdq(array, 1, 1);
	 read_reg_v2(0xDC, buffer2, 1);
    id = (buffer[0] << 16) | (buffer1[0] << 8) | buffer2[0]; 
#ifdef BUILD_LK
    printf("lsw: hx83102d %s %d, id = %x id1=%x, id2=%x\n", __func__,__LINE__, buffer[0],buffer1[0],buffer2[0]);
#else
    printk("lsw: hx83102d %s %d, id = %x id1=%x, id2=%x\n", __func__,__LINE__, buffer[0],buffer1[0],buffer2[0]);
#endif
    if ((id == LCM_ID) || (id == 0x84102d)){
         return 1;
    }
    else 
         return 0;
}

/*
#define AUXADC_LCM_VOLTAGE_CHANNEL (2)
#define MIN_VOLTAGE (900000)
#define MAX_VOLTAGE (1100000)
extern int IMM_GetOneChannelValue_Cali(int Channel, int *voltage);
static unsigned int hx_lcm_compare_id(void)
{
    int res = 0;
    int lcm_vol = 0;
#ifdef AUXADC_LCM_VOLTAGE_CHANNEL
    res = IMM_GetOneChannelValue_Cali(AUXADC_LCM_VOLTAGE_CHANNEL,&lcm_vol);
    if(res < 0)
    {
#ifdef BUILD_LK
        dprintf(0,"lixf lcd [adc_uboot]: get data error\n");
#endif
        return 0;
    }
#endif
#ifdef BUILD_LK
    dprintf(0,"lsw lk: lcm_vol= %d , file : %s, line : %d\n",lcm_vol, __FILE__, __LINE__);
#else
    printk("lsw kernel: lcm_vol= %d , file : %s, line : %d\n",lcm_vol, __FILE__, __LINE__);
#endif
    if (lcm_vol >= MIN_VOLTAGE && lcm_vol <= MAX_VOLTAGE)
    {
        return 1;
    }


    return 0;
}

*/

static void lcm_init(void)
{
	int ret = 0;
	lcm_set_gpio_output(GPIO45,0);  //lcd reset 0
	ret = PMU_REG_MASK(0xB2, (0x3 << 6), (0x3 << 6));
	ret = PMU_REG_MASK(0xB3, 28, (0x3F << 0));
	ret = PMU_REG_MASK(0xB4, 28, (0x3F << 0));
	ret = PMU_REG_MASK(0xB1, (1<<3) | (1<<6), (1<<3) | (1<<6));
	MDELAY(20);

   lcm_set_gpio_output(GPIO45,1);  //lcd reset 1
	MDELAY(10);//100
	lcm_set_gpio_output(GPIO45,0);  //lcd reset 0
	MDELAY(10);//100
	lcm_set_gpio_output(GPIO45,1);  //lcd reset 1
    MDELAY(10);
    push_table(lcm_initialization_setting, sizeof(lcm_initialization_setting) / sizeof(struct LCM_setting_table), 1);
}

static void lcm_suspend(void)
{
	int ret = 0;
    push_table(lcm_suspend_setting, sizeof(lcm_suspend_setting) / sizeof(struct LCM_setting_table), 1);

    
    MDELAY(5);
	ret = PMU_REG_MASK(0xB1, (0<<3) | (0<<6), (1<<3) | (1<<6));
	lcm_set_gpio_output(GPIO45,0);  //lcd reset 0
	MDELAY(5);
}

static void lcm_resume(void)
{
    lcm_init();
}

LCM_DRIVER hx83102d_hdp_dsi_vdo_boe_drip_incell_lcm_drv =
{
    .name			= "hx83102d_hdp_dsi_vdo_boe_drip_incell",
#if defined(CONFIG_PRIZE_HARDWARE_INFO) && !defined (BUILD_LK)
	.lcm_info = {
		.chip	= "hx83102d",
		.vendor	= "focaltech",
		.id		= "0x83102d",
		.more	= "720*1560",
	},
#endif
    .set_util_funcs = lcm_set_util_funcs,
    .get_params     = lcm_get_params,
    .init           = lcm_init,
    .suspend        = lcm_suspend,
    .resume         = lcm_resume,
    .compare_id     = lcm_compare_id,
};
