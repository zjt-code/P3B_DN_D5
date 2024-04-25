/******************** (C) COPYRIGHT 2023 陈苏阳 ********************************
* File Name          :  cgms_sst.h
* Author             :  陈苏阳
* CPU Type         	 :  RSL15
* IDE                :  Onsemi IDE
* Version            :  V1.0
* Date               :  28/4/2023
* Description        :  
*******************************************************************************/

/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __CGMS_SST_H
#define __CGMS_SST_H

/* Includes ------------------------------------------------------------------*/
#include "app.h"
#include "app_util.h"
/* Private define ------------------------------------------------------------*/


#define	IsLeapYear(yr)	(!((yr) % 400) || (((yr) % 100) && !((yr) % 4)))        // 判断是否是闰年
#define	YearLength(yr)	(IsLeapYear(yr) ? 366 : 365)                            // 获取当年的天数

#define MAXCALCTICKS  ((uint16_t)(13105))
#define	BEGYEAR	        2000     // UTC started at 00:00:00 January 1, 2000
#define	DAY             86400UL  // 24 hours * 60 minutes * 60 seconds


#define NRF_BLE_CGMS_SST_LEN    9

/* Private typedef -----------------------------------------------------------*/

typedef struct
{
    time_info_t time_info;
    uint8_t  time_zone; /**< Time zone. */
}__attribute__((packed)) ble_date_time_t;// 日期时间信息结构体

typedef struct
{
    ble_date_time_t date_time; /**< Date and time. */
    uint8_t         dst;       /**< Daylight saving time. */
}__attribute__((packed)) ble_cgms_sst_t;// CGM会话启动时间信息结构体

/* Private variables ---------------------------------------------------------*/
extern ble_cgms_sst_t g_mSST;

/* Private function prototypes -----------------------------------------------*/
void cgms_update_sst_and_time_zone(uint16_t usYear, uint8_t ucMonth, uint8_t ucDay, uint8_t ucHour, uint8_t ucMinute, uint8_t ucSecond, uint8_t ucTimeZone, uint8_t ucDataSaveingTime);
uint32_t cgms_sst_init(void);
void cgms_sst_recover(ble_cgms_sst_t SST);
#endif /* __CGMS_SST_H */

/******************* (C) COPYRIGHT 2023 陈苏阳 **** END OF FILE ****************/
