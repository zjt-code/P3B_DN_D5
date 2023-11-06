/******************** (C) COPYRIGHT 2023 陈苏阳 ********************************
* File Name          :  cgms_db_port.h
* Author             :  陈苏阳
* CPU Type         	 :  NRF52832
* IDE                :  Keil
* Version            :  V1.0
* Date               :  23/10/2023
* Description        :  
*******************************************************************************/

/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __CGMS_DB_PORT_H
#define __CGMS_DB_PORT_H

/* Includes ------------------------------------------------------------------*/
#include "stdint.h"

/* Private define ------------------------------------------------------------*/



/* Private typedef -----------------------------------------------------------*/
typedef struct
{
    uint16_t usSectorByteSize;                      // 单个扇区大小(单位:byte)
    uint32_t uiAddroffset;                          // 地址基准偏移
    uint8_t ucAlignAtNByte;                         // 按N字节对齐
}cgms_db_port_info_t;

/* Private variables ---------------------------------------------------------*/
uint8_t cgms_db_flash_init(void);
cgms_db_port_info_t* cgm_db_flash_get_info(void);
uint8_t cgms_db_flash_erase(uint32_t uiAddr, uint32_t uiEraseSize);
uint8_t cgms_db_flash_write(uint32_t uiAddr, uint8_t* pWriteData, uint16_t usWriteLen);
uint8_t cgms_db_flash_read(uint32_t uiAddr, uint8_t* pReadData, uint16_t usReadLen);
/* Private function prototypes -----------------------------------------------*/

#endif /* __CGMS_DB_PORT_H */

/******************* (C) COPYRIGHT 2023 陈苏阳 **** END OF FILE ****************/

