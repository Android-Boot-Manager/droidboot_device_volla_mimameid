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

/*prize-add-for round corner-houjian-20180918-start*/
#ifdef MTK_ROUND_CORNER_SUPPORT
#include "data_rgba4444_roundedpattern.h"
#endif
/*prize-add-for round corner-houjian-20180918-end*/

#ifdef BUILD_LK
#define LCM_LOGI(string, args...)  dprintf(0, "[LK/"LOG_TAG"]"string, ##args)
#define LCM_LOGD(string, args...)  dprintf(1, "[LK/"LOG_TAG"]"string, ##args)
#else
#define LCM_LOGI(fmt, args...)  pr_notice("[KERNEL/"LOG_TAG"]"fmt, ##args)
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

static const unsigned char LCD_MODULE_ID = 0x01;
/* --------------------------------------------------------------------------- */
/* Local Constants */
/* --------------------------------------------------------------------------- */
#define LCM_DSI_CMD_MODE	0
#define FRAME_WIDTH  										1080
#define FRAME_HEIGHT 										2400
//#define USE_LDO
#define GPIO_TP_RST (GPIO20 | 0x80000000)
#define LCM_PHYSICAL_WIDTH                  				(69500)
#define LCM_PHYSICAL_HEIGHT                  				(154440)

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

struct LCM_setting_table {
	unsigned int cmd;
	unsigned char count;
	unsigned char para_list[64];
};

static struct LCM_setting_table lcm_initialization_setting[] = {
#if 0
{0x00,1,{0x00}},
{0xFF,3,{0x87,0x56,0x01}},
{0x00,1,{0x80}},
{0xFF,2,{0x87,0x56}},
{0x00,1,{0xA1}},
{0xB3,6,{0x04,0x38,0x09,0x60,0x00,0xFC}},
{0x00,1,{0x80}},
{0xC0,6,{0x00,0x8D,0x00,0x08,0x00,0x24}},
{0x00,1,{0x90}},
{0xC0,6,{0x00,0x8D,0x00,0x08,0x00,0x24}},
{0x00,1,{0xA0}},
{0xC0,6,{0x01,0x18,0x00,0x08,0x00,0x24}},
{0x00,1,{0xB0}},
{0xC0,5,{0x00,0x8D,0x00,0x08,0x24}},
{0x00,1,{0xC1}},
{0xC0,8,{0x00,0xD0,0x00,0x9F,0x00,0x8B,0x00,0xEE}},
{0x00,1,{0xD7}},
{0xC0,6,{0x00,0x8B,0x00,0x08,0x00,0x24}},
{0x00,1,{0xA3}},
{0xC1,6,{0x00,0x2C,0x00,0x2C,0x00,0x02}},
{0x00,1,{0x80}},
{0xCE,16,{0x01,0x81,0x09,0x13,0x00,0xD0,0x00,0xE8,0x00,0x8A,0x00,0x9A,0x00,0x68,0x00,0x74}},
{0x00,1,{0x90}},
{0xCE,15,{0x00,0x8E,0x0C,0xDF,0x00,0x8E,0x80,0x09,0x13,0x00,0x04,0x00,0x21,0x1F,0x16}},
{0x00,1,{0xA0}},
{0xCE,3,{0x00,0x00,0x00}},
{0x00,1,{0xB0}},
{0xCE,3,{0x22,0x00,0x00}},
{0x00,1,{0xD1}},
{0xCE,7,{0x00,0x00,0x01,0x00,0x00,0x00,0x00}},
{0x00,1,{0xE1}},
{0xCE,11,{0x08,0x02,0x4D,0x02,0x4D,0x02,0x4D,0x00,0x00,0x00,0x00}},
{0x00,1,{0xF1}},
{0xCE,9,{0x13,0x09,0x0D,0x00,0xF6,0x01,0x96,0x00,0xC2}},
{0x00,1,{0xB0}},
{0xCF,4,{0x00,0x00,0xBF,0xC3}},
{0x00,1,{0xB5}},
{0xCF,4,{0x04,0x04,0xE4,0xE8}},
{0x00,1,{0xC0}},
{0xCF,4,{0x09,0x09,0x2C,0x30}},
{0x00,1,{0xC5}},
{0xCF,4,{0x00,0x00,0x08,0x0C}},
{0x00,1,{0xE8}},
{0xC0,1,{0x40}},
{0x00,1,{0x80}},
{0xC2,8,{0x84,0x00,0x01,0x8D,0x83,0x00,0x01,0x8D}},
{0x00,1,{0xA0}},
{0xC2,15,{0x82,0x04,0x00,0x02,0x8C,0x81,0x04,0x00,0x02,0x8C,0x00,0x04,0x00,0x02,0x8C}},
{0x00,1,{0xB0}},
{0xC2,15,{0x01,0x04,0x00,0x02,0x8C,0x02,0x04,0x00,0x02,0x8C,0x03,0x04,0x00,0x02,0x8C}},
{0x00,1,{0xC0}},
{0xC2,10,{0x04,0x04,0x00,0x02,0x8C,0x05,0x04,0x00,0x02,0x8C}},
{0x00,1,{0xE0}},
{0xC2,4,{0x77,0x77,0x77,0x77}},
{0x00,1,{0xC0}},
{0xC3,4,{0x99,0x99,0x99,0x99}},
{0x00,1,{0x80}},
{0xCB,16,{0x00,0xC5,0x00,0x00,0x05,0x05,0x00,0x05,0x0A,0x05,0xC5,0x00,0x05,0x05,0x00,0xC0}},
{0x00,1,{0x90}},
{0xCB,16,{0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00}},
{0x00,1,{0xA0}},
{0xCB,4,{0x00,0x00,0x00,0x00}},
{0x00,1,{0xB0}},
{0xCB,4,{0x10,0x51,0x94,0x50}},
{0x00,1,{0xC0}},
{0xCB,4,{0x10,0x51,0x94,0x50}},
{0x00,1,{0x80}},
{0xCC,16,{0x00,0x00,0x00,0x00,0x07,0x09,0x0B,0x0D,0x25,0x25,0x03,0x00,0x22,0x00,0x24,0x00}},
{0x00,1,{0x90}},
{0xCC,8,{0x29,0x16,0x17,0x18,0x19,0x1A,0x1B,0x26}},
{0x00,1,{0x80}},
{0xCD,16,{0x00,0x00,0x00,0x00,0x06,0x08,0x0A,0x0C,0x25,0x00,0x02,0x00,0x22,0x00,0x24,0x00}},
{0x00,1,{0x90}},
{0xCD,8,{0x29,0x16,0x17,0x18,0x19,0x1A,0x1B,0x26}},
{0x00,1,{0xA0}},
{0xCC,16,{0x00,0x00,0x00,0x00,0x08,0x06,0x0C,0x0A,0x25,0x25,0x02,0x00,0x24,0x00,0x23,0x00}},
{0x00,1,{0xB0}},
{0xCC,8,{0x29,0x16,0x17,0x18,0x19,0x1A,0x1B,0x26}},
{0x00,1,{0xA0}},
{0xCD,16,{0x00,0x00,0x00,0x00,0x09,0x07,0x0D,0x0B,0x25,0x00,0x02,0x00,0x24,0x00,0x23,0x00}},
{0x00,1,{0xB0}},
{0xCD,8,{0x29,0x16,0x17,0x18,0x19,0x1A,0x1B,0x26}},
{0x00,1,{0x80}},
{0xA7,1,{0x13}},
{0x00,1,{0x82}},
{0xA7,2,{0x22,0x02}},
{0x00,1,{0x85}},
{0xC4,1,{0x1C}},
{0x00,1,{0x97}},
{0xC4,1,{0x01}},
{0x00,1,{0xA0}},
{0xC4,3,{0x8D,0xD8,0x8D}},
{0x00,1,{0x93}},
{0xC5,1,{0x37}},
{0x00,1,{0x97}},
{0xC5,1,{0x37}},
{0x00,1,{0xB6}},
{0xC5,2,{0x23,0x23}},
{0x00,1,{0x9B}},
{0xF5,1,{0x4B}},
{0x00,1,{0x93}},
{0xF5,2,{0x00,0x00}},
{0x00,1,{0x9D}},
{0xF5,1,{0x49}},
{0x00,1,{0x82}},
{0xF5,2,{0x00,0x00}},
{0x00,1,{0x8C}},
{0xC3,3,{0x00,0x00,0x00}},
{0x00,1,{0x84}},
{0xC5,2,{0x28,0x28}},
{0x00,1,{0xA4}},
{0xD7,1,{0x00}},
{0x00,1,{0x80}},
{0xF5,2,{0x59,0x59}},
{0x00,1,{0x84}},
{0xF5,3,{0x59,0x59,0x59}},
{0x00,1,{0x96}},
{0xF5,1,{0x59}},
{0x00,1,{0xA6}},
{0xF5,1,{0x59}},
{0x00,1,{0xCA}},
{0xC0,1,{0x80}},
{0x00,1,{0xB1}},
{0xF5,1,{0x1F}},
{0x00,1,{0x00}},
{0xD8,2,{0x2B,0x2B}},
{0x00,1,{0x00}},
{0xD9,2,{0x1E,0x1E}},
{0x00,1,{0x86}},
{0xC0,6,{0x01,0x03,0x01,0x01,0x10,0x03}},
{0x00,1,{0x96}},
{0xC0,6,{0x01,0x03,0x01,0x01,0x10,0x03}},
{0x00,1,{0xA6}},
{0xC0,6,{0x01,0x03,0x01,0x01,0x10,0x03}},
{0x00,1,{0xE9}},
{0xC0,6,{0x01,0x03,0x01,0x01,0x10,0x03}},
{0x00,1,{0xA3}},
{0xCE,6,{0x01,0x03,0x01,0x01,0x10,0x03}},
{0x00,1,{0xB3}},
{0xCE,6,{0x01,0x03,0x01,0x01,0x10,0x03}},

//Gamma2.5
{0x00,1,{0x00}},
{0xE1,40,{0x05,0x05,0x06,0x0A,0x69,0x12,0x1A,0x1F,0x29,0xCF,0x31,0x38,0x3E,0x43,0xEB,0x48,0x51,0x58,0x5F,0x85,0x66,0x6D,0x74,0x7C,0x06,0x86,0x8B,0x91,0x99,0x3C,0xA1,0xAC,0xBB,0xC4,0xA0,0xD0,0xE4,0xF3,0xFF,0x8F}},
{0x00,1,{0x00}},
{0xE2,40,{0x05,0x05,0x06,0x0A,0x69,0x12,0x1A,0x1F,0x29,0xCF,0x31,0x38,0x3E,0x43,0xEB,0x48,0x51,0x58,0x5F,0x85,0x66,0x6D,0x74,0x7C,0x06,0x86,0x8B,0x91,0x99,0x3C,0xA1,0xAC,0xBB,0xC4,0xA0,0xD0,0xE4,0xF3,0xFF,0x8F}},
{0x00,1,{0x00}},
{0xE3,40,{0x05,0x05,0x06,0x0A,0x69,0x12,0x1A,0x1F,0x29,0xCF,0x31,0x38,0x3E,0x43,0xEB,0x48,0x51,0x58,0x5F,0x85,0x66,0x6D,0x74,0x7C,0x06,0x86,0x8B,0x91,0x99,0x3C,0xA1,0xAC,0xBB,0xC4,0xA0,0xD0,0xE4,0xF3,0xFF,0x8F}},
{0x00,1,{0x00}},
{0xE4,40,{0x05,0x05,0x06,0x0A,0x69,0x12,0x1A,0x1F,0x29,0xCF,0x31,0x38,0x3E,0x43,0xEB,0x48,0x51,0x58,0x5F,0x85,0x66,0x6D,0x74,0x7C,0x06,0x86,0x8B,0x91,0x99,0x3C,0xA1,0xAC,0xBB,0xC4,0xA0,0xD0,0xE4,0xF3,0xFF,0x8F}},
{0x00,1,{0x00}},
{0xE5,40,{0x05,0x05,0x06,0x0A,0x69,0x12,0x1A,0x1F,0x29,0xCF,0x31,0x38,0x3E,0x43,0xEB,0x48,0x51,0x58,0x5F,0x85,0x66,0x6D,0x74,0x7C,0x06,0x86,0x8B,0x91,0x99,0x3C,0xA1,0xAC,0xBB,0xC4,0xA0,0xD0,0xE4,0xF3,0xFF,0x8F}},
{0x00,1,{0x00}},
{0xE6,40,{0x05,0x05,0x06,0x0A,0x69,0x12,0x1A,0x1F,0x29,0xCF,0x31,0x38,0x3E,0x43,0xEB,0x48,0x51,0x58,0x5F,0x85,0x66,0x6D,0x74,0x7C,0x06,0x86,0x8B,0x91,0x99,0x3C,0xA1,0xAC,0xBB,0xC4,0xA0,0xD0,0xE4,0xF3,0xFF,0x8F}},

{0x00,1,{0xCC}},
{0xC0,1,{0x13}},
{0x00,1,{0x82}},
{0xC5,1,{0x3A}},
{0x00,1,{0x00}},
{0xFF,3,{0xFF,0xFF,0xFF}},
{0x35,1,{0x00}},
//{0x1C,1,{0x10}},//720*1600
#endif

{0XFF,3,{0X78,0X07,0X01}},
{0X00,1,{0X63}},
{0X01,1,{0XE2}},
{0X02,1,{0X34}},  		//20191023
{0X03,1,{0X1f}},  		//20191023
{0X04,1,{0X23}},	
{0X05,1,{0X42}},	
{0X06,1,{0X00}},  		//20200304
{0X07,1,{0X00}}, 		//20191023
{0X08,1,{0XA3}},
{0X09,1,{0X04}},
{0X0A,1,{0X30}},
{0X0B,1,{0X01}},
{0X0C,1,{0X00}},
{0X0E,1,{0X5A}},


{0X31,1,{0X07}}, 		//FGOUTR01
{0X32,1,{0X07}},     	//FGOUTR02
{0X33,1,{0X07}},     	//FGOUTR03
{0X34,1,{0X3D}},    	//FGOUTR04    TP_SW
{0X35,1,{0X3D}},    	//FGOUTR05    TP_SW
{0X36,1,{0X02}}, 		//FGOUTR06    CT_SW / CT_CTRL
{0X37,1,{0X2E}},    	//FGOUTR07    MUXR1
{0X38,1,{0X2F}},     	//FGOUTR08    MUXG1
{0X39,1,{0X30}},     	//FGOUTR09    MUXB1
{0X3A,1,{0X31}},    	//FGOUTR10    MUXR2
{0X3B,1,{0X32}},    	//FGOUTR11    MUXG2
{0X3C,1,{0X33}},    	//FGOUTR12    MUXB2
{0X3D,1,{0X01}},    	//FGOUTR13    U2D
{0X3E,1,{0X00}},    	//FGOUTR14    D2U
{0X3F,1,{0X11}},     	//FGOUTR15    CLK_O
{0X40,1,{0X11}},     	//FGOUTR16    CLK_O
{0X41,1,{0X13}},     	//FGOUTR17    XCLK_O
{0X42,1,{0X13}},     	//FGOUTR18    XCLK_O
{0X43,1,{0X40}},    	//FGOUTR19    GOFF
{0X44,1,{0X08}},     	//FGOUTR20    STV
{0X45,1,{0X0C}},    	//FGOUTR21    VEND
{0X46,1,{0X2C}},    	//FGOUTR22    RST
{0X47,1,{0X28}},     	//FGOUTR23    XDONB
{0X48,1,{0X28}},     	//FGOUTR24    XDONB

{0X49,1,{0X07}}, 		//FGOUTL01
{0X4A,1,{0X07}},    	//FGOUTL02
{0X4B,1,{0X07}},    	//FGOUTL03
{0X4C,1,{0X3D}},   	//FGOUTL04    TP_SW
{0X4D,1,{0X3D}},   	//FGOUTL05    TP_SW
{0X4E,1,{0X02}},  		//FGOUTL06    CT_SW / CT_CTRL
{0X4F,1,{0X2E}},    	//FGOUTL07    MUXR1
{0X50,1,{0X2F}},     	//FGOUTL08    MUXG1
{0X51,1,{0X30}},     	//FGOUTL09    MUXB1
{0X52,1,{0X31}},     	//FGOUTL10    MUXR2
{0X53,1,{0X32}},     	//FGOUTL11    MUXG2
{0X54,1,{0X33}},     	//FGOUTL12    MUXB2
{0X55,1,{0X01}},     	//FGOUTL13    U2D
{0X56,1,{0X00}},     	//FGOUTL14    D2U
{0X57,1,{0X10}},     	//FGOUTL15    CLK_O
{0X58,1,{0X10}},     	//FGOUTL16    CLK_O
{0X59,1,{0X12}},     	//FGOUTL17    XCLK_O
{0X5A,1,{0X12}},    	//FGOUTL18    XCLK_O
{0X5B,1,{0X40}},   	//FGOUTL19    GOFF
{0X5C,1,{0X08}},    	//FGOUTL20    STV
{0X5D,1,{0X0C}},   	//FGOUTL21    VEND
{0X5E,1,{0X2C}},    	//FGOUTL22    RST
{0X5F,1,{0X28}},     	//FGOUTL23    XDONB
{0X60,1,{0X28}},     	//FGOUTL24    XDONB

{0X61,1,{0X07}}, 		//BGOUTR01
{0X62,1,{0X07}},     	//BGOUTR02
{0X63,1,{0X07}},     	//BGOUTR03
{0X64,1,{0X3D}},    	//BGOUTR04   TP_SW
{0X65,1,{0X3D}},    	//BGOUTR05   TP_SW
{0X66,1,{0X28}},     	//BGOUTR06   CT_SW
{0X67,1,{0X2E}},    	//BGOUTR07   MUXR1
{0X68,1,{0X2F}},     	//BGOUTR08   MUXG1
{0X69,1,{0X30}},     	//BGOUTR09   MUXB1
{0X6A,1,{0X31}},    	//BGOUTR10   MUXR2
{0X6B,1,{0X32}},    	//BGOUTR11   MUXG2
{0X6C,1,{0X33}},    	//BGOUTR12   MUXB2
{0X6D,1,{0X01}},    	//BGOUTR13   U2D
{0X6E,1,{0X00}},    	//BGOUTR14   D2U
{0X6F,1,{0X12}},     	//BGOUTR15   XCLK_O
{0X70,1,{0X12}},     	//BGOUTR16   XCLK_O
{0X71,1,{0X10}},     	//BGOUTR17   CLK_O
{0X72,1,{0X10}},     	//BGOUTR18   CLK_O
{0X73,1,{0X3C}},    	//BGOUTR19   GOFF
{0X74,1,{0X0C}},    	//BGOUTR20   VEND
{0X75,1,{0X08}},     	//BGOUTR21   STV
{0X76,1,{0X2C}},    	//BGOUTR22   RST
{0X77,1,{0X28}},     	//BGOUTR23   XDONB
{0X78,1,{0X28}},     	//BGOUTR24   XDONB

{0X79,1,{0X07}},     	//BGOUTL01
{0X7A,1,{0X07}},    	//BGOUTL02
{0X7B,1,{0X07}},    	//BGOUTL03
{0X7C,1,{0X3D}},   	//BGOUTL04   TP_SW
{0X7D,1,{0X3D}},   	//BGOUTL05   TP_SW
{0X7E,1,{0X28}},    	//BGOUTL06   CT_SW
{0X7F,1,{0X2E}},    	//BGOUTL07   MUXR1
{0X80,1,{0X2F}},     	//BGOUTL08   MUXG1
{0X81,1,{0X30}},     	//BGOUTL09   MUXB1
{0X82,1,{0X31}},     	//BGOUTL10   MUXR2
{0X83,1,{0X32}},     	//BGOUTL11   MUXG2
{0X84,1,{0X33}},     	//BGOUTL12   MUXB2
{0X85,1,{0X01}},     	//BGOUTL13   U2D
{0X86,1,{0X00}},     	//BGOUTL14   D2U
{0X87,1,{0X13}},     	//BGOUTL15   XCLK_O
{0X88,1,{0X13}},     	//BGOUTL16   XCLK_O
{0X89,1,{0X11}},     	//BGOUTL17   CLK_O
{0X8A,1,{0X11}},    	//BGOUTL18   CLK_O
{0X8B,1,{0X3C}},   	//BGOUTL19   GOFF
{0X8C,1,{0X0C}},   	//BGOUTL20   VEND
{0X8D,1,{0X08}},    	//BGOUTL21   STV
{0X8E,1,{0X2C}},    	//BGOUTL22   RST
{0X8F,1,{0X28}},     	//BGOUTL23   XDONB
{0X90,1,{0X28}},     	//BGOUTL24   XDONB

{0X91,1,{0XE1}},
{0X92,1,{0X19}},
{0X93,1,{0X08}},
{0X94,1,{0X00}},
{0X95,1,{0X21}},
{0X96,1,{0X19}},
{0X97,1,{0X08}},
{0X98,1,{0X00}},
{0XA0,1,{0X83}},
{0XA1,1,{0X44}},
{0XA2,1,{0X83}},
{0XA3,1,{0X44}},
{0XA4,1,{0X61}},
{0XA5,1,{0X00}},  		//20200304
{0XA6,1,{0X15}},
{0XA7,1,{0X50}},
{0XA8,1,{0X1A}},
{0XAE,1,{0X00}},
{0XB0,1,{0X00}},
{0XB1,1,{0X00}},
{0XB2,1,{0X02}},
{0XB3,1,{0X00}},
{0XB4,1,{0X02}},
{0XC1,1,{0X60}},
{0XC2,1,{0X60}},
{0XC5,1,{0X29}},
{0XC6,1,{0X20}},
{0XC7,1,{0X20}},
{0XC8,1,{0X1F}},
{0XC9,1,{0X00}}, 		//20191023
{0XCA,1,{0X01}},
{0XD1,1,{0X11}},
{0XD2,1,{0X00}},
{0XD3,1,{0X01}},
{0XD4,1,{0X00}},
{0XD5,1,{0X00}},
{0XD6,1,{0X3D}},
{0XD7,1,{0X00}},
{0XD8,1,{0X01}},
{0XD9,1,{0X54}},
{0XDA,1,{0X00}},
{0XDB,1,{0X00}},
{0XDC,1,{0X00}},
{0XDD,1,{0X00}},
{0XDE,1,{0X00}},
{0XDF,1,{0X00}},
{0XE0,1,{0X00}},
{0XE1,1,{0X04}},		//20191025
{0XE2,1,{0X00}},
{0XE3,1,{0X1B}},
{0XE4,1,{0X52}},
{0XE5,1,{0X4B}},                   //APO
{0XE6,1,{0X44}}, 		//20191025
{0XE7,1,{0X00}},
{0XE8,1,{0X01}},
{0XED,1,{0X55}},
{0XEF,1,{0X30}},
{0XF0,1,{0X00}},
{0XF4,1,{0X54}},






// SRC PART
{0XFF,3,{0X78,0X07,0X02}},
{0X01,1,{0X7D}}, 		//20191023
{0X02,1,{0X08}}, 		//mipi time out 256us
{0X06,1,{0X6B}},		//BIST FRAME 60 hz
{0X08,1,{0X00}},		//BIST FRAME //20191025
{0X0E,1,{0X14}},
{0X0F,1,{0X34}},




{0X40,1,{0X0F}},		//t8_de 0.5us
{0X41,1,{0X00}},
{0X42,1,{0X09}},
{0X43,1,{0X12}},		//ckh width 500ns
{0X46,1,{0X32}},
{0X4D,1,{0X02}},		//RRGGBB
{0X47,1,{0X00}},		//CKH connect
{0X53,1,{0X09}},
{0X54,1,{0X05}},
{0X56,1,{0X02}},
{0X5D,1,{0X07}},
{0X5E,1,{0XC0}},
{0X80,1,{0X3F}},            //source 推力
//{0X76,1,{0XFF}},           //buffer 推力
{0XF4,1,{0X00}},
{0XF5,1,{0X00}},
{0XF6,1,{0X00}},
{0XF7,1,{0X00}},



{0XFF,3,{0X78,0X07,0X04}},
{0XB7,1,{0X45}}, 		//


{0XFF,3,{0X78,0X07,0X05}},   // SWITCH TO PAGE 5 //REGULATOR
{0X5B,1,{0X7E}},		//GVDDP +5V
{0X5C,1,{0X7E}},		//GVDDP -5V
{0X3C,1,{0X00}},		//FOLLOW ABNORMAL SEQ 01:TM SLP IN SEQ
{0X52,1,{0X60}}, 		//VGH 9.5V
{0X56,1,{0X5B}},		//VGHO 8.5V
{0X5A,1,{0X54}},		//VGLO -8.15V
{0X54,1,{0X56}}, 		//VGL -9V
{0X02,1,{0X00}}, 		//VCOM -0.2V
{0X03,1,{0X87}},	        //VCOM设定，OTP 代码不在initial code部分烧录
{0X44,1,{0XEF}},		//VGH X2  //FF VGH X3
{0X4E,1,{0X3F}},		//VGL X2  //7F VGL X3
{0X20,1,{0X13}}, 		//20191023
{0X2A,1,{0X13}},		//20191023
{0X2B,1,{0X08}},	
{0X23,1,{0XF7}}, 		//20191025
{0X2D,1,{0X54}},		//20191025 VGHO
{0X2E,1,{0X74}},		//20191025 VGLO
{0XB5,1,{0X54}},		//20191025
{0XB7,1,{0X74}},		//20191025


{0XFF,3,{0X78,0X07,0X06}},
{0X0A,1,{0X0C}},		//NL_SEL = 1 NL[10:8] = 4  2+(2NL) = 2400
{0X0B,1,{0XAF}}, 		//NL[7:0]  = 91
{0X0E,1,{0X06}}, 		//SS
//{0X3E,1,{0XE2}}, 		//DONT reload
//{0XD6,1,{0X55}}, 	
//{0XCD,1,{0X68}},
{0X83,1,{0X00}}, 		//mipi err off
{0X84,1,{0X00}},		//mipi err off
//{0XC7,1,{0X05}},		//1BIT esd with 0A

{0XFF,3,{0X78,0X07,0X08}},
{0XE0,40,{0X00,0X00,0X1A,0X44,0X00,0X67,0X81,0X9A,0X00,0XB0,0XC3,0XD5,0X15,0X0E,0X3B,0X7E,0X25,0XAE,0XFA,0X35,0X2A,0X36,0X6E,0XAF,0X3E,0XD7,0X0A,0X30,0X3F,0X55,0X63,0X71,0X3F,0X7E,0X91,0XA2,0X3F,0XBD,0XD8,0XD9}},
{0XE1,40,{0X00,0X00,0X1A,0X44,0X00,0X67,0X81,0X9A,0X00,0XB0,0XC3,0XD5,0X15,0X0E,0X3B,0X7E,0X25,0XAE,0XFA,0X35,0X2A,0X36,0X6E,0XAF,0X3E,0XD7,0X0A,0X30,0X3F,0X55,0X63,0X71,0X3F,0X7E,0X91,0XA2,0X3F,0XBD,0XD8,0XD9}},



//************AUTO TRIM********************
{0XFF,3,{0X78,0X07,0X0B}},
{0X94,1,{0X88}},
{0X95,1,{0X22}},
{0X96,1,{0X06}},
{0X97,1,{0X06}},
{0X98,1,{0XCB}},
{0X99,1,{0XCB}},
{0X9A,1,{0X06}},
{0X9B,1,{0XCD}},
{0X9C,1,{0X05}},
{0X9D,1,{0X05}},
{0X9E,1,{0XAA}},
{0X9F,1,{0XAA}},
{0XAB,1,{0XF0}},

//************LONG H TIMING SETTING********************
{0XFF,3,{0X78,0X07,0X0E}},
{0X00,1,{0XA3}}, 	//LH MODE
{0X02,1,{0X12}}, 	//VBP
{0X40,1,{0X07}}, 	//8 UNIT
{0X49,1,{0X2C}}, 	//UNIT LINE[7:0] = 300
{0X47,1,{0X90}}, 	//UNIT LINE[7:0] = 300
{0X45,1,{0X0A}}, 	//TP TERM2_UNIT 0 = 170 US 
{0X46,1,{0XA1}},
{0X4D,1,{0XC4}},	//RTN = 6.15US
{0X51,1,{0X00}},
 
{0XB0,1,{0X01}},	//term1 number  
{0XB1,1,{0X60}},
{0XB3,1,{0X00}},
{0XB4,1,{0X33}},

{0XBC,1,{0X04}},
{0XBD,1,{0XFC}},

{0XC0,1,{0X63}}, //term3 number  
{0XC7,1,{0X61}},
{0XC8,1,{0X61}},
{0XC9,1,{0X61}},
{0XD2,1,{0X00}},
{0XD3,1,{0XA8}},
{0XD4,1,{0X77}},
{0XD5,1,{0XA8}},
{0XD6,1,{0X00}},
{0XD7,1,{0XA8}},
{0XD8,1,{0X00}},
{0XD9,1,{0XA8}},

{0XE0,1,{0X00}},
{0XE1,1,{0X00}},
{0XE2,1,{0X09}},
{0XE3,1,{0X17}},

{0XE4,1,{0X04}},
{0XE5,1,{0X04}},
{0XE6,1,{0X00}},
{0XE7,1,{0X05}},
{0XE8,1,{0X00}},
{0XE9,1,{0X02}},
{0XEA,1,{0X07}},

{0X07,1,{0X21}},  //GOFF shift
{0X4B,1,{0X14}},


//***********TP modulation  ***************************
{0XFF,3,{0X78,0X07,0X0C}},
{0X00,1,{0X12}},
{0X01,1,{0X26}},
{0X02,1,{0X11}},
{0X03,1,{0X1B}},
{0X04,1,{0X11}},
{0X05,1,{0X18}},
{0X06,1,{0X11}},
{0X07,1,{0X18}},
{0X08,1,{0X12}},
{0X09,1,{0X27}},
{0X0A,1,{0X11}},
{0X0B,1,{0X14}},
{0X0C,1,{0X11}},
{0X0D,1,{0X17}},
{0X0E,1,{0X12}},
{0X0F,1,{0X24}},
{0X10,1,{0X12}},
{0X11,1,{0X28}},
{0X12,1,{0X12}},
{0X13,1,{0X23}},
{0X14,1,{0X11}},
{0X15,1,{0X1E}},
{0X16,1,{0X11}},
{0X17,1,{0X1C}},
{0X18,1,{0X11}},
{0X19,1,{0X20}},
{0X1A,1,{0X11}},
{0X1B,1,{0X16}},
{0X1C,1,{0X11}},
{0X1D,1,{0X15}},
{0X1E,1,{0X11}},
{0X1F,1,{0X19}},
{0X20,1,{0X12}},
{0X21,1,{0X21}},
{0X22,1,{0X11}},
{0X23,1,{0X1F}},
{0X24,1,{0X11}},
{0X25,1,{0X1A}},
{0X26,1,{0X11}},
{0X27,1,{0X1D}},
{0X28,1,{0X12}},
{0X29,1,{0X25}},


{0XFF,3,{0X78,0X07,0X00}},

{0x11,0,{}},
{REGFLAG_DELAY,120, {}},
{0x29,0,{}},
{REGFLAG_DELAY,30, {}},
{0x35,1,{0x00}},

	{REGFLAG_END_OF_TABLE, 0x00, {}}
};

static struct LCM_setting_table lcm_suspend_setting[] = {
	{0x28, 0, {}},
	{REGFLAG_DELAY, 30, {} },
	{0x10, 0, {}},
	{REGFLAG_DELAY, 120, {} },
	{0x00, 1, {0x00}},
	{0xF7, 4, {0x5A,0xA5,0x95,0x27}},

	{REGFLAG_END_OF_TABLE, 0x00, {}}  
};

static void push_table(struct LCM_setting_table *table, unsigned int count,
		       unsigned char force_update)
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
	
	params->type = LCM_TYPE_DSI;
	
	params->width = FRAME_WIDTH;
	params->height = FRAME_HEIGHT;
	
	// enable tearing-free
	params->dbi.te_mode 				= LCM_DBI_TE_MODE_VSYNC_ONLY;
	params->dbi.te_edge_polarity		= LCM_POLARITY_RISING;
	#if (LCM_DSI_CMD_MODE)
	params->dsi.mode   = CMD_MODE;
	params->dsi.switch_mode = SYNC_PULSE_VDO_MODE;
	#else
	params->dsi.mode   = SYNC_PULSE_VDO_MODE;//SYNC_EVENT_VDO_MODE;//BURST_VDO_MODE;////
	#endif
		
	// DSI
	/* Command mode setting */
	//1 Three lane or Four lane
	params->dsi.LANE_NUM				= LCM_FOUR_LANE;
	//The following defined the fomat for data coming from LCD engine.
	params->dsi.data_format.color_order = LCM_COLOR_ORDER_RGB;
	params->dsi.data_format.trans_seq	= LCM_DSI_TRANS_SEQ_MSB_FIRST;
	params->dsi.data_format.padding 	= LCM_DSI_PADDING_ON_LSB;
	params->dsi.data_format.format		= LCM_DSI_FORMAT_RGB888;
		
		
	params->dsi.PS=LCM_PACKED_PS_24BIT_RGB888;
		
	#if (LCM_DSI_CMD_MODE)
	params->dsi.intermediat_buffer_num = 0;//because DSI/DPI HW design change, this parameters should be 0 when video mode in MT658X; or memory leakage
	params->dsi.word_count=FRAME_WIDTH*3;	//DSI CMD mode need set these two bellow params, different to 6577
	#else
	params->dsi.intermediat_buffer_num = 0; //because DSI/DPI HW design change, this parameters should be 0 when video mode in MT658X; or memory leakage
	#endif
	
	// Video mode setting
	params->dsi.packet_size=256;
	
	params->dsi.vertical_sync_active = 2;//
	params->dsi.vertical_backporch = 13;//
	params->dsi.vertical_frontporch = 34;//
	params->dsi.vertical_active_line = FRAME_HEIGHT;

	params->dsi.horizontal_sync_active = 4;
	params->dsi.horizontal_backporch =28;
	params->dsi.horizontal_frontporch = 38;
	params->dsi.horizontal_active_pixel = FRAME_WIDTH;

    params->dsi.PLL_CLOCK= 509;        //4LANE   232
	params->dsi.ssc_disable = 0;
	params->dsi.ssc_range = 1;
	params->dsi.cont_clock = 0;
	params->dsi.clk_lp_per_line_enable = 0;

	params->physical_width = LCM_PHYSICAL_WIDTH/1000;
	params->physical_height = LCM_PHYSICAL_HEIGHT/1000;

	#if 1
	params->dsi.ssc_disable = 1;
	params->dsi.lcm_ext_te_monitor = FALSE;
		
	params->dsi.esd_check_enable = 1;
	params->dsi.customization_esd_check_enable = 1;
	params->dsi.lcm_esd_check_table[0].cmd			= 0x0a;
	params->dsi.lcm_esd_check_table[0].count		= 1;
	params->dsi.lcm_esd_check_table[0].para_list[0] = 0x9c;
	#endif
	/*prize-add-for round corner-houjian-20180918-start*/
	#ifdef MTK_ROUND_CORNER_SUPPORT
	params->round_corner_params.w = ROUND_CORNER_W;
	params->round_corner_params.h = ROUND_CORNER_H;
	params->round_corner_params.lt_addr = left_top;
	params->round_corner_params.rt_addr = right_top;
	params->round_corner_params.lb_addr = left_bottom;
	params->round_corner_params.rb_addr = right_bottom;
	#endif
	/*prize-add-for round corner-houjian-20180918-end*/
}


static unsigned int lcm_compare_id(void)
{
	#define LCM_ID 0x00008000
    int array[4];
    char buffer[4]={0,0,0,0};
    int id = 0;

	display_ldo18_enable(1);
	MDELAY(5);

	display_bias_enable();
	SET_RESET_PIN(1);
	MDELAY(10);
	SET_RESET_PIN(0);
	MDELAY(10);  

	SET_RESET_PIN(1);
	MDELAY(60);//250

    array[0] = 0x00013700;
    dsi_set_cmdq(array, 1, 1);

    MDELAY(1);
    read_reg_v2(0xda, &buffer[0], 1);//    NC 0x00  0x98 0x16
    MDELAY(1);
    read_reg_v2(0xdb, &buffer[1], 1);//    NC 0x00  0x98 0x16
    MDELAY(1);
    read_reg_v2(0xdc, &buffer[2], 1);//    NC 0x00  0x98 0x16

	id = (buffer[0]<<16) | (buffer[1]<<8) | buffer[2];

#ifdef BUILD_LK
	dprintf(0,"%s: ili7807q id = 0x%08x\n", __func__, id);
#else
	printf("%s: ili7807q id = 0x%08x \n", __func__, id);
#endif

    return (LCM_ID == id)?1:0;
}

static void lcm_init(void)
{
	dprintf("ili7807q: %s\n", __func__);
	
//	int ret = 0;
#if defined (USE_LDO)
	display_ldo18_enable(1);
//	MDELAY(2);
	//SET_RESET_PIN(0);
	display_ldo28_enable(1);
	MDELAY(10);
#endif
	SET_RESET_PIN(0);
	//AVDD  AVEE
	display_bias_enable();
	//lcm_ret
	MDELAY(10);
	SET_RESET_PIN(1);
	//TP_rest prize-huangjiwu-for fae  begin
	mt_set_gpio_mode(GPIO_TP_RST, GPIO_MODE_00);
	mt_set_gpio_dir(GPIO_TP_RST, GPIO_DIR_OUT);
	mt_set_gpio_out(GPIO_TP_RST, GPIO_OUT_ONE);
	//TP_rest prize-huangjiwu-for fae  end
	MDELAY(10);
	//SET_RESET_PIN(0);
	//MDELAY(10);  

	//SET_RESET_PIN(1);
	//MDELAY(60);//250
	
	push_table(lcm_initialization_setting, sizeof(lcm_initialization_setting)/sizeof(struct LCM_setting_table), 1);
}

static void lcm_suspend(void)
{
	int ret = 0;

	push_table(lcm_suspend_setting, sizeof(lcm_suspend_setting)/sizeof(struct LCM_setting_table), 1);
	MDELAY(10);//100
	
}

static void lcm_resume(void)
{
	lcm_init();
}

LCM_DRIVER ili7807q_hdp_dsi_vdo_auo_lcm_drv = 
{
    .name			= "ili7807q_hdp_dsi_vdo_auo",
	.set_util_funcs = lcm_set_util_funcs,
	.get_params     = lcm_get_params,
	.init           = lcm_init,
	.suspend        = lcm_suspend,
	.resume         = lcm_resume,
	.compare_id     = lcm_compare_id,
};
