/******************** (C) COPYRIGHT 2024 陈苏阳 ********************************
* File Name          :  cgms_debug_db.h
* Author             :  陈苏阳
* CPU Type         	 :  NRF52840
* IDE                :  IAR 8.11
* Version            :  V1.0
* Date               :  5/3/2024
* Description        :  
*******************************************************************************/

/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __CGMS_DEBUG_DB_H
#define __CGMS_DEBUG_DB_H

/* Includes ------------------------------------------------------------------*/
#include "stdint.h"
#include "stdbool.h"
/* Private define ------------------------------------------------------------*/

#define CGMS_DEBUG_MAX_KV_NUM                                       (15)        // 支持的最大键值对数量

/* Private typedef -----------------------------------------------------------*/

typedef struct
{
    char cKey[12];
    uint8_t ucData[4];
}__attribute__((packed))cgms_debug_kv_info_t;

typedef struct
{
    cgms_debug_kv_info_t KvArray[CGMS_DEBUG_MAX_KV_NUM];   // kv数组
    uint8_t ucNone[14];
    uint16_t usCrc;                                        // crc效验
}__attribute__((packed))cgms_debug_info_t;

/* Private variables ---------------------------------------------------------*/



/* Private function prototypes -----------------------------------------------*/
void cgms_debug_db_write(void);
bool cgms_debug_db_read(void);
void cgms_debug_db_print(void);
void cgms_debug_db_clear(void);

bool cgms_debug_write_kv(char* pKey, uint8_t* pData);
bool cgms_debug_read_kv(char* pKey, uint8_t* pData);
bool debug_get_kv(uint8_t ucIndex, char* pKey, uint8_t* pData);
void cgms_debug_clear_all_kv(void);


#endif /* __CGMS_DEBUG_DB_H */

/******************* (C) COPYRIGHT 2024 陈苏阳 **** END OF FILE ****************/

