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
 *
 * The following software/firmware and/or related documentation ("MediaTek Software")
 * have been modified by MediaTek Inc. All revisions are subject to any receiver\'s
 * applicable license agreements with MediaTek Inc.
 */


#include <platform/mt_typedefs.h>
#include <platform/errno.h>
#include <printf.h>
#include <string.h>
#include "mt_pumpexpress.h"
#include "mtk_charger.h"
#include "mtk_charger_intf.h"
#include "mt6370_pmu_charger.h"
#include "bq25601.h"
#include "mt6360_pmu_charger.h"
#include "mt6362_pmu_charger.h"
#include "rt9466.h"
#include "rt9467.h"
#include "rt9471.h"
#include "sgm41512.h" //prize add by huarui, 20210312
#include "hl7005all_charger.h" //prize add by huarui, 20210312

#define MAX_MCHR_INFO_SIZE	2

static struct mtk_charger_info *mchr_info_list[MAX_MCHR_INFO_SIZE];
static int (*mtk_charger_init_list[])(void) = {
#if defined(MTK_BQ25601_SUPPORT)
	bq25601_chg_probe,
#else
	mt6370_chg_probe,
	mt6360_chg_probe,
	mt6362_chg_probe,
	rt9471_probe,
	rt9466_probe,
	rt9467_probe,
	sgm41512_probe,//prize add by huarui, 20210312
	hl7005all_chg_probe,//prize add by huarui, 20210312
#endif
};

void __attribute__((weak)) charger_driver_init(void)
{
	dprintf(CRITICAL, "%s: Please implement charger_driver_init() if you don't use mtk charger.\n", __func__);
}

int mtk_charger_init(void)
{
	int i = 0, ret = 0;
	int size = ARRAY_SIZE(mtk_charger_init_list);
	int (*charger_init)(void) = NULL;

	init_charger_custom_data();
	ret = init_cust_data_from_dt();

	if (ret) {
		charger_driver_init();
		return ret;
	}

	if(!is_disable_charger()) {
		for (i = 0; i < size; i++) {
			charger_init = mtk_charger_init_list[i];
			ret = (*charger_init)();
			if (ret < 0)
				dprintf(CRITICAL,
					"%s: fail to init charger(%d), ret = %d\n",
					__func__, i, ret);
		}
	}
	else
		dprintf(CRITICAL, "%s: skip mtk_charger_init\n", __func__);

	reset_pe_adapter();
	reset_pd_adapter();
	check_charger_volt();

	return ret;
}

int mtk_charger_set_info(struct mtk_charger_info *mchr_info)
{
	int i = 0;

	for (i = 0; i < MAX_MCHR_INFO_SIZE; i++) {
		if (!mchr_info_list[i]) {
			mchr_info_list[i] = mchr_info;
			break;
		}
	}

	if (i == MAX_MCHR_INFO_SIZE)
		return -ENOMEM;

	return 0;
}

struct mtk_charger_info *mtk_charger_get_by_name(const char *name)
{
	int i = 0;
	struct mtk_charger_info *mchr_info = NULL;

	for (i = 0; i < MAX_MCHR_INFO_SIZE; i++) {
		if (!mchr_info_list[i])
			continue;
		mchr_info = mchr_info_list[i];
		if (strcmp(mchr_info->name, name) == 0)
			return mchr_info;
	}

	return NULL;
}

#define MTK_CHG_CHECK_INFO_OPS(info, ops) \
	do { \
		if (!info) \
			return -EINVAL; \
		if (!ops) \
			return -EINVAL; \
	} while (0)

int mtk_charger_dump_register(struct mtk_charger_info *mchr_info)
{
	MTK_CHG_CHECK_INFO_OPS(mchr_info, mchr_info->mchr_ops);

	if (mchr_info->mchr_ops->dump_register)
		return mchr_info->mchr_ops->dump_register(mchr_info);
	else
		return -EOPNOTSUPP;
}

int mtk_charger_enable_charging(struct mtk_charger_info *mchr_info,
	bool enable)
{
	MTK_CHG_CHECK_INFO_OPS(mchr_info, mchr_info->mchr_ops);

	if (mchr_info->mchr_ops->enable_charging)
		return mchr_info->mchr_ops->enable_charging(mchr_info, enable);
	else
		return -EOPNOTSUPP;
}

int mtk_charger_get_ichg(struct mtk_charger_info *mchr_info,
	unsigned int *ichg)
{
	MTK_CHG_CHECK_INFO_OPS(mchr_info, mchr_info->mchr_ops);

	if (mchr_info->mchr_ops->get_ichg)
		return mchr_info->mchr_ops->get_ichg(mchr_info, ichg);
	else
		return -EOPNOTSUPP;
}

int mtk_charger_set_ichg(struct mtk_charger_info *mchr_info,
	unsigned int ichg)
{
	MTK_CHG_CHECK_INFO_OPS(mchr_info, mchr_info->mchr_ops);

	if (mchr_info->mchr_ops->set_ichg)
		return mchr_info->mchr_ops->set_ichg(mchr_info, ichg);
	else
		return -EOPNOTSUPP;
}

int mtk_charger_get_aicr(struct mtk_charger_info *mchr_info,
	unsigned int *aicr)
{
	MTK_CHG_CHECK_INFO_OPS(mchr_info, mchr_info->mchr_ops);

	if (mchr_info->mchr_ops->get_aicr)
		return mchr_info->mchr_ops->get_aicr(mchr_info, aicr);
	else
		return -EOPNOTSUPP;
}

int mtk_charger_set_aicr(struct mtk_charger_info *mchr_info,
	unsigned int aicr)
{
	MTK_CHG_CHECK_INFO_OPS(mchr_info, mchr_info->mchr_ops);

	if (mchr_info->mchr_ops->set_aicr)
		return mchr_info->mchr_ops->set_aicr(mchr_info, aicr);
	else
		return -EOPNOTSUPP;
}

int mtk_charger_get_adc(struct mtk_charger_info *mchr_info, int chan,
	unsigned int *data)
{
	MTK_CHG_CHECK_INFO_OPS(mchr_info, mchr_info->mchr_ops);

	if (mchr_info->mchr_ops->get_adc)
		return mchr_info->mchr_ops->get_adc(mchr_info, chan, data);
	else
		return -EOPNOTSUPP;
}

int mtk_charger_get_vbus(struct mtk_charger_info *mchr_info,
	unsigned int *vbus)
{
	MTK_CHG_CHECK_INFO_OPS(mchr_info, mchr_info->mchr_ops);

	if (mchr_info->mchr_ops->get_adc)
		return mchr_info->mchr_ops->get_vbus(mchr_info, vbus);
	else
		return -EOPNOTSUPP;
}

int mtk_charger_set_mivr(struct mtk_charger_info *mchr_info,
	unsigned int mivr)
{
	MTK_CHG_CHECK_INFO_OPS(mchr_info, mchr_info->mchr_ops);

	if (mchr_info->mchr_ops->set_mivr)
		return mchr_info->mchr_ops->set_mivr(mchr_info, mivr);
	else
		return -EOPNOTSUPP;
}

int mtk_charger_enable_power_path(struct mtk_charger_info *mchr_info,
	bool enable)
{
	MTK_CHG_CHECK_INFO_OPS(mchr_info, mchr_info->mchr_ops);

	if (mchr_info->mchr_ops->enable_power_path)
		return mchr_info->mchr_ops->enable_power_path(mchr_info, enable);
	else
		return -EOPNOTSUPP;
}

int mtk_charger_reset_pumpx(struct mtk_charger_info *mchr_info,
	bool reset)
{
	MTK_CHG_CHECK_INFO_OPS(mchr_info, mchr_info->mchr_ops);

	if (mchr_info->mchr_ops->reset_pumpx)
		return mchr_info->mchr_ops->reset_pumpx(mchr_info, reset);
	else
		return -EOPNOTSUPP;
}

int mtk_charger_enable_wdt(struct mtk_charger_info *mchr_info, bool en)
{
	MTK_CHG_CHECK_INFO_OPS(mchr_info, mchr_info->mchr_ops);

	if (mchr_info->mchr_ops->enable_wdt)
		return mchr_info->mchr_ops->enable_wdt(mchr_info, en);
	else
		return -EOPNOTSUPP;
}

int mtk_charger_reset_wdt(struct mtk_charger_info *mchr_info)
{
	MTK_CHG_CHECK_INFO_OPS(mchr_info, mchr_info->mchr_ops);

	if (mchr_info->mchr_ops->reset_wdt)
		return mchr_info->mchr_ops->reset_wdt(mchr_info);
	else
		return -EOPNOTSUPP;
}

int mtk_charger_sw_reset(struct mtk_charger_info *mchr_info)
{
	MTK_CHG_CHECK_INFO_OPS(mchr_info, mchr_info->mchr_ops);

	if (mchr_info->mchr_ops->sw_reset)
		return mchr_info->mchr_ops->sw_reset(mchr_info);
	else
		return -EOPNOTSUPP;
}

int mtk_charger_check_charging_mode(struct mtk_charger_info *mchr_info)
{
	MTK_CHG_CHECK_INFO_OPS(mchr_info, mchr_info->mchr_ops);

	if (mchr_info->mchr_ops->check_charging_mode)
		return mchr_info->mchr_ops->check_charging_mode(mchr_info);
	else
		return -EOPNOTSUPP;
}

int mtk_charger_enable_discharge(struct mtk_charger_info *mchr_info, bool en)
{
	MTK_CHG_CHECK_INFO_OPS(mchr_info, mchr_info->mchr_ops);

	if (mchr_info->mchr_ops->enable_discharge)
		return mchr_info->mchr_ops->enable_discharge(mchr_info, en);
	else
		return -EOPNOTSUPP;
}

/* The following APIs are from MTK's originall solution */
void pumpex_reset_adapter(void)
{
	unsigned int aicr = 500;
	int ret = 0;
	struct mtk_charger_info *mchr_info = NULL;

	mchr_info = mtk_charger_get_by_name("primary_charger");
	if (!mchr_info)
		return;

	ret = mtk_charger_set_aicr(mchr_info, aicr);
	if (ret < 0)
		dprintf(CRITICAL, "%s: set aicr failed, ret = %d\n",
			__func__, ret);

	dprintf(CRITICAL, "PE+ adapter reset back to 5v.\r\n");
}

void pumpex_reset_adapter_enable(int en)
{
	int ret = 0;
	struct mtk_charger_info *mchr_info = NULL;

	mchr_info = mtk_charger_get_by_name("primary_charger");
	if (!mchr_info)
		return;

	ret = mtk_charger_reset_pumpx(mchr_info, en);
	if (ret < 0)
		dprintf(CRITICAL, "%s: reset pumpx failed, ret = %d\n",
			__func__, ret);

	dprintf(CRITICAL, "PE+ adapter reset back to 5v.\r\n");
}

