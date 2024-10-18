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
