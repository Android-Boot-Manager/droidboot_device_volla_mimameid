#include <mt_gpio.h>
#include <mt_i2c.h>
#ifndef USE_DTB_NO_DWS
#include <cust_gpio_usage.h>
#endif
#include <stdbool.h>
#include "aw99703_lk_driver.h"

#define CPD_TAG			"[LK][chargepump_aw99703] "
#define CPD_FUN(f)		printf(CPD_TAG"%s\n", __FUNCTION__)
#define CPD_ERR(fmt, args...)	printf(CPD_TAG"%s %d : "fmt, __FUNCTION__, __LINE__, ##args)
#define CPD_LOG(fmt, args...)	printf(CPD_TAG fmt, ##args)

//prize add by lipengpeng 20200928 start
//#define AW99703_I2C_ID	I2C0
#define AW99703_I2C_ID	I2C6
//prize add by lipengpeng 20200928 end
static struct mt_i2c_t	aw99703_i2c;
unsigned char aw99703_reg[AW99703_REG_MAX] = { 0 };

int aw99703_i2c_write(unsigned char reg_addr, unsigned char data)
{
	u32 ret_code = I2C_OK;
	u8 write_data[2];
	u16 len;

	write_data[0]= reg_addr;
	write_data[1] = data;
	aw99703_i2c.id = AW99703_I2C_ID;
	aw99703_i2c.addr = AW99703_SLAVE_ADDR;
	aw99703_i2c.mode = ST_MODE;
	aw99703_i2c.speed = 100;
	len = 2;

	ret_code = i2c_write(&aw99703_i2c, write_data, len);
	//dprintf(CRITICAL, "%s: i2c_write: ret_code: %d\n", __func__, ret_code);
	return ret_code;
}

int aw99703_i2c_read(u8 addr, u8 *dataBuffer)
{
	u32 ret_code = I2C_OK;
	u16 len;
	*dataBuffer = addr;

	aw99703_i2c.id = AW99703_I2C_ID;
	aw99703_i2c.addr = AW99703_SLAVE_ADDR;
	aw99703_i2c.mode = ST_MODE;
	aw99703_i2c.speed = 100;
	len = 1;
	ret_code = i2c_write_read(&aw99703_i2c, dataBuffer, len, len);
	//dprintf(CRITICAL, "%s: i2c_read: ret_code: %d\n", __func__, ret_code);
	return ret_code;
}

int aw99703_i2c_write_bit(unsigned char reg_addr, unsigned int mask, unsigned char reg_data)
{
	unsigned char reg_val = 0;

	aw99703_i2c_read(reg_addr, &reg_val);
	reg_val &= mask;
	reg_val |= reg_data;
	aw99703_i2c_write(reg_addr, reg_val);

	return 0;
}


void aw99703_dump_register()
{
	int i;
	CPD_FUN();

	for (i= 0;i < AW99703_REG_MAX; i++) {
		if(!(aw99703_reg_access[i]&REG_RD_ACCESS))
			continue;
		aw99703_i2c_read(i, &aw99703_reg[i]);
		printf("aw99703_register_[0x%x]=0x%x \n ", i, aw99703_reg[i]);
	}
}

static int aw99703_hw_init()
{
	CPD_FUN();
	dprintf(1,"%s wuhui  init start\n",__func__);
	// GPIO for DSV setting
	//mt_set_gpio_mode(AW99703_DSV_VPOS_EN, AW99703_DSV_VPOS_EN_MODE);
	//mt_set_gpio_pull_enable(AW99703_DSV_VPOS_EN, GPIO_PULL_ENABLE);
	//mt_set_gpio_dir(AW99703_DSV_VPOS_EN, GPIO_DIR_OUT);
	//mt_set_gpio_mode(AW99703_DSV_VNEG_EN, AW99703_DSV_VNEG_EN_MODE);
	//mt_set_gpio_pull_enable(AW99703_DSV_VNEG_EN, GPIO_PULL_ENABLE);
	//mt_set_gpio_dir(AW99703_DSV_VNEG_EN, GPIO_DIR_OUT);

	// Backlight Enable Pin
	mt_set_gpio_mode((0x80000000 | 24), AW99703_CHIP_ENABLE_PIN_MODE);
	mt_set_gpio_dir((0x80000000 | 24), GPIO_DIR_OUT);
	mt_set_gpio_out((0x80000000 | 24), GPIO_OUT_ONE);
	udelay(100);
	mt_set_gpio_out((0x80000000 | 24),GPIO_OUT_ZERO);
	udelay(10000);
	mt_set_gpio_out((0x80000000 | 24),GPIO_OUT_ONE);
	udelay(250);

	/*disable pwm*/
	aw99703_i2c_write_bit(AW99703_REG_MODE,
				AW99703_MODE_PDIS_MASK,
				AW99703_MODE_PDIS_DISABLE);

	/*config map type*/
	aw99703_i2c_write_bit(AW99703_REG_MODE,
				AW99703_MODE_MAP_MASK,
				AW99703_MODE_MAP_LINEAR);

	/*config backlight max current:default:4.8mA+19*0.8=20mA */
	aw99703_i2c_write_bit(AW99703_REG_LEDCUR, 
				AW99703_LEDCUR_BLFS_MASK,
				0x15 << 3);//prize-modify backlight max current 40ma-pengzhipeng-20210805

	/*channel 1 enable*/
	aw99703_i2c_write_bit(AW99703_REG_LEDCUR,
				AW99703_LEDCUR_CHANNEL_MASK,
				AW99703_LEDCUR_CH1_ENABLE |
				AW99703_LEDCUR_CH2_ENABLE |
				AW99703_LEDCUR_CH3_DISABLE);//prize wyq disable ch3 

	/*turncfg on_time and off time*/
	aw99703_i2c_write(AW99703_REG_TURNCFG, (0x04 << 4) | (0x04 << 0));//on:8ms, off:8ms

	/*trancef  on_time*/
	aw99703_i2c_write(AW99703_REG_TRANCFG, (0x00 << 4) | (0x00 << 0));//pwm:2ms, i2c:1us

	//prize wyq 20191019 move hw init to lk-start
	/*default OVPSEL 38V*/
	aw99703_i2c_write_bit(AW99703_REG_BSTCTR1,
				AW99703_BSTCTR1_OVPSEL_MASK,
				AW99703_BSTCTR1_OVPSEL_38V);

	/*switch frequency 1000kHz*/
	aw99703_i2c_write_bit(AW99703_REG_BSTCTR1,
				AW99703_BSTCTR1_SF_MASK,
				AW99703_BSTCTR1_SF_500KHZ);

	/*OCP SELECT*/
	aw99703_i2c_write_bit(AW99703_REG_BSTCTR1,
				AW99703_BSTCTR1_OCPSEL_MASK,
				AW99703_BSTCTR1_OCPSEL_3P3A);

	/*BSTCRT2 IDCTSEL*/
	aw99703_i2c_write_bit(AW99703_REG_BSTCTR2,
				AW99703_BSTCTR2_IDCTSEL_MASK,
				AW99703_BSTCTR2_IDCTSEL_10UH);
                
	aw99703_i2c_write_bit(AW99703_REG_BSTCTR2,
				AW99703_BSTCTR2_IDCTSEL_MASK,
				AW99703_BSTCTR2_EMISEL_SLOW3);

	/*aw99703_i2c_write_bit(AW99703_REG_TURNCFG,
				AW99703_TURNCFG_ON_TIM_MASK,
				0x02 << 4);

	aw99703_i2c_write_bit(AW99703_REG_TURNCFG,
				AW99703_TURNCFG_OFF_TIM_MASK,
				0x02);
	
	aw99703_i2c_write_bit(AW99703_REG_TRANCFG,
				AW99703_TRANCFG_PWM_TIM_MASK,
				0x00);

	aw99703_i2c_write_bit(AW99703_REG_TRANCFG,
				AW99703_TRANCFG_I2C_TIM_MASK,
				0x07);*/
	//prize wyq 20191019 move hw init to lk-start
    
	aw99703_i2c_write_bit(0x67, 0x00, 0x1c);
                
	aw99703_i2c_write_bit(0x25, 0x00, 0x61);
                
	aw99703_i2c_write_bit(0x24, 0x00, 0x4c);
                
	aw99703_i2c_write_bit(0x23, 0x00, 0x61);
	dprintf(1,"%s wuhui  init exit\n",__func__);
		
	return 0;
}

static int get_backlight_enable=0;
int chargepump_set_backlight_level(int level)
{
	int data_len = 1;

	if(get_backlight_enable == 0)
	{
		aw99703_hw_init();
		get_backlight_enable=1;
	}

	level *= 8;//prize wyq 20191019 (0-255) map to (0-2047)
	
	CPD_LOG("chargepump_set_backlight_level(0-2047), level : %d\n", level);

	if(level != 0) 
	{
		/* set backlight level */
		aw99703_i2c_write(AW99703_REG_LEDLSB, level&0x0007);
		aw99703_i2c_write(AW99703_REG_LEDMSB, (level >> 3)&0xff);

		/* backlight enable */
		aw99703_i2c_write_bit(AW99703_REG_MODE,
					AW99703_MODE_WORKMODE_MASK,
					AW99703_MODE_WORKMODE_BACKLIGHT);
	} else {
		/* standby mode*/
		aw99703_i2c_write_bit(AW99703_REG_MODE,
					AW99703_MODE_WORKMODE_MASK,
					AW99703_MODE_WORKMODE_STANDBY);
	}
	aw99703_dump_register();
	dprintf(1,"%s exit\n",__func__);
	return 1;
}

void chargepump_DSV_on()
{
	CPD_FUN(); 
	printf("20190410 enter %s\n",__func__);

	//mt_set_gpio_out(AW99703_DSV_VPOS_EN, GPIO_OUT_ONE);
	//mdelay(1);
	//mt_set_gpio_out(AW99703_DSV_VNEG_EN, GPIO_OUT_ONE);
}

void chargepump_DSV_off()
{
	CPD_FUN();

	//mt_set_gpio_out(AW99703_DSV_VPOS_EN, GPIO_OUT_ZERO);
	//mdelay(1);
	//mt_set_gpio_out(AW99703_DSV_VNEG_EN, GPIO_OUT_ZERO);
}
