/*****************************************************************************
*
* Filename:
* ---------
*   cw2017_fuel_guau.h
*
* Project:
* --------
*   Android
*
* Description:
* ------------
*   cw2017 header file
*
* Author:
* -------
*
****************************************************************************/

#ifndef _CW2017_SW_H_
#define _CW2017_SW_H_


/**********************************************************
  *
  *   [Extern Function] 
  *
  *********************************************************/
//CON0----------------------------------------------------
extern int cw2017_init(void);
extern int cw2017_get_capacity(void);
extern int cw2017_get_voltage(void);
//CON1----------------------------------------------------

//prize-add-sunshuai-2017 Multi-Battery Solution-20200222-start
#if defined(CONFIG_MTK_CW2017_BATTERY_ID_AUXADC)
struct cw2017batinfo {
   int bat_first_startrange;
   int bat_first_endrange;
   int bat_second_startrange;
   int bat_second_endrange;
   int bat_third_startrange;
   int bat_third_endrange;
   int bat_channel_num;
   int bat_id;
};

struct cw2017batinfo cw2017fuelguage;
static char *fuelguage_name[] = {
	"batinfo_first", "batinfo_second", "batinfo_third","batinfo_default"
};
#endif
//prize-add-sunshuai-2017 Multi-Battery Solution-20200222-end

#endif // _CW2017_SW_H_

