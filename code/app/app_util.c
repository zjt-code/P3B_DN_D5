/******************** (C) COPYRIGHT 2023 陈苏阳 ********************************
* File Name          :  app_util.c
* Author             :  陈苏阳
* CPU Type         	 :  NRF52832
* IDE                :  Keil
* Version            :  V1.0
* Date               :  16/10/2023
* Description        :  
*******************************************************************************/

/* Includes ------------------------------------------------------------------*/

#include<app.h>
#include"app_util.h"
#include "stdio.h"
#include "string.h"


/* Private variables ---------------------------------------------------------*/

const uint8_t g_ucDayOfMon[12] = { 31,28,31,30,31,30,31,31,30,31,30,31 };

        
/* Private function prototypes -----------------------------------------------*/



/* Private functions ---------------------------------------------------------*/


/**@brief Function for encoding a uint16 value.
 *
 * @param[in]   value            Value to be encoded.
 * @param[out]  p_encoded_data   Buffer where the encoded data is to be written.
 *
 * @return      Number of bytes written.
 */
uint8_t uint16_encode(uint16_t value, uint8_t* p_encoded_data)
{
    p_encoded_data[0] = (uint8_t)((value & 0x00FF) >> 0);
    p_encoded_data[1] = (uint8_t)((value & 0xFF00) >> 8);
    return sizeof(uint16_t);
}


/**@brief Function for decoding a uint16 value.
 *
 * @param[in]   p_encoded_data   Buffer where the encoded data is stored.
 *
 * @return      Decoded value.
 */
uint16_t uint16_decode(const uint8_t* p_encoded_data)
{
    return ((((uint16_t)((uint8_t*)p_encoded_data)[0])) |
        (((uint16_t)((uint8_t*)p_encoded_data)[1]) << 8));
}



/**@brief Function for encoding a three-byte value.
 *
 * @param[in]   value            Value to be encoded.
 * @param[out]  p_encoded_data   Buffer where the encoded data is to be written.
 *
 * @return      Number of bytes written.
 */
uint8_t uint24_encode(uint32_t value, uint8_t* p_encoded_data)
{
    p_encoded_data[0] = (uint8_t)((value & 0x000000FF) >> 0);
    p_encoded_data[1] = (uint8_t)((value & 0x0000FF00) >> 8);
    p_encoded_data[2] = (uint8_t)((value & 0x00FF0000) >> 16);
    return 3;
}



void swmLogHex(uint32_t ucLevl, uint8_t* pData, uint32_t uiLen)
{
    uint8_t ucTmpBuffer[128];
    memset(ucTmpBuffer, 0x00, sizeof(ucTmpBuffer));
    uint8_t ucOneByteHexStr[4];
    for (uint32_t i = 0; i < uiLen; i++)
    {
        sprintf((char*)ucOneByteHexStr, "%02X ", pData[i]);
        strcat((char*)ucTmpBuffer, (char*)ucOneByteHexStr);
    }
    swmLogInfo("%s\r\n", ucTmpBuffer);
}




/*******************************************************************************
*                           陈苏阳@2019-06-11
* Function Name  :  get_second_time
* Description    :  日期时间转时间戳
* Input          :  time_info_t* pDateTime
* Output         :  None
* Return         :  uint32_t
*******************************************************************************/
uint32_t get_second_time(time_info_t* pDateTime)
{
    uint16_t iYear;
    uint8_t iMon, iDay, iHour, iMin, iSec;
    iYear = pDateTime->year;
    iMon = pDateTime->month;
    iDay = pDateTime->day;
    iHour = pDateTime->hour;
    iMin = pDateTime->minute;
    iSec = pDateTime->sec;

    uint16_t i, Cyear = 0;
    uint32_t CountDay = 0;

    for (i = 1970; i < iYear; i++)
    {
        if (((i % 4 == 0) && (i % 100 != 0)) || (i % 400 == 0)) Cyear++;
    }
    CountDay = Cyear * 366 + (iYear - 1970 - Cyear) * 365;
    for (i = 1; i < iMon; i++)
    {
        if ((i == 2) && (((iYear % 4 == 0) && (iYear % 100 != 0)) || (iYear % 400 == 0)))
            CountDay += 29;
        else
            CountDay += g_ucDayOfMon[i - 1];
    }
    CountDay += (iDay - 1);

    CountDay = CountDay * SECOND_OF_DAY + (uint32_t)iHour * 3600 + (uint32_t)iMin * 60 + iSec;
    return CountDay;
}



/*******************************************************************************
*                           陈苏阳@2019-06-11
* Function Name  :  get_date_time_from_second
* Description    :  时间戳转日期时间
* Input          :  unsigned int lSec
* Input          :  time_info_t* pDateTime
* Output         :  None
* Return         :  void
*******************************************************************************/
void get_date_time_from_second(uint32_t lSec, time_info_t* pDateTime)
{
    uint16_t i, j, iDay;
    uint32_t lDay;

    lDay = lSec / SECOND_OF_DAY;
    lSec = lSec % SECOND_OF_DAY;

    i = 1970;
    while (lDay > 365)
    {
        if (((i % 4 == 0) && (i % 100 != 0)) || (i % 400 == 0))
            lDay -= 366;
        else
            lDay -= 365;
        i++;
    }
    if ((lDay == 365) && !(((i % 4 == 0) && (i % 100 != 0)) || (i % 400 == 0)))
    {
        lDay -= 365;
        i++;
    }
    pDateTime->year = i;
    for (j = 0; j < 12; j++)
    {
        if ((j == 1) && (((i % 4 == 0) && (i % 100 != 0)) || (i % 400 == 0)))
            iDay = 29;
        else
            iDay = g_ucDayOfMon[j];
        if (lDay >= iDay) lDay -= iDay;
        else break;
    }
    pDateTime->month = j + 1;
    pDateTime->day = lDay + 1;
    pDateTime->hour = (lSec / 3600) % 24;
    pDateTime->minute = (lSec % 3600) / 60;
    pDateTime->sec = (lSec % 3600) % 60;
}




/******************* (C) COPYRIGHT 2023 陈苏阳 **** END OF FILE ****************/
