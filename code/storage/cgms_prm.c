#if !defined(LOG_TAG)
    #define LOG_TAG                    "cgms_prm"
#endif
#undef LOG_LVL
#define LOG_LVL                    ELOG_LVL_INFO




#include <stdbool.h>
#include "cgms_prm.h"
#include "cgms_crc.h"
//#include "simplegluco.h"
#include "stdio.h"
#include "cgms_prm_port.h"
#include "string.h"
#include <elog.h>
uint8_t g_ucSn[11] = { 'J','N','-','X','X', 'X', '0', '0', '0', '0',0x00 };
prm_t g_PrmDb __attribute__((aligned(4)));
user_usage_data_t g_UserUsageData __attribute__((aligned(4)));


/*******************************************************************************
*                           陈苏阳@2022-12-26
* Function Name  :  cgms_prm_db_write_flash
* Description    :  将参数数据库写入Flash
* Output         :  None
* Return         :  ret_code_t
*******************************************************************************/
ret_code_t cgms_prm_db_write_flash()
{
    uint8_t ucResult;

    // 擦除参数所在的Flash区域
    ucResult = cgms_prm_flash_erase_sector(0);
    if (ucResult)
    {
        log_e("cgms_prm_flash_erase_sector fail:%d", ucResult);
        return ucResult;
    }
    // 写入参数结构体
    ucResult = cgms_prm_flash_write(0, (uint8_t*)&g_PrmDb, sizeof(g_PrmDb));
    if (ucResult)
    {
        log_e("cgms_prm_flash_write fail:%d", ucResult);
        return ucResult;
    }
    else
    {
        // 读取参数
        cgms_prm_flash_read(0, (uint8_t*)&g_PrmDb, sizeof(g_PrmDb));
        // 如果CRC错误,则将数据设为默认值
        if (0x00 != do_crc((uint8_t*)&g_PrmDb, sizeof(g_PrmDb)))
        {
            log_e("cgms_prm_flash_write read back fail");
        }
    }
    return 0;
}

/*******************************************************************************
*                           陈苏阳@2022-12-26
* Function Name  :  cgms_prm_get_sn
* Description    :  获取设备SN
* Input          :  char * buff
* Output         :  None
* Return         :  ret_code_t
*******************************************************************************/
ret_code_t cgms_prm_get_sn(char* buff)
{
    static bool bLogOutFlag = false;
    uint32_t uiFlag;

    // 如果SN值非法,或者CRC错误,则恢复默认SN
    if ((0x00 != do_crc((uint8_t*)&g_PrmDb, sizeof(g_PrmDb))) || g_PrmDb.prmWMY[0] == 0xFF)
    {
        g_PrmDb.prmWMY[0] = 'A'; //A
        g_PrmDb.prmWMY[1] = 'B'; //B
        g_PrmDb.prmWMY[2] = 'C'; //C
        g_PrmDb.prmWMY[3] = 0;  //null,end of string
        if (bLogOutFlag == false)
        {
            log_w("can not read SN,use default SN");
        }
        g_PrmDb.SN =0;
        uiFlag = RET_CODE_FAIL;
    }
    else
    {
        uiFlag = RET_CODE_SUCCESS;
    }
    sprintf(buff, "%s-%s%04d", BLE_ADV_NAME_PREFIXES,(unsigned char*)g_PrmDb.prmWMY, g_PrmDb.SN);
    if (bLogOutFlag == false)
    {
        log_i("SN:%s", buff);
        bLogOutFlag = true;
    }
    return uiFlag;
}



/*******************************************************************************
*                           陈苏阳@2022-12-29
* Function Name  :  cgms_prm_db_power_on_init
* Description    :  参数数据库上电初始化
* Input          :  void
* Output         :  None
* Return         :  void
*******************************************************************************/
void cgms_prm_db_power_on_init(void)
{
    // 读取参数
    cgms_prm_flash_read(0, (uint8_t*)&g_PrmDb, sizeof(g_PrmDb));

    // 如果CRC错误,则将数据设为默认值
    if (0x00 != do_crc((uint8_t*)&g_PrmDb, sizeof(g_PrmDb)))
    {
        log_w("flash prm data crc fail");
        g_PrmDb.AdcB = 0;
        g_PrmDb.AdcK = 1000;
        g_PrmDb.DacVolOffset = 0;
    }
    else
    {
        log_i("flash prm data read done");
    }

    cgms_prm_get_sn((char*)g_ucSn);

    // 读取用户使用数据
    cgms_prm_db_read_user_usage_data(&g_UserUsageData);
}

/*******************************************************************************
*                           陈苏阳@2022-12-29
* Function Name  :  cgms_prm_get_sn_p
* Description    :  获取SN字符串指针
* Input          :  void
* Output         :  None
* Return         :  uint8_t*
*******************************************************************************/
uint8_t* cgms_prm_get_sn_p(void)
{
    return g_ucSn;
}

/*******************************************************************************
*                           陈苏阳@2024-10-22
* Function Name  :  cgms_prm_db_write_user_usage_data
* Description    :  写入用户使用数据
* Input          :  user_usage_data_t * pData
* Output         :  None
* Return         :  ret_code_t
*******************************************************************************/
ret_code_t cgms_prm_db_write_user_usage_data(user_usage_data_t* pData)
{
  uint8_t ucResult;
    if (pData)
    {
        // 擦除参数所在的Flash区域
        ucResult = cgms_prm_flash_erase_sector(4096);
        if (ucResult)
        {
            log_e("cgms_prm_flash_erase_sector fail:%d", ucResult);
            return ucResult;
        }
        user_usage_data_t WriteData;
        memcpy(&WriteData, pData, sizeof(user_usage_data_t));
        // 计算当前要写入的数据的CRC
        uint16_t usCrc = do_crc((uint8_t*)&WriteData, sizeof(user_usage_data_t) - 2);
        WriteData.usCrc16 = usCrc;
        // 写入参数结构体
        ucResult = cgms_prm_flash_write(4096, (uint8_t*)&WriteData, sizeof(user_usage_data_t));
        
        return ucResult;
    }
    else
    {
        return RET_CODE_FAIL;
    }
}

/*******************************************************************************
*                           陈苏阳@2024-10-22
* Function Name  :  cgms_prm_db_read_user_usage_data
* Description    :  读取用户使用数据
* Input          :  user_usage_data_t * pData
* Output         :  None
* Return         :  ret_code_t
*******************************************************************************/
ret_code_t cgms_prm_db_read_user_usage_data(user_usage_data_t* pData)
{
    // 读取参数
    cgms_prm_flash_read(4096, (uint8_t*)pData, sizeof(user_usage_data_t));

    // 如果CRC错误,则将数据设为默认值
    if (0x00 != do_crc((uint8_t*)pData, sizeof(user_usage_data_t)))
    {
        log_w("flash user usage data crc fail");
        memset(pData, 0x00, sizeof(user_usage_data_t));
        return RET_CODE_FAIL;
    }
    else
    {
        log_i("flash user usage data read done");
        return RET_CODE_SUCCESS;
    }

}

/*******************************************************************************
*                           陈苏阳@2024-10-24
* Function Name  :  cgms_prm_db_print_user_usage_data
* Description    :  打印用户使用数据
* Input          :  user_usage_data_t * pData
* Output         :  None
* Return         :  void
*******************************************************************************/
void cgms_prm_db_print_user_usage_data(user_usage_data_t* pData)
{
    if (pData)
    {
        log_i("ucDataValidFlag:%d", pData->ucDataValidFlag);
        if (pData->ucDataValidFlag == 0x01)
        {
            log_i("LastCgmState:%d", pData->LastCgmState);
            log_i("fUseSensorK:%f", pData->fUseSensorK);
            log_i("uiLastCgmSessionStartTime:%d", pData->uiLastCgmSessionStartTime);
            log_i("ucLastTimeZone:%d", pData->ucLastTimeZone);
            log_i("ucLastStartBy:%d", pData->ucLastStartBy);
            log_i("ucLastStartByVersion:%d,%d,%d", pData->ucLastStartByVersion[2], pData->ucLastStartByVersion[1], pData->ucLastStartByVersion[0]);
            log_i("usLastPassword:0x%04X", pData->usLastPassword);
            log_i("ucCgmSessionCnt:%d", pData->ucCgmSessionCnt);
        }
    }
}