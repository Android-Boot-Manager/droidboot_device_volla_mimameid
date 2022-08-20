
/*
PRIZE Inc. (C) 2021. All rights reserved.
driver code for sgm41512 charge ic
zhaopengge@szprize.com
2021/02/24
*/

#include <platform/mt_typedefs.h>
#include <platform/errno.h>
#include <platform/mt_i2c.h>
#include <platform/mt_gpio.h>
#include <libfdt.h>
#include <lk_builtin_dtb.h>
#include "sgm41512.h"
#include "mtk_charger_intf.h"

#define SGM41512_LK_DRV_VERSION	"1.0.3_MTK LK"

#define U32_MAX     ((u32)~0U)

static const u8 sgm41512_irq_maskall[SGM41512_STREG_MAX] = {
	0xFF, 0xFF, 0xFF,
};

static const u32 sgm41512_vac_ovp[] = {
	5500000, 6500000, 10500000, 14000000,
};

static const u32 sgm41512_wdt[] = {
	0, 40, 80, 160,
};

static const u32 sgm41512_otgcc[] = {
	500000, 1200000,
};

static const u8 sgm41512_reg_addr[] = {
	SGM41512_REG_00,
	SGM41512_REG_01,
	SGM41512_REG_02,
	SGM41512_REG_03,
	SGM41512_REG_04,
	SGM41512_REG_05,
	SGM41512_REG_06,
	SGM41512_REG_07,
	SGM41512_REG_08,
	SGM41512_REG_09,
	SGM41512_REG_0A,
	SGM41512_REG_0B,
};

enum sgm41512_mivr_track {
	SGM41512_MIVRTRACK_REG = 0,
	SGM41512_MIVRTRACK_VBAT_200MV,
	SGM41512_MIVRTRACK_VBAT_250MV,
	SGM41512_MIVRTRACK_VBAT_300MV,
	SGM41512_MIVRTRACK_MAX,
};

struct sgm41512_desc {
	u32 vac_ovp;
	u32 mivr;
	u32 aicr;
	u32 cv;
	u32 ichg;
	u32 ieoc;
	u32 safe_tmr;
	u32 wdt;
	u32 mivr_track;
	bool en_safe_tmr;
	bool en_te;
	bool en_jeita;
	bool ceb_invert;
	bool dis_i2c_tout;
	bool en_qon_rst;
	bool auto_aicr;
};

struct sgm41512_chip {
	struct mtk_charger_info mchr_info;
	struct mt_i2c_t i2c;
	int i2c_log_level;
	int hidden_mode_cnt;
	u8 chip_rev;
	struct sgm41512_desc *desc;
	u32 ceb_gpio;
	bool wkard_en;
};


static struct sgm41512_desc sgm41512_default_desc = {
	.vac_ovp = 14000000,
	.mivr = 4400000,
	.aicr = 500000,
	.cv = 4350000,
	.ichg = 1000000,
	.ieoc = 200000,
	.safe_tmr = 10,
	.wdt = 40,
	.mivr_track = SGM41512_MIVRTRACK_REG,
	.en_safe_tmr = true,
	.en_te = true,
	.en_jeita = false,
	.ceb_invert = false,
	.dis_i2c_tout = false,
	.en_qon_rst = true,
	.auto_aicr = false,
};

static int sgm41512_i2c_read_byte(struct sgm41512_chip *chip, u8 cmd, u8 *data)
{
	int ret = 0;
	u8 regval = cmd;
	struct mt_i2c_t *i2c = &chip->i2c;

	ret = i2c_write_read(i2c, &regval, 1, 1);
	if (ret != I2C_OK)
		dprintf(CRITICAL, "%s reg0x%02X fail(%d)\n",
				  __func__, cmd, ret);
	else {
		*data = regval;
	}

	return ret;
}

static int sgm41512_i2c_write_byte(struct sgm41512_chip *chip, u8 cmd, u8 data)
{
	int ret = 0;
	u8 write_buf[2] = {cmd, data};
	struct mt_i2c_t *i2c = &chip->i2c;

	ret = i2c_write(i2c, write_buf, 2);
	if (ret != I2C_OK)
		dprintf(CRITICAL, "%s reg0x%02X = 0x%02X fail(%d)\n",__func__, cmd, data, ret);

	return ret;
}

static int sgm41512_i2c_block_read(struct sgm41512_chip *chip, u8 cmd, u32 len,
				 u8 *data)
{
	int ret = 0, i = 0;
	struct mt_i2c_t *i2c = &chip->i2c;

	data[0] = cmd;

	ret = i2c_write_read(i2c, data, 1, len);
	if (ret != I2C_OK)
		dprintf(CRITICAL, "%s reg0x%02X..reg0x%02X fail(%d)\n",
				  __func__, cmd, cmd + len - 1, ret);

	return ret;
}

static int sgm41512_i2c_block_write(struct sgm41512_chip *chip, u8 cmd, u32 len,
				  const u8 *data)
{
	int ret = 0, i = 0;
	u8 write_buf[len + 1];
	struct mt_i2c_t *i2c = &chip->i2c;

	write_buf[0] = cmd;
	memcpy(&write_buf[1], data, len);

	ret = i2c_write(i2c, write_buf, len + 1);
	if (ret != I2C_OK) {
		dprintf(CRITICAL, "%s fail(%d)\n", __func__, ret);
		for (i = 0; i <= len - 1; i++)
			dprintf(CRITICAL, "%s reg0x%02X = 0x%02X\n",
					  __func__, cmd + i, data[i]);
	}

	return ret;
}

static inline u32 sgm41512_closest_value(u32 min, u32 max, u32 step, u8 regval)
{
	u32 val = 0;

	val = min + regval * step;
	if (val > max)
		val = max;

	return val;
}

static inline u8 sgm41512_closest_reg(u32 min, u32 max, u32 step, u32 target)
{
	if (target < min)
		return 0;

	if (target >= max)
		target = max;

	return (target - min) / step;
}

static int sgm41512_i2c_test_bit(struct sgm41512_chip *chip, u8 cmd, u8 shift,
			       bool *is_one)
{
	int ret = 0;
	u8 regval = 0;

	ret = sgm41512_i2c_read_byte(chip, cmd, &regval);
	if (ret != I2C_OK) {
		*is_one = false;
		return ret;
	}

	regval &= 1 << shift;
	*is_one = (regval ? true : false);

	return ret;
}

static int sgm41512_i2c_update_bits(struct sgm41512_chip *chip, u8 cmd, u8 data,
				  u8 mask)
{
	int ret = 0;
	u8 regval = 0;

	ret = sgm41512_i2c_read_byte(chip, cmd, &regval);
	if (ret != I2C_OK)
		return ret;

	regval &= ~mask;
	regval |= (data & mask);

	return sgm41512_i2c_write_byte(chip, cmd, regval);
}

static inline int sgm41512_set_bit(struct sgm41512_chip *chip, u8 cmd, u8 mask)
{
	return sgm41512_i2c_update_bits(chip, cmd, mask, mask);
}

static inline int sgm41512_clr_bit(struct sgm41512_chip *chip, u8 cmd, u8 mask)
{
	return sgm41512_i2c_update_bits(chip, cmd, 0x00, mask);
}

static inline u8 sgm41512_closest_reg_via_tbl(const u32 *tbl, u32 tbl_size,
					    u32 target)
{
	u32 i = 0;

	if (target < tbl[0])
		return 0;

	for (i = 0; i < tbl_size - 1; i++) {
		if (target >= tbl[i] && target < tbl[i + 1])
			return i;
	}

	return tbl_size - 1;
}

static int __sgm41512_set_wdt(struct sgm41512_chip *chip, u32 sec)
{
	u8 regval = 0;

	/* 40s is the minimum, set to 40 except sec == 0 */
	if (sec <= 40 && sec > 0)
		sec = 40;
	regval = sgm41512_closest_reg_via_tbl(sgm41512_wdt, ARRAY_SIZE(sgm41512_wdt),sec);

	dprintf(chip->i2c_log_level, "%s time = %d(0x%02X)\n",
				     __func__, sec, regval);

	return sgm41512_i2c_update_bits(chip, SGM41512_REG_WDT,
				      regval << SGM41512_WDT_SHIFT,
				      SGM41512_WDT_MASK);
}

static int __sgm41512_set_vac_ovp(struct sgm41512_chip *chip, u32 vac_ovp)
{
	u8 regval = 0;

	regval = sgm41512_closest_reg_via_tbl(sgm41512_vac_ovp,
					    ARRAY_SIZE(sgm41512_vac_ovp),
					    vac_ovp);

	dprintf(CRITICAL, "%s vac_ovp = %d(0x%02X)\n",
			    __func__, vac_ovp, regval);

	return sgm41512_i2c_update_bits(chip, SGM41512_REG_VAC_OVP,
				      regval << SGM41512_VAC_OVP_SHIFT,
				      SGM41512_VAC_OVP_MASK);
}

static int __sgm41512_set_mivr(struct sgm41512_chip *chip, u32 mivr)
{
	u8 regval = 0;

	regval = sgm41512_closest_reg(SGM41512_MIVR_MIN, SGM41512_MIVR_MAX,
				    SGM41512_MIVR_STEP, mivr);

	dprintf(CRITICAL, "%s mivr = %d(0x%02X)\n", __func__, mivr, regval);

	return sgm41512_i2c_update_bits(chip, SGM41512_REG_MIVR,
				      regval << SGM41512_MIVR_SHIFT,
				      SGM41512_MIVR_MASK);
}

static int sgm41512_set_mivr(struct mtk_charger_info *mchr_info, u32 mivr)
{
	struct sgm41512_chip *chip = (struct sgm41512_chip *)mchr_info;

	return __sgm41512_set_mivr(chip, mivr * 1000);
}

static int __sgm41512_set_aicr(struct sgm41512_chip *chip, u32 aicr)
{
	u8 regval = 0;

	regval = sgm41512_closest_reg(SGM41512_AICR_MIN, SGM41512_AICR_MAX,
				    SGM41512_AICR_STEP, aicr);
	/* 0 & 1 are both 50mA */
	if (aicr < SGM41512_AICR_MAX)
		regval += 1;

	dprintf(CRITICAL, "%s aicr = %d(0x%02X)\n", __func__, aicr, regval);

	return sgm41512_i2c_update_bits(chip, SGM41512_REG_AICR,
				      regval << SGM41512_AICR_SHIFT,
				      SGM41512_AICR_MASK);
}

static int sgm41512_set_aicr(struct mtk_charger_info *mchr_info, u32 aicr)
{
	struct sgm41512_chip *chip = (struct sgm41512_chip *)mchr_info;

	return __sgm41512_set_aicr(chip, aicr * 1000);
}



static int __sgm41512_set_cv(struct sgm41512_chip *chip, u32 cv)
{
	u8 regval = 0;

	regval = sgm41512_closest_reg(SGM41512_CV_MIN, SGM41512_CV_MAX,
				    SGM41512_CV_STEP, cv);

	dprintf(CRITICAL, "%s cv = %d(0x%02X)\n", __func__, cv, regval);

	return sgm41512_i2c_update_bits(chip, SGM41512_REG_CV,
				      regval << SGM41512_CV_SHIFT,
				      SGM41512_CV_MASK);
}

static int __sgm41512_set_ichg(struct sgm41512_chip *chip, u32 ichg)
{
	u8 regval = 0;

	regval = sgm41512_closest_reg(SGM41512_ICHG_MIN, SGM41512_ICHG_MAX,
				    SGM41512_ICHG_STEP, ichg);

	dprintf(CRITICAL, "%s ichg = %d(0x%02X)\n", __func__, ichg, regval);

	return sgm41512_i2c_update_bits(chip, SGM41512_REG_ICHG,
				      regval << SGM41512_ICHG_SHIFT,
				      SGM41512_ICHG_MASK);
}

static int sgm41512_set_ichg(struct mtk_charger_info *mchr_info, u32 ichg)
{
	struct sgm41512_chip *chip = (struct sgm41512_chip *)mchr_info;

	return __sgm41512_set_ichg(chip, ichg * 1000);
}

static int __sgm41512_set_ieoc(struct sgm41512_chip *chip, u32 ieoc)
{
	u8 regval = 0;

	regval = sgm41512_closest_reg(SGM41512_IEOC_MIN, SGM41512_IEOC_MAX,
				    SGM41512_IEOC_STEP, ieoc);

	dprintf(CRITICAL, "%s ieoc = %d(0x%02X)\n", __func__, ieoc, regval);

	return sgm41512_i2c_update_bits(chip, SGM41512_REG_IEOC,
				      regval << SGM41512_IEOC_SHIFT,
				      SGM41512_IEOC_MASK);
}

static int __sgm41512_set_safe_tmr(struct sgm41512_chip *chip, u32 hr)
{
	u8 regval = 0;

	regval = sgm41512_closest_reg(SGM41512_SAFETMR_MIN, SGM41512_SAFETMR_MAX,
				    SGM41512_SAFETMR_STEP, hr);

	dprintf(CRITICAL, "%s time = %d(0x%02X)\n", __func__, hr, regval);

	return sgm41512_i2c_update_bits(chip, SGM41512_REG_SAFETMR,
				      regval << SGM41512_SAFETMR_SHIFT,
				      SGM41512_SAFETMR_MASK);
}


static int __sgm41512_set_mivrtrack(struct sgm41512_chip *chip, u32 mivr_track)
{
	if (mivr_track >= SGM41512_MIVRTRACK_MAX)
		mivr_track = SGM41512_MIVRTRACK_VBAT_300MV;

	dprintf(CRITICAL, "%s mivrtrack = %d\n", __func__, mivr_track);

	return sgm41512_i2c_update_bits(chip, SGM41512_REG_MIVRTRACK,
				      mivr_track << SGM41512_MIVRTRACK_SHIFT,
				      SGM41512_MIVRTRACK_MASK);
}

static int __sgm41512_enable_safe_tmr(struct sgm41512_chip *chip, bool en)
{
	dprintf(CRITICAL, "%s en = %d\n", __func__, en);
	return (en ? sgm41512_set_bit : sgm41512_clr_bit)
		(chip, SGM41512_REG_SAFETMR_EN, SGM41512_SAFETMR_EN_MASK);
}

static int __sgm41512_enable_te(struct sgm41512_chip *chip, bool en)
{
	dprintf(CRITICAL, "%s en = %d\n", __func__, en);
	return (en ? sgm41512_set_bit : sgm41512_clr_bit)
		(chip, SGM41512_REG_TE, SGM41512_TE_MASK);
}

static int __sgm41512_enable_hz(struct sgm41512_chip *chip, bool en)
{
	int ret = 0;

	dprintf(CRITICAL, "%s en = %d\n", __func__, en);

	if (ret < 0)
		return ret;

	/* Use force HZ */
	ret = (en ? sgm41512_set_bit : sgm41512_clr_bit)
		(chip, SGM41512_REG_HZ, SGM41512_FORCE_HZ_MASK);

	return ret;
}


static int __sgm41512_enable_chg(struct sgm41512_chip *chip, bool en)
{
	int ret = 0;
	struct sgm41512_desc *desc = chip->desc;

	dprintf(CRITICAL, "%s en = %d, chip_rev = %d\n",
			    __func__, en, chip->chip_rev);

	if (chip->ceb_gpio != U32_MAX) {
		mt_set_gpio_mode((chip->ceb_gpio | 0x80000000), GPIO_MODE_00);
		mt_set_gpio_dir((chip->ceb_gpio  | 0x80000000), GPIO_DIR_OUT);
		mt_set_gpio_out((chip->ceb_gpio  | 0x80000000), desc->ceb_invert ? en : !en);
	}

	ret = (en ? sgm41512_set_bit : sgm41512_clr_bit)
		(chip, SGM41512_REG_CHG_EN, SGM41512_CHG_EN_MASK);


	return ret;
}

static int sgm41512_enable_charging(struct mtk_charger_info *mchr_info, bool en)
{
	int ret = 0;
	
	struct sgm41512_chip *chip = (struct sgm41512_chip *)mchr_info;

	return __sgm41512_enable_chg(chip, en);
}

static int __sgm41512_kick_wdt(struct sgm41512_chip *chip)
{
	dprintf(CRITICAL, "%s\n", __func__);
	return sgm41512_set_bit(chip, SGM41512_REG_WDTCNTRST, SGM41512_WDTCNTRST_MASK);
}

static int __sgm41512_get_vbus_stat(struct sgm41512_chip *chip, u8 *stat)
{
	int ret = 0;
	u8 regval = 0;

	ret = sgm41512_i2c_read_byte(chip, SGM41512_REG_ICSTAT, &regval);
	if (ret < 0)
		return ret;
	*stat = (regval & SGM41512_VBUSSTAT_MASK) >> SGM41512_VBUSSTAT_SHIFT;

	return ret;
}


static int __sgm41512_get_mivr(struct sgm41512_chip *chip, u32 *mivr)
{
	int ret = 0;
	u8 regval = 0;

	ret = sgm41512_i2c_read_byte(chip, SGM41512_REG_MIVR, &regval);
	if (ret < 0)
		return ret;

	regval = (regval & SGM41512_MIVR_MASK) >> SGM41512_MIVR_SHIFT;
	*mivr = sgm41512_closest_value(SGM41512_MIVR_MIN, SGM41512_MIVR_MAX,
				     SGM41512_MIVR_STEP, regval);

	return ret;
}

static int __sgm41512_get_aicr(struct sgm41512_chip *chip, u32 *aicr)
{
	int ret = 0;
	u8 regval = 0;

	ret = sgm41512_i2c_read_byte(chip, SGM41512_REG_AICR, &regval);
	if (ret < 0)
		return ret;

	regval = (regval & SGM41512_AICR_MASK) >> SGM41512_AICR_SHIFT;
	*aicr = sgm41512_closest_value(SGM41512_AICR_MIN, SGM41512_AICR_MAX,
				     SGM41512_AICR_STEP, regval);
	if (*aicr > SGM41512_AICR_MIN && *aicr < SGM41512_AICR_MAX)
		*aicr -= SGM41512_AICR_STEP;

	return ret;
}

static int sgm41512_get_aicr(struct mtk_charger_info *mchr_info, u32 *aicr)
{
	int ret = 0;
	struct sgm41512_chip *chip = (struct sgm41512_chip *)mchr_info;

	ret = __sgm41512_get_aicr(chip, aicr);
	*aicr /= 1000;	/* uA --> mA */

	return ret;
}


static int __sgm41512_get_cv(struct sgm41512_chip *chip, u32 *cv)
{
	int ret = 0;
	u8 regval = 0;

	ret = sgm41512_i2c_read_byte(chip, SGM41512_REG_CV, &regval);
	if (ret < 0)
		return ret;

	regval = (regval & SGM41512_CV_MASK) >> SGM41512_CV_SHIFT;
	*cv = sgm41512_closest_value(SGM41512_CV_MIN, SGM41512_CV_MAX, SGM41512_CV_STEP,
				   regval);

	return ret;
}

static int __sgm41512_get_ichg(struct sgm41512_chip *chip, u32 *ichg)
{
	int ret = 0;
	u8 regval = 0;

	ret = sgm41512_i2c_read_byte(chip, SGM41512_REG_ICHG, &regval);
	if (ret < 0)
		return ret;

	regval = (regval & SGM41512_ICHG_MASK) >> SGM41512_ICHG_SHIFT;
	*ichg = sgm41512_closest_value(SGM41512_ICHG_MIN, SGM41512_ICHG_MAX,
				     SGM41512_ICHG_STEP, regval);

	return ret;
}

static int sgm41512_get_ichg(struct mtk_charger_info *mchr_info, u32 *ichg)
{
	int ret = 0;
	struct sgm41512_chip *chip = (struct sgm41512_chip *)mchr_info;

	ret = __sgm41512_get_ichg(chip, ichg);
	*ichg /= 1000;	/* uA --> mA */

	return ret;
}

static int __sgm41512_get_ieoc(struct sgm41512_chip *chip, u32 *ieoc)
{
	int ret = 0;
	u8 regval = 0;

	ret = sgm41512_i2c_read_byte(chip, SGM41512_REG_IEOC, &regval);
	if (ret < 0)
		return ret;

	regval = (regval & SGM41512_IEOC_MASK) >> SGM41512_IEOC_SHIFT;
	*ieoc = sgm41512_closest_value(SGM41512_IEOC_MIN, SGM41512_IEOC_MAX,
				     SGM41512_IEOC_STEP, regval);

	return ret;
}

static int __sgm41512_is_hz_enabled(struct sgm41512_chip *chip, bool *en)
{
	return sgm41512_i2c_test_bit(chip, SGM41512_REG_HZ,
				   SGM41512_FORCE_HZ_SHIFT, en);
}

static int __sgm41512_is_chg_enabled(struct sgm41512_chip *chip, bool *en)
{
	return sgm41512_i2c_test_bit(chip, SGM41512_REG_CHG_EN,
				   SGM41512_CHG_EN_SHIFT, en);
}

static int __sgm41512_get_chrg_stat(struct sgm41512_chip *chip,
				u8 *stat)
{
	int ret = 0;
	u8 regval = 0;

	ret = sgm41512_i2c_read_byte(chip, SGM41512_REG_ICSTAT, &regval);
	if (ret < 0)
		return ret;
	*stat = (regval & SGM41512_CHRGSTAT_MASK) >> SGM41512_CHRGSTAT_SHIFT;

	return ret;
}

static int __sgm41512_is_fault(struct sgm41512_chip *chip, bool *normal)
{
	int ret = 0;
	u8 regval = 0;

	ret = sgm41512_i2c_read_byte(chip, SGM41512_REG_FAULT, &regval);
	if (ret < 0)
		return ret;
	*normal = (regval == 0);

	return ret;
}

static int __sgm41512_get_ic_stat(struct sgm41512_chip *chip,
				enum sgm41512_ic_stat *stat)
{
	int ret = 0;
	u8 regval = 0;

	ret = sgm41512_i2c_read_byte(chip, SGM41512_REG_ICSTAT, &regval);
	if (ret < 0)
		return ret;
	*stat = (regval & SGM41512_ICSTAT_MASK) >> SGM41512_ICSTAT_SHIFT;

	return ret;
}

static int sgm41512_enable_power_path(struct sgm41512_chip *chip, bool en)
{
	int ret = 0;
/*
	dprintf(CRITICAL, "%s en = %d\n", __func__, en);

	if (ret < 0)
		return ret;

	ret = (en ? sgm41512_set_bit : sgm41512_clr_bit)
		(chip, SGM41512_REG_HZ, SGM41512_FORCE_HZ_MASK);
*/
	return ret;
}

static int sgm41512_reset_pumpx(struct mtk_charger_info *mchr_info, bool en)
{
	int ret = 0;
	
	return ret;
}

static int sgm41512_reset_register(struct sgm41512_chip *chip)
{
	int ret = 0;

	dprintf(chip->i2c_log_level, "%s\n", __func__);

	ret = sgm41512_set_bit(chip, SGM41512_REG_DEVID, SGM41512_REGRST_MASK);
	if (ret != I2C_OK)
		return ret;
	return __sgm41512_set_wdt(chip, 0);
}

static int sgm41512_enable_wdt(struct mtk_charger_info *mchr_info, bool en)
{
	struct sgm41512_chip *chip = (struct sgm41512_chip *)mchr_info;

	return __sgm41512_set_wdt(chip, en ? chip->desc->wdt : 0);
}

static int sgm41512_reset_wdt(struct mtk_charger_info *mchr_info, bool en)
{
	struct sgm41512_chip *chip = (struct sgm41512_chip *)mchr_info;

	return __sgm41512_kick_wdt(chip);
}

static int sgm41512_sw_reset(struct mtk_charger_info *mchr_info)
{
	struct sgm41512_chip *chip = (struct sgm41512_chip *)mchr_info;

	return sgm41512_reset_register(chip);
}


static int sgm41512_dump_register(struct mtk_charger_info *mchr_info)
{
	int ret = 0, i = 0;
	u32 mivr = 0, aicr = 0, cv = 0, ichg = 0, ieoc = 0;
	u8 chrg_stat = 0, vbus_stat = 0;
	bool chg_en = 0;
	u8 regval = 0;
	bool ic_normal = false;
	enum sgm41512_ic_stat ic_stat = SGM41512_ICSTAT_SLEEP;
	
	struct sgm41512_chip *chip = (struct sgm41512_chip *)mchr_info;
	
	ret = __sgm41512_kick_wdt(chip);

	ret = __sgm41512_get_mivr(chip, &mivr);
	ret = __sgm41512_get_aicr(chip, &aicr);
	ret = __sgm41512_get_cv(chip, &cv);
	ret = __sgm41512_get_ichg(chip, &ichg);
	ret = __sgm41512_get_ieoc(chip, &ieoc);
	ret = __sgm41512_is_chg_enabled(chip, &chg_en);

	ret = __sgm41512_get_chrg_stat(chip, &chrg_stat);
	ret = __sgm41512_get_vbus_stat(chip, &vbus_stat);
	ret = __sgm41512_get_ic_stat(chip, &ic_stat);

	ret = __sgm41512_is_fault(chip, &ic_normal);
	if (ret < 0)
		dprintf(CRITICAL, "%s check charger fault fail(%d)\n",
					      __func__, ret);

	if (ic_normal) {
		for (i = 0; i < ARRAY_SIZE(sgm41512_reg_addr); i++) {
			ret = sgm41512_i2c_read_byte(chip, sgm41512_reg_addr[i],
						   &regval);
			if (ret < 0)
				continue;
			dprintf(CRITICAL, "%s reg0x%02X = 0x%02X\n",
					      __func__, sgm41512_reg_addr[i],
					      regval);
		}
	}
	else{
		dprintf(CRITICAL, "%s check charger fault state error(%d)\n",__func__, ic_normal);
	}

	dprintf(CRITICAL, "%s MIVR = %dmV, AICR = %dmA\n",
		 __func__, mivr / 1000, aicr / 1000);

	dprintf(CRITICAL, "%s chg_en = %d, chrg_stat = %d, vbus_stat = %d ic_stat = %d\n",
		 __func__, chg_en, chrg_stat, vbus_stat,ic_stat);
		 
	dprintf(CRITICAL, "%s CV = %dmV, ICHG = %dmA, IEOC = %dmA\n",
		 __func__, cv / 1000, ichg / 1000, ieoc / 1000);

	return 0;
}

static int sgm41512_check_charging_mode(struct mtk_charger_info *mchr_info)
{
	int ret = 0;
	struct sgm41512_chip *chip = (struct sgm41512_chip *)mchr_info;
	static enum sgm41512_ic_stat pre_stat = SGM41512_ICSTAT_SLEEP;
	enum sgm41512_ic_stat cur_stat = SGM41512_ICSTAT_SLEEP;

	dprintf(chip->i2c_log_level, "%s\n", __func__);

	ret = __sgm41512_get_ic_stat(chip, &cur_stat);
	if (ret != I2C_OK)
		goto out;

	dprintf(chip->i2c_log_level, "%s in %d stat, previously in %d stat\n",__func__,cur_stat,pre_stat);
	if (cur_stat == pre_stat)
		goto out;
	
/*
	SGM41512_ICSTAT_SLEEP
	SGM41512_ICSTAT_PRECHG,
	SGM41512_ICSTAT_FASTCHG,
	SGM41512_ICSTAT_CHGDONE,
	SGM41512_ICSTAT_MAX,
*/
	pre_stat = cur_stat;
out:
	sgm41512_dump_register(mchr_info);
	return ret;
}

static bool sgm41512_is_hw_exist(struct sgm41512_chip *chip)
{
	int ret = 0;
	u8 info = 0, dev_id = 0, dev_rev = 0;

	ret = sgm41512_i2c_read_byte(chip, SGM41512_REG_DEVID, &info);
	if (ret != I2C_OK) {
		dprintf(CRITICAL, "%s get devinfo fail(%d)\n", __func__, ret);
		return false;
	}
	dev_id = (info & SGM41512_DEVID_MASK) >> SGM41512_DEVID_SHIFT;
/*prize add by zhaopengge 20210623 start*/
	if((dev_id != SGM41512_DEVID) && (dev_id != ETA6963_DEVID) && (dev_id != ETA6953_DEVID)){
/*prize add by zhaopengge 20210623 end*/
		dprintf(CRITICAL, "%s incorrect devid 0x%02X\n",__func__, dev_id);
		return false;
	}
	
	chip->mchr_info.device_id = 0;

	return true;
}

static int sgm41512_init_setting(struct sgm41512_chip *chip)
{
	int ret = 0;
	struct sgm41512_desc *desc = chip->desc;
	u8 evt[SGM41512_STREG_MAX] = {0};
	//unsigned int boot_mode = get_boot_mode();

	dprintf(CRITICAL, "%s\n", __func__);

	/* Clear all IRQs */
	ret = sgm41512_i2c_block_read(chip, SGM41512_REG_STATUS, SGM41512_STREG_MAX,
				    evt);
	if (ret < 0)
		dprintf(CRITICAL, "%s clear irq fail(%d)\n", __func__, ret);

	ret = __sgm41512_set_vac_ovp(chip, desc->vac_ovp);
	if (ret < 0)
		dprintf(CRITICAL, "%s set vac ovp fail(%d)\n",__func__, ret);

	ret = __sgm41512_set_mivr(chip, desc->mivr);
	if (ret < 0)
		dprintf(CRITICAL, "%s set mivr fail(%d)\n", __func__, ret);

	ret = __sgm41512_set_aicr(chip, desc->aicr);
	if (ret < 0)
		dprintf(CRITICAL, "%s set aicr fail(%d)\n", __func__, ret);

	ret = __sgm41512_set_cv(chip, desc->cv);
	if (ret < 0)
		dprintf(CRITICAL, "%s set cv fail(%d)\n", __func__, ret);

	ret = __sgm41512_set_ichg(chip, desc->ichg);
	if (ret < 0)
		dprintf(CRITICAL, "%s set ichg fail(%d)\n", __func__, ret);

	ret = __sgm41512_set_ieoc(chip, desc->ieoc);
	if (ret < 0)
		dprintf(CRITICAL, "%s set ieoc fail(%d)\n", __func__, ret);

	ret = __sgm41512_set_safe_tmr(chip, desc->safe_tmr);
	if (ret < 0)
		dprintf(CRITICAL, "%s set safe tmr fail(%d)\n",
				      __func__, ret);

	ret = __sgm41512_set_mivrtrack(chip, desc->mivr_track);
	if (ret < 0)
		dprintf(CRITICAL, "%s set mivrtrack fail(%d)\n",
				      __func__, ret);

	ret = __sgm41512_enable_safe_tmr(chip, desc->en_safe_tmr);
	if (ret < 0)
		dprintf(CRITICAL, "%s en safe tmr fail(%d)\n",
				      __func__, ret);

	ret = __sgm41512_enable_te(chip, desc->en_te);
	if (ret < 0)
		dprintf(CRITICAL, "%s en te fail(%d)\n", __func__, ret);

	/*
	 * Customization for MTK platform
	 * Primary charger: CHG_EN=1 at sgm41512_plug_in()
	 * Secondary charger: CHG_EN=1 at needed, e.x.: PE10, PE20, etc...
	 */
	ret = __sgm41512_enable_chg(chip, false);
	if (ret < 0)
		dprintf(CRITICAL, "%s dis chg fail(%d)\n", __func__, ret);

	/*
	 * Customization for MTK platform
	 * Primary charger: HZ=0 at sink vbus 5V with TCPC enabled
	 * Secondary charger: HZ=0 at needed, e.x.: PE10, PE20, etc...
	 */

	return 0;
}




static struct mtk_charger_ops sgm41512_mchr_ops = {
	.dump_register = sgm41512_dump_register,
	.enable_charging = sgm41512_enable_charging,
	.set_mivr = sgm41512_set_mivr,
	.get_aicr = sgm41512_get_aicr,
	.set_aicr = sgm41512_set_aicr,
	.get_ichg = sgm41512_get_ichg,
	.set_ichg = sgm41512_set_ichg,
	.enable_power_path = sgm41512_enable_power_path,
	.reset_pumpx = sgm41512_reset_pumpx,
	.enable_wdt = sgm41512_enable_wdt,
	.reset_wdt = sgm41512_reset_wdt,
	.sw_reset = sgm41512_sw_reset,
	.check_charging_mode = sgm41512_check_charging_mode,
};

static struct sgm41512_chip g_sgm41512_chip = {
	.mchr_info = {
		.name = "primary_charger",
		.alias_name = "sgm41512_charger",
		.device_id = -1,
		.mchr_ops = &sgm41512_mchr_ops,
	},
	.i2c = {
		.id = I2C7,
		.addr = SGM41512_SLAVE_ADDR,
		.mode = FS_MODE,
		.speed = 400,
	},
	.i2c_log_level = INFO,
	.hidden_mode_cnt = 0,
	.chip_rev = 0,
	.desc = &sgm41512_default_desc,
	.ceb_gpio = U32_MAX,
	.wkard_en = false,
};

static int sgm41512_parse_dt(struct sgm41512_chip *chip)
{	
	int offset = 0;
	const void *data = NULL;
	int len = 0;
	void *lk_drv_fdt = get_lk_overlayed_dtb();
	
	offset = fdt_node_offset_by_compatible(lk_drv_fdt,0,"sgm,swchg");
	if (offset < 0){
		dprintf(CRITICAL, "[sgm41512] Failed to find sgm,swchg in dtb\n");
		return -1;
	}else{
		data = fdt_getprop(lk_drv_fdt, offset, "rt,ceb_gpio_num", &len);
		if (len > 0 && data){
			chip->ceb_gpio = fdt32_to_cpu(*(unsigned int *)data);
			dprintf(INFO, "[sgm41512] rt,ceb_gpio_num(%d)\n",chip->ceb_gpio);
		}else{
			dprintf(INFO, "[sgm41512] get rt,ceb_gpio_num fail, default(%d)\n",chip->ceb_gpio);
		}
		
		offset = fdt_parent_offset(lk_drv_fdt,offset);
		data = fdt_getprop(lk_drv_fdt, offset, "id", &len);
		if (len > 0 && data){
			chip->i2c.id = fdt32_to_cpu(*(unsigned int *)data);
			dprintf(INFO, "[sgm41512] i2c id(%d)\n",chip->i2c.id);
		}else{
			dprintf(INFO, "[sgm41512] get i2c id fail, default(%d)\n",chip->i2c.id);
		}
	}
	return 0;
}

int sgm41512_probe(void)
{
	int ret = 0;

	dprintf(CRITICAL, "%s (%s)\n", __func__, SGM41512_LK_DRV_VERSION);

	ret = sgm41512_parse_dt(&g_sgm41512_chip);
	if (ret < 0) {
		dprintf(CRITICAL, "%s: parse dt failed\n", __func__);
	}
	
	if (!sgm41512_is_hw_exist(&g_sgm41512_chip))
		return -ENODEV;

	ret = sgm41512_reset_register(&g_sgm41512_chip);
	if (ret != I2C_OK)
		return ret;

	ret = sgm41512_init_setting(&g_sgm41512_chip);
	if (ret != I2C_OK)
		return ret;

	ret = mtk_charger_set_info(&g_sgm41512_chip.mchr_info);
	if (ret < 0)
		return ret;

	sgm41512_dump_register(&g_sgm41512_chip.mchr_info);
	
	dprintf(CRITICAL, "%s successfully\n", __func__);
	return 0;
}