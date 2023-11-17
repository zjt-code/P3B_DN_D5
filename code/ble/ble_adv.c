/******************** (C) COPYRIGHT 2023 陈苏阳 ********************************
* File Name          :  ble_adv.c
* Author             :  陈苏阳
* CPU Type         	 :  NRF52832
* IDE                :  Keil
* Version            :  V1.0
* Date               :  24/10/2023
* Description        :
*******************************************************************************/

/* Includes ------------------------------------------------------------------*/
#include "ble_adv.h"
#include "string.h"
#include "app_global.h"
#include <elog.h>
#include "cgms_prm.h"
/* Private variables ---------------------------------------------------------*/

char g_cAdvSnStr[12];


/* Private function prototypes -----------------------------------------------*/



/* Private functions ---------------------------------------------------------*/



/*******************************************************************************
*                           陈苏阳@2023-10-24
* Function Name  :  ble_adv_generate_adv_data
* Description    :  生成广播数据
* Input          :  uint8_t * pAdvDataBuffer
* Input          :  uint8_t * pAdvDataLen
* Output         :  None
* Return         :  void
*******************************************************************************/
void ble_adv_generate_adv_data(uint8_t* pAdvDataBuffer, uint8_t* pAdvDataLen)
{
    uint16_t usCompanyID = APP_COMPANY_ID;
    uint16_t usAppCompleteList16BitUuid = APP_COMPLETE_LIST_16_BIT_UUID;
    uint8_t ucDataIndex = 0;
    if (pAdvDataBuffer)
    {
        // 获取存储的SN
        cgms_prm_get_sn(g_cAdvSnStr);

        // 添加Flag
        pAdvDataBuffer[ucDataIndex++] = 0x02;
        pAdvDataBuffer[ucDataIndex++] = 0x01;
        pAdvDataBuffer[ucDataIndex++] = 0x06;

        // 添加设备名
        pAdvDataBuffer[ucDataIndex++] = 10 + 1;
        pAdvDataBuffer[ucDataIndex++] = 0x09;
        memcpy(&pAdvDataBuffer[ucDataIndex], g_cAdvSnStr, 10);
        ucDataIndex += 10;

        // 添加16bit UUID
        pAdvDataBuffer[ucDataIndex++] = sizeof(usAppCompleteList16BitUuid) + 1;
        pAdvDataBuffer[ucDataIndex++] = 0x03;
        memcpy(&pAdvDataBuffer[ucDataIndex], &usAppCompleteList16BitUuid, sizeof(usAppCompleteList16BitUuid));
        ucDataIndex += sizeof(usAppCompleteList16BitUuid);

        // 添加厂商自定义数据
        pAdvDataBuffer[ucDataIndex++] = sizeof(usCompanyID) + 1;
        pAdvDataBuffer[ucDataIndex++] = 0xFF;
        memcpy(&pAdvDataBuffer[ucDataIndex], &usCompanyID, sizeof(usCompanyID));
        ucDataIndex += sizeof(usCompanyID);

        log_i("ble_adv_generate_adv_data");
        elog_hexdump("adv_Data", 8, pAdvDataBuffer, ucDataIndex);
        if (pAdvDataLen)*pAdvDataLen = ucDataIndex;
    }
}


/*******************************************************************************
*                           陈苏阳@2023-10-24
* Function Name  :  ble_adv_generate_adv_scan_response_data
* Description    :  生成广播扫描回应包数据
* Input          :  uint8_t * pAdvScanRespDataBuffer
* Input          :  uint8_t * pAdvScanRespDataLen
* Output         :  None
* Return         :  void
*******************************************************************************/
void ble_adv_generate_adv_scan_response_data(uint8_t* pAdvScanRespDataBuffer, uint8_t* pAdvScanRespDataLen)
{
    if (pAdvScanRespDataLen)*pAdvScanRespDataLen = 0;
}




/******************* (C) COPYRIGHT 2023 陈苏阳 **** END OF FILE ****************/




