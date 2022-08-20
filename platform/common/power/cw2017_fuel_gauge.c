#include <platform/mt_typedefs.h>
#include <platform/mt_i2c.h>   
#include <platform/mt_pmic.h>
#include "cw2017_fuel_gauge.h"
#include <platform/timer.h>
#include <platform/mt_pmic.h>
#if defined(CONFIG_MTK_CW2017_SUPPORT_OF)
#include <libfdt.h>
#include <lk_builtin_dtb.h>
#endif

#define CWFG_ENABLE_LOG 1 //CHANGE   Customer need to change this for enable/disable log
#define CWFG_I2C_BUSNUM 0 //CHANGE   Customer need to change this number according to the principle of hardware
/*
#define USB_CHARGING_FILE "/sys/class/power_supply/usb/online" // Chaman
#define DC_CHARGING_FILE "/sys/class/power_supply/ac/online"
*/

#define REG_VERSION             0x00
#define REG_VCELL_H             0x02
#define REG_VCELL_L             0x03
#define REG_SOC_INT             0x04
#define REG_SOC_DECIMAL         0x05
#define REG_TEMP                0x06
#define REG_MODE_CONFIG         0x08
#define REG_GPIO_CONFIG         0x0A
#define REG_SOC_ALERT           0x0B
#define REG_TEMP_MAX            0x0C
#define REG_TEMP_MIN            0x0D
#define REG_VOLT_ID_H           0x0E
#define REG_VOLT_ID_L           0x0F
#define REG_BATINFO             0x10

#define MODE_SLEEP              0x30
#define MODE_NORMAL             0x00
#define MODE_DEFAULT            0xF0
#define CONFIG_UPDATE_FLG       0x80
#define NO_START_VERSION        160

#define GPIO_CONFIG_MIN_TEMP             (0x00 << 4)
#define GPIO_CONFIG_MAX_TEMP             (0x00 << 5)
#define GPIO_CONFIG_SOC_CHANGE           (0x00 << 6)
#define GPIO_CONFIG_MIN_TEMP_MARK        (0x01 << 4)
#define GPIO_CONFIG_MAX_TEMP_MARK        (0x01 << 5)
#define GPIO_CONFIG_SOC_CHANGE_MARK      (0x01 << 6)
#define ATHD                              0x0		 //0x7F
#define DEFINED_MAX_TEMP                          450
#define DEFINED_MIN_TEMP                          0


#define cw_printk(fmt, arg...)        \
	({                                    \
		if(CWFG_ENABLE_LOG){\
			dprintf(CRITICAL,"FG_CW2017 : %s : " fmt, __FUNCTION__ ,##arg);\
		}else{}                           \
	})     //need check by Chaman

//#define cw_printk(fmt, arg...)  printk("FG_CW2017 : %s : " fmt, __FUNCTION__ ,##arg);

#define CWFG_NAME "cw2017"
#define SIZE_BATINFO    80
#define CW2017_SLAVE_ADDR_WRITE   0xC6 //0x62
#define CW2017_SLAVE_ADDR_READ    0xC7 //0x63
#define CW2017_I2C_ID	I2C0
//#define  LOW_TEMPERATURE_POWER_OFF 0xf0
static struct mt_i2c_t cw2017_i2c;


#if defined(CONFIG_MTK_CW2017_SUPPORT_OF)
static kal_uint16 cw2017_i2c_idinfo = I2C6;
static unsigned char config_info[SIZE_BATINFO] = {0};
#else
static unsigned char config_info[SIZE_BATINFO] = {
    #include "S7_Profile_8000mAh_profile_BRZH470X_20210317.i"
};
#endif



struct cw_battery {
    struct i2c_client *client;
    int charger_mode;
    int capacity;
    int voltage;
    int status;
    int time_to_empty;
	int change;
    //int alt;
};

int cw_write(kal_uint8 addr, kal_uint8 value)
{
    int ret_code = I2C_OK;
    kal_uint8 write_data[2];
    kal_uint16 len;

    write_data[0]= addr;
    write_data[1] = value;

#if defined(CONFIG_MTK_CW2017_SUPPORT_OF)
    cw2017_i2c.id = cw2017_i2c_idinfo;
#else
    cw2017_i2c.id = CW2017_I2C_ID;
#endif
	
    /* Since i2c will left shift 1 bit, we need to set CW2017 I2C address to >>1 */
    cw2017_i2c.addr = (CW2017_SLAVE_ADDR_WRITE >> 1);
    cw2017_i2c.mode = ST_MODE;
    cw2017_i2c.speed = 100;
    len = 2;

    ret_code = i2c_write(&cw2017_i2c, write_data, len);
    if(I2C_OK != ret_code)
        cw_printk("%s: i2c_write: ret_code: %d\n", __func__, ret_code);

	//cw_printk("cw2017 %2x = %2x\n", addr, value);
    return ret_code;
}

int cw_read(kal_uint8 addr, kal_uint8 *dataBuffer) 
{
    int ret_code = I2C_OK;
    kal_uint16 len;
    *dataBuffer = addr;

#if defined(CONFIG_MTK_CW2017_SUPPORT_OF)
    cw2017_i2c.id = cw2017_i2c_idinfo;
#else
    cw2017_i2c.id = CW2017_I2C_ID;
#endif

    /* Since i2c will left shift 1 bit, we need to set CW2017 I2C address to >>1 */
    cw2017_i2c.addr = (CW2017_SLAVE_ADDR_READ >> 1);
    cw2017_i2c.mode = ST_MODE;
    cw2017_i2c.speed = 100;
    len = 1;

    ret_code = i2c_write_read(&cw2017_i2c, dataBuffer, len, len);
	//cw_printk("cw2017 read %2x = %2x ret_code=%d\n", addr,  ret_code);
    if(I2C_OK != ret_code)
        cw_printk("%s: i2c_read: ret_code: %d\n", __func__, ret_code);
	
	//cw_printk("cw2017 read %2x = %2x\n", addr,  dataBuffer[0]);

    return ret_code;
}

int cw_read_word(kal_uint8 addr, kal_uint8 *dataBuffer) 
{
		int ret_code = I2C_OK;
		kal_uint16 len;
		*dataBuffer = addr;
	
#if defined(CONFIG_MTK_CW2017_SUPPORT_OF)
		cw2017_i2c.id = cw2017_i2c_idinfo;
#else
		cw2017_i2c.id = CW2017_I2C_ID;
#endif
	
		/* Since i2c will left shift 1 bit, we need to set CW2017 I2C address to >>1 */
		cw2017_i2c.addr = (CW2017_SLAVE_ADDR_READ >> 1);
		cw2017_i2c.mode = ST_MODE;
		cw2017_i2c.speed = 100;
		len = 2;
	
		ret_code = i2c_write_read(&cw2017_i2c, dataBuffer, 1, len);
		//cw_printk("cw2017 read %2x = %2x ret_code=%d\n", addr,  ret_code);
		if(I2C_OK != ret_code)
			cw_printk("%s: i2c_read: ret_code: %d\n", __func__, ret_code);
		
		//cw_printk("cw2017 read %2x = %2x\n", addr,  dataBuffer[0]);
		return ret_code;

}

static int cw2017_enable(void)
{
	int ret;
    unsigned char reg_val = MODE_DEFAULT;
	cw_printk("cw2017_enable!!!\n");

	ret = cw_write(REG_MODE_CONFIG, &reg_val);
	if(ret < 0)
		return ret;
	
	mdelay(20); 
	reg_val = MODE_SLEEP;
	ret = cw_write(REG_MODE_CONFIG, &reg_val);
	if(ret < 0)
		return ret;
	
	mdelay(20);
	reg_val = MODE_NORMAL;
	ret = cw_write(REG_MODE_CONFIG, &reg_val);
	if(ret < 0)
		return ret;
	
	mdelay(20);
	return 0;	
}

static int cw_get_version(void)
{
	int ret = 0;
	unsigned char reg_val = 0;
	int version = 0;
	ret = cw_read(REG_VERSION, &reg_val);
	if(ret < 0)
		return ret;
	
	version = reg_val;	 
	cw_printk("version = %d\n", version);
	return version;	
}

/*CW2017 update profile function, Often called during initialization*/
int cw_update_config_info(void)
{
	int ret = 0;
	unsigned char i = 0;
	unsigned char reg_val = 0;
	unsigned char reg_val_dig = 0;
	int count = 0;

	/* update new battery info */
	for(i = 0; i < SIZE_BATINFO; i++)
	{
		reg_val = config_info[i];
		ret = cw_write(REG_BATINFO + i, &reg_val);
        if(ret < 0) 
			return ret;
//		cw_printk("w reg[%02X] = %02X\n", REG_BATINFO +i, reg_val);
	}
	
	ret = cw_read(REG_SOC_ALERT, &reg_val);
	if(ret < 0)
		return ret;
	
	reg_val |= CONFIG_UPDATE_FLG;   /* set UPDATE_FLAG */
	ret = cw_write(REG_SOC_ALERT, &reg_val);
	if(ret < 0)
		return ret;

	ret = cw2017_enable();
	if(ret < 0) 
		return ret;
	
	while(cw_get_version() == NO_START_VERSION){
		mdelay(100);
		count++;
		if(count > 30)
			break;
	}

	for (i = 0; i < 30; i++) {
		mdelay(100);
        ret = cw_read(REG_SOC_INT, &reg_val);
        ret = cw_read(REG_SOC_INT + 1, &reg_val_dig);
		cw_printk("i = %d soc = %d, .soc = %d\n", i, reg_val, reg_val_dig);
        if (ret < 0)
            return ret;
        else if (reg_val <= 100) 
            break;	
    }
    return 0;
}

//prize-add-sunshuai-2015 Multi-Battery Solution-20200222-start
#if defined(CONFIG_MTK_CW2017_BATTERY_ID_AUXADC)
static unsigned int chr_fdt_getprop_u32(const void *fdt, int nodeoffset,
                                 const char *name)
{
	const void *data = NULL;
	int len = 0;

	data = fdt_getprop(fdt, nodeoffset, name, &len);
	if (len > 0 && data)
		return fdt32_to_cpu(*(unsigned int *)data);
	else
		return 0;
}
#endif
//prize-add-sunshuai-2015 Multi-Battery Solution-20200222-end

/*CW2017 init function, Often called during initialization*/
int cw2017_init(void)
{
    int ret;
    int i;
    unsigned char reg_val = MODE_SLEEP;
	unsigned char reg_val00;
	unsigned char config_flg = 0;

#if defined(CONFIG_MTK_CW2017_SUPPORT_OF)
	const void *data = NULL;
	int len = 0;
	int offset = 0;

// prize-add-sunshuai-2015 Multi-Battery Solution-20200222-start
#if defined(CONFIG_MTK_CW2017_BATTERY_ID_AUXADC)
	int val =0;
	int Voltiage_cali =0;
#endif
// prize-add-sunshuai-2015 Multi-Battery Solution-20200222-end
	
	//void *kernel_fdt = get_kernel_fdt();
	
//prize-add-sunshuai-2015 Multi-Battery Solution-20200222-start
#if defined(CONFIG_MTK_CW2017_BATTERY_ID_AUXADC)
    	//get i2c_id from dts
	offset = fdt_node_offset_by_compatible(get_lk_overlayed_dtb(),-1,"cellwise,cw2017");
	if (offset < 0) {
		cw_printk("[cw2017] Failed to find cellwise,cw2017 in dtb\n");
	}else{		
        val = chr_fdt_getprop_u32(get_lk_overlayed_dtb(), offset, "bat_first_startrange");
		if (val)
			cw2017fuelguage.bat_first_startrange = val;
		
		val = chr_fdt_getprop_u32(get_lk_overlayed_dtb(), offset, "bat_first_endrange");
		if (val)
			cw2017fuelguage.bat_first_endrange = val;
		
		val = chr_fdt_getprop_u32(get_lk_overlayed_dtb(), offset, "bat_second_startrange");
		if (val)
			cw2017fuelguage.bat_second_startrange = val;
		
		val = chr_fdt_getprop_u32(get_lk_overlayed_dtb(), offset, "bat_second_endrange");
		if (val)
			cw2017fuelguage.bat_second_endrange = val;
		
		val = chr_fdt_getprop_u32(get_lk_overlayed_dtb(), offset, "bat_third_startrange");
		if (val)
			cw2017fuelguage.bat_third_startrange = val;
		
		val = chr_fdt_getprop_u32(get_lk_overlayed_dtb(), offset, "bat_third_endrange");
		if (val)
			cw2017fuelguage.bat_third_endrange = val;
		
		val = chr_fdt_getprop_u32(get_lk_overlayed_dtb(), offset, "bat_channel_num");
		if (val)
			cw2017fuelguage.bat_channel_num = val;

	    cw_printk("[cw2017] cw2017fuelguage.bat_first_startrange =%d \n",cw2017fuelguage.bat_first_startrange);
		cw_printk("[cw2017] cw2017fuelguage.bat_first_endrange =%d \n",cw2017fuelguage.bat_first_endrange);
		cw_printk("[cw2017] cw2017fuelguage.bat_second_startrange =%d \n",cw2017fuelguage.bat_second_startrange);
	    cw_printk("[cw2017] cw2017fuelguage.bat_second_endrange =%d \n",cw2017fuelguage.bat_second_endrange);
		cw_printk("[cw2017] cw2017fuelguage.bat_third_startrange =%d \n",cw2017fuelguage.bat_third_startrange);
	    cw_printk("[cw2017] cw2017fuelguage.bat_third_endrange =%d \n",cw2017fuelguage.bat_third_endrange);
		cw_printk("[cw2017] cw2017fuelguage.bat_channel_num =%d \n",cw2017fuelguage.bat_channel_num);

        ret= IMM_GetOneChannelValue_Cali(cw2017fuelguage.bat_channel_num, &Voltiage_cali);
		if (ret < 0) {
			dprintf(1,"cw2017_init [adc_driver]: get channel[%d] cali voltage error\n",cw2017fuelguage.bat_channel_num);
		} else {
			dprintf(1,"cw2017_init [adc_driver]: get channel[%d] cali_voltage =%d\n",cw2017fuelguage.bat_channel_num,Voltiage_cali);
		}

		if(Voltiage_cali > cw2017fuelguage.bat_first_startrange && Voltiage_cali < cw2017fuelguage.bat_first_endrange)
			cw2017fuelguage.bat_id = 0;
		else if(Voltiage_cali > cw2017fuelguage.bat_second_startrange && Voltiage_cali < cw2017fuelguage.bat_second_endrange)
			cw2017fuelguage.bat_id = 1;
		else if(Voltiage_cali > cw2017fuelguage.bat_third_startrange && Voltiage_cali < cw2017fuelguage.bat_third_endrange)
			cw2017fuelguage.bat_id = 2;
		else{
			cw2017fuelguage.bat_id = 3;
			cw_printk("[cw2017] cw2017_init did not find Curve corresponding to the battery ,use default Curve\n");
		}
		
		cw_printk("[cw2017]  Curve name %s\n",fuelguage_name[cw2017fuelguage.bat_id]);
		
		//batinfo
		data = fdt_getprop(get_lk_overlayed_dtb(), offset, fuelguage_name[cw2017fuelguage.bat_id], &len);
		if (len > 0 && data){
			cw_printk("[cw2017] batinfo len(%d)\n",len);
			if (len != SIZE_BATINFO){
				cw_printk("[cw2017] get bat batinfo fail len(%d) != SIZE_BATINFO(%d)\n",len,SIZE_BATINFO);
			}else{
				memcpy(config_info,data,len);
				cw_printk("[cw2017]batinfo \n");
				for (i=0;i<SIZE_BATINFO;i++){
					cw_printk("cw2017_init lk config_info[%d] =%x\n",i,config_info[i]);
				}
				cw_printk("\n");
			}
		}
		//i2c id
		offset = fdt_parent_offset(get_lk_overlayed_dtb(),offset);
		data = fdt_getprop(get_lk_overlayed_dtb(), offset, "id", &len);
		if (len > 0 && data){
			cw2017_i2c_idinfo = fdt32_to_cpu(*(unsigned int *)data);
			dprintf(INFO, "[cw2017] i2c_id(%d)\n",cw2017_i2c_idinfo);
		}
	}
#else
	//get i2c_id from dts
	offset = fdt_node_offset_by_compatible(get_lk_overlayed_dtb(),-1,"cellwise,cw2017");
	if (offset < 0) {
		cw_printk("[cw2017] Failed to find cellwise,cw2017 in dtb\n");
	}else{
		//batinfo
		data = fdt_getprop(get_lk_overlayed_dtb(), offset, "batinfo", &len);
		if (len > 0 && data){
			cw_printk("[cw2017] batinfo len(%d)\n",len);
			if (len != SIZE_BATINFO){
				cw_printk("[cw2017] get bat batinfo fail len(%d) != SIZE_BATINFO(%d)\n",len,SIZE_BATINFO);
			}else{
				memcpy(config_info,data,len);
				cw_printk("[cw2017]batinfo ");
				for (i=0;i<SIZE_BATINFO;i++){
					cw_printk("cw2017_init lk config_info[%d] =%x\n",i,config_info[i]);
				}
				cw_printk("\n");
			}
		}
		//i2c id
		offset = fdt_parent_offset(get_lk_overlayed_dtb(),offset);
		data = fdt_getprop(get_lk_overlayed_dtb(), offset, "id", &len);
		if (len > 0 && data){
			cw2017_i2c_idinfo = fdt32_to_cpu(*(unsigned int *)data);
			dprintf(INFO, "[cw2017] i2c_id(%d)\n",cw2017_i2c_idinfo);
		}
	}
#endif
//prize-add-sunshuai-2015 Multi-Battery Solution-20200222-end

#endif

	ret = cw_read(REG_MODE_CONFIG, &reg_val);
	if(ret < 0)
		return ret;
	
	ret =  cw_read(REG_SOC_ALERT, &config_flg);
	if(ret < 0)
		return ret;

	if(reg_val != MODE_NORMAL || ((config_flg & CONFIG_UPDATE_FLG) == 0x00)){
		ret = cw_update_config_info();
		if(ret < 0)
			return ret;
	} else {
		for(i = 0; i < SIZE_BATINFO; i++)
		{ 
			ret =  cw_read(REG_BATINFO +i, &reg_val);
			if(ret < 0)
				return ret;
			
			cw_printk("r reg[%02X] = %02X\n", REG_BATINFO +i, reg_val);
			if(config_info[i] != reg_val)
			{
				break;
			}
		}
		if(i != SIZE_BATINFO)
		{
			//"update flag for new battery info need set"
			ret = cw_update_config_info();
			if(ret < 0)
				return ret;
		}
	}
	cw_printk("cw2017 init success!\n");	

    return 0;
}

/*static int cw_por(struct cw_battery *cw_bat)
{
	int ret;
	unsigned char reset_val;
	
	reset_val = MODE_SLEEP; 			  
	ret = cw_write(cw_bat->client, REG_MODE, &reset_val);
	if (ret < 0)
		return ret;
	reset_val = MODE_NORMAL;
	mdelay(10);
	ret = cw_write(cw_bat->client, REG_MODE, &reset_val);
	if (ret < 0)
		return ret;
	ret = cw_init(cw_bat);
	if (ret) 
		return ret;
	return 0;
}
static int cw_quickstart(void)
{
        int ret = 0;
        u8 reg_val = MODE_QUICK_START;

        ret = cw_write(REG_MODE, reg_val);     //(MODE_QUICK_START | MODE_NORMAL));  // 0x30
        if(ret < 0) {
                cw_printk("cw2017 Error quick start1\n");
                return ret;
        }
        
        reg_val = MODE_NORMAL;
        ret = cw_write(REG_MODE, reg_val);
        if(ret < 0) {
                cw_printk("cw2017 Error quick start2\n");
                return ret;
        }
        return 1;
}
*/

#define UI_FULL 100
#define DECIMAL_MAX 80
#define DECIMAL_MIN 20 
int cw2017_get_capacity(void)
{
//	int cw_capacity = 200;
	int ret = 0;
	unsigned char reg_val = 0;
	int soc = 0;
	int soc_decimal = 0;
	int UI_SOC = 0;
	int UI_decimal = 0;
	static int reset_loop = 0;
	ret = cw_read(REG_SOC_INT, &reg_val);
	if(ret < 0)
		return ret;
	soc = reg_val;
	
	ret = cw_read(REG_SOC_DECIMAL, &reg_val);
	if(ret < 0)
		return ret;
	soc_decimal = reg_val;
		
	if(soc > 100){		
		reset_loop++;
		 cw_printk("IC error read soc error %d times\n", reset_loop);
		if(reset_loop > 5){
			reset_loop = 0;
			 cw_printk("IC error. please reset IC");
			cw2017_enable(); //here need modify
		}
//		return cw_bat->capacity;
	}
	else{
		reset_loop = 0; 
	}
	
	UI_SOC = ((soc * 256 + soc_decimal) * 100)/ (UI_FULL*256);
	UI_decimal = (((soc * 256 + soc_decimal) * 100 * 100) / (UI_FULL*256)) % 100;
	cw_printk("UI_SOC = %d, UI_decimal = %d soc = %d, soc_decimal = %d\n", UI_SOC, UI_decimal, soc, soc_decimal);
	
	/* case 1 : aviod swing */
	if(UI_SOC >= 100){
		cw_printk("UI_SOC = %d larger 100!!!!\n", UI_SOC);
		UI_SOC = 100;
	}

	return UI_SOC;
}

/*This function called when get voltage from cw2017*/
int cw2017_get_voltage(void)
{    
    int ret;
    unsigned char reg_val[2];
//    u16 value16, value16_1, value16_2, value16_3;
    int voltage;
    
    ret = cw_read_word(REG_VCELL_H, reg_val);
    if(ret < 0) {
        return ret;
    }

	voltage = (reg_val[0] << 8) + reg_val[1];
	voltage = voltage  * 5 / 16;
	cw_printk("voltage = %d\n",voltage);
    return voltage;
}

