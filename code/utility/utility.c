/******************** (C) COPYRIGHT 2024 ≥¬À’—Ù ********************************
* File Name          :  utility.c
* Author             :  ≥¬À’—Ù
* CPU Type         	 :  NRF52840
* IDE                :  IAR 8.11
* Version            :  V1.0
* Date               :  19/9/2024
* Description        :  
*******************************************************************************/

/* Includes ------------------------------------------------------------------*/
#if !defined(LOG_TAG)
#define LOG_TAG                    "utility"
#endif
#undef LOG_LVL
#define LOG_LVL                    ELOG_LVL_INFO

#include <elog.h>
#include "em_emu.h"
#include <elog.h>
#include "cgms_aes128.h"
/* Private variables ---------------------------------------------------------*/


        
/* Private function prototypes -----------------------------------------------*/



/* Private functions ---------------------------------------------------------*/


/*******************************************************************************
*                           ≥¬À’—Ù@2024-10-11
* Function Name  :  datapacket_padding_and_encrpty
* Description    :   ˝æ›∞¸ÃÓ≥‰“‘º∞º”√‹
* Input          :  uint8_t * pDestin
* Input          :  uint8_t * pSource
* Input          :  uint8_t ucSrcLen
* Output         :  None
* Return         :  uint8_t
*******************************************************************************/
uint8_t datapacket_padding_and_encrpty(uint8_t* pDestin, uint8_t* pSpSourcerc, uint8_t ucSrcLen)
{
#if ((USE_BLE_PROTOCOL==P3_ENCRYPT_PROTOCOL) ||(USE_BLE_PROTOCOL==GN_2_PROTOCOL))
    uint8_t ucDatapacketBuffer[16];
    uint8_t ucCipher[16];
    memcpy(ucDatapacketBuffer, pSpSourcerc, ucSrcLen);
    mbedtls_aes_pkcspadding(ucDatapacketBuffer, ucSrcLen);
    cgms_aes128_encrpty(ucDatapacketBuffer, pDestin);
    return 16;
#else
    memcpy(pDestin, pSpSourcerc, ucSrcLen);
    return ucSrcLen;
#endif
}


/*******************************************************************************
*                           ≥¬À’—Ù@2024-04-17
* Function Name  :  print_reset_cause
* Description    :  ¥Ú”°∏¥Œª‘≠“Ú
* Input          :  uint32_t uiResetCause
* Output         :  None
* Return         :  void
*******************************************************************************/
void print_reset_cause(uint32_t uiResetCause)
{
    if (uiResetCause & EMU_RSTCAUSE_POR)
    {
        log_i("ResetCause:Power On Reset");
    }
    if (uiResetCause & EMU_RSTCAUSE_PIN)
    {
        log_w("ResetCause:Pin Reset");
    }
    if (uiResetCause & EMU_RSTCAUSE_EM4)
    {
        log_i("ResetCause:EM4 Wakeup Reset");
    }
    if (uiResetCause & EMU_RSTCAUSE_WDOG0)
    {
        log_e("ResetCause:Watchdog 0 Reset");
    }
    if (uiResetCause & EMU_RSTCAUSE_LOCKUP)
    {
        log_e("ResetCause:M33 Core Lockup Reset");
    }
    if (uiResetCause & EMU_RSTCAUSE_SYSREQ)
    {
        log_w("ResetCause:M33 Core Sys Reset");
    }
    if (uiResetCause & EMU_RSTCAUSE_DVDDBOD)
    {
        log_e("ResetCause:Shift value for EMU_DVDDBOD");
    }
    if (uiResetCause & EMU_RSTCAUSE_DVDDLEBOD)
    {
        log_e("ResetCause:LEBOD Reset");
    }
    if (uiResetCause & EMU_RSTCAUSE_DECBOD)
    {
        log_e("ResetCause:LVBOD Reset");
    }
    if (uiResetCause & EMU_RSTCAUSE_AVDDBOD)
    {
        log_e("ResetCause:LEBOD1 Reset");
    }
    if (uiResetCause & EMU_RSTCAUSE_IOVDD0BOD)
    {
        log_e("ResetCause:LEBOD2 Reset");
    }
    if (uiResetCause & EMU_RSTCAUSE_DCI)
    {
        log_e("ResetCause:DCI reset");
    }
    if (uiResetCause & EMU_RSTCAUSE_BOOSTON)
    {
        log_w("ResetCause:BOOST_EN pin reset");
    }
    if (uiResetCause & EMU_RSTCAUSE_VREGIN)
    {
        log_e("ResetCause:DCDC VREGIN comparator");
    }
}

/******************* (C) COPYRIGHT 2024 ≥¬À’—Ù **** END OF FILE ****************/




