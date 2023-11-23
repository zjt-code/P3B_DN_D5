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
prm_t g_PrmDb;


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
    ucResult = cgms_prm_flash_erase(0, 1);
    if (ucResult)
    {
        return ucResult;
    }
    // 写入参数结构体,最后补4byte,防止结构体大小没法对齐
	ucResult = cgms_prm_flash_write(0, (sizeof(g_PrmDb) / 4) + 1, (uint32_t*)&g_PrmDb);
    if (ucResult)
    {
        return ucResult;
    }
    return 0;
}

/*******************************************************************************
*                           陈苏阳@2022-12-26
* Function Name  :  cgms_prm_get_sn
* Description    :  获取设备SN
* Input          :  unsigned char * buff
* Output         :  None
* Return         :  ret_code_t
*******************************************************************************/
ret_code_t cgms_prm_get_sn(unsigned char* buff)
{
    uint32_t uiFlag;

    // 如果SN值非法,或者CRC错误,则恢复默认SN
    if ((0x00 != do_crc((uint8_t*)&g_PrmDb.P4, sizeof(P4_t))) || g_PrmDb.P4.prmWMY[0] == 0xFF)
    {
        g_PrmDb.P4.prmWMY[0] = 'Z'; //A
        g_PrmDb.P4.prmWMY[1] = 'H'; //B
        g_PrmDb.P4.prmWMY[2] = 'T'; //C
        g_PrmDb.P4.prmWMY[3] = 0;  //null,end of string
        log_e("can not read SN,use default SN");
        g_PrmDb.P4.SN = 3;
        uiFlag = RET_CODE_FAIL;
    }
    else
    {
        uiFlag = RET_CODE_SUCCESS;
    }
    sprintf((char*)buff, "JN-%s%04d", (unsigned char*)g_PrmDb.P4.prmWMY, g_PrmDb.P4.SN);
    log_e("SN:%s", buff);
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

    memset(g_ucSn, 0, sizeof(g_ucSn));
    cgms_prm_get_sn(g_ucSn);
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
