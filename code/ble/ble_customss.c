/******************** (C) COPYRIGHT 2022 陈苏阳 ********************************
* File Name          :  ble_customss.c
* Author             :  陈苏阳
* CPU Type         	 :  NRF52832
* IDE                :  Keil
* Version            :  V1.0
* Date               :  15/12/2022
* Description        :
*******************************************************************************/

/* Includes ------------------------------------------------------------------*/

#include "gatt_db.h"
#include <string.h>
#include <ble_customss.h>
#include <stdio.h>
#include "cgms_sst.h"
#include "app_util.h"
#include "cgms_socp.h"
#include "cgms_racp.h"
//#include "cgms_db.h"
#include "cgms_crc.h"
#include "sl_bt_api.h"
/* Private variables ---------------------------------------------------------*/

static struct ble_customss_service_att_database  cs_att_db;
/* Private function prototypes -----------------------------------------------*/



/* Private functions ---------------------------------------------------------*/

/*******************************************************************************
*                           陈苏阳@2022-12-16
* Function Name  :  att_initialize
* Description    :  初始化自定义BLE服务
* Input          :  void
* Output         :  None
* Return         :  void
*******************************************************************************/
void att_initialize(void)
{
    memset(&cs_att_db, 0, sizeof(struct ble_customss_service_att_database));
}


/*******************************************************************************
*                           陈苏阳@2023-10-19
* Function Name  :  att_set_start_handle
* Description    :
* Input          :  uint16_t handle
* Output         :  None
* Return         :  void
*******************************************************************************/
void att_set_start_handle(uint16_t handle)
{
    app_global_get_app_state()->start_hdl = handle;
}


/*******************************************************************************
*                           陈苏阳@2023-10-19
* Function Name  :  att_get_att_handle
* Description    :
* Input          :  uint16_t attindx
* Output         :  None
* Return         :  uint16_t
*******************************************************************************/
uint16_t att_get_att_handle(uint16_t attindx)
{
    return (app_global_get_app_state()->start_hdl + attindx);
}

/*******************************************************************************
*                           陈苏阳@2023-10-19
* Function Name  :  att_get_start_handle
* Description    :
* Input          :  void
* Output         :  None
* Return         :  uint16_t
*******************************************************************************/
uint16_t att_get_start_handle(void)
{
    return app_global_get_app_state()->start_hdl;
}


/*******************************************************************************
*                           陈苏阳@2023-10-19
* Function Name  :  att_get_start_time
* Description    :  获取ATT表中的启动时间Char数据的结构体指针
* Input          :  void
* Output         :  None
* Return         :  cgm_session_start_time_char_data_t*
*******************************************************************************/
cgm_session_start_time_char_data_t* att_get_start_time(void)
{
    return &(cs_att_db.CgmSessionStartTimeValue);
}


#if (USE_BLE_PROTOCOL==GN_2_PROTOCOL)
/*******************************************************************************
*                           陈苏阳@2023-10-19
* Function Name  :  att_update_start_time_char_data_crc
* Description    :  更新ATT表中的启动时间Char数据的CRC
* Input          :  void
* Output         :  None
* Return         :  void
*******************************************************************************/
void att_update_start_time_char_data_crc(void)
{
    // 计算CRC
    uint16_t usCrc16 = do_crc(&(cs_att_db.CgmSessionStartTimeValue), sizeof(cs_att_db.CgmSessionStartTimeValue) - sizeof(cs_att_db.CgmSessionStartTimeValue.usCrc16));
    // 赋值CRC
    cs_att_db.CgmSessionStartTimeValue.usCrc16 = usCrc16;

    sl_bt_gatt_server_write_attribute_value(gattdb_cgm_session_start_time, 0, sizeof(cs_att_db.CgmSessionStartTimeValue), &(cs_att_db.CgmSessionStartTimeValue));
}
#else
/*******************************************************************************
*                           陈苏阳@2023-10-19
* Function Name  :  att_update_start_time_char_data
* Description    :  更新ATT表中的启动时间Char数据
* Input          :  void
* Output         :  None
* Return         :  void
*******************************************************************************/
void att_update_start_time_char_data(void)
{
    sl_bt_gatt_server_write_attribute_value(gattdb_cgm_session_start_time, 0, sizeof(cs_att_db.CgmSessionStartTimeValue), (uint8_t*)&(cs_att_db.CgmSessionStartTimeValue));
}
#endif



#if (USE_BLE_PROTOCOL==GN_2_PROTOCOL)
/*******************************************************************************
*                           陈苏阳@2023-10-19
* Function Name  :  att_update_cgm_status_char_data_crc
* Description    :  更新ATT表中的CGM状态Char数据的CRC
* Input          :  void
* Output         :  None
* Return         :  void
*******************************************************************************/
void att_update_cgm_status_char_data_crc(void)
{
    // 计算CRC
    uint16_t usCrc16 = do_crc(&(cs_att_db.CgmStatusValue), sizeof(cs_att_db.CgmStatusValue) - sizeof(cs_att_db.CgmStatusValue.usCrc16));
    // 赋值CRC
    cs_att_db.CgmStatusValue.usCrc16 = usCrc16;

    sl_bt_gatt_server_write_attribute_value(gattdb_cgm_status, 0, sizeof(cs_att_db.CgmStatusValue), &(cs_att_db.CgmStatusValue));
}
#else
/*******************************************************************************
*                           陈苏阳@2023-10-19
* Function Name  :  att_get_cgm_status
* Description    :  获取ATT表中的CGM状态Char数据的结构体指针
* Input          :  void
* Output         :  None
* Return         :  cgm_status_char_data_t*
*******************************************************************************/
cgm_status_char_data_t* att_get_cgm_status(void)
{
    return &(cs_att_db.CgmStatusValue);
}
#endif

#if ((USE_BLE_PROTOCOL==P3_ENCRYPT_PROTOCOL)||(USE_BLE_PROTOCOL==GN_2_PROTOCOL))
/*******************************************************************************
*                           陈苏阳@2024-09-10
* Function Name  :  att_get_feature
* Description    :  获取ATT表中的特征Char数据
* Input          :  void
* Output         :  None
* Return         :  cgm_feature_char_data_t*
*******************************************************************************/
cgm_feature_char_data_t* att_get_feature(void)
{
    return &(cs_att_db.CgmFeatureValue);
}

/*******************************************************************************
*                           陈苏阳@2023-10-19
* Function Name  :  att_update_feature_char_data_crc
* Description    :  更新ATT表中的特征Char数据的CRC
* Input          :  void
* Output         :  None
* Return         :  void
*******************************************************************************/
void att_update_feature_char_data_crc(void)
{
    // 计算CRC
    uint16_t usCrc16 = do_crc(&(cs_att_db.CgmFeatureValue), sizeof(cs_att_db.CgmFeatureValue) - sizeof(cs_att_db.CgmFeatureValue.usCrc16));
    // 赋值CRC
    cs_att_db.CgmFeatureValue.usCrc16 = usCrc16;

    sl_bt_gatt_server_write_attribute_value(gattdb_cgm_feature, 0, sizeof(cs_att_db.CgmFeatureValue), &(cs_att_db.CgmFeatureValue));
}
#endif

/*******************************************************************************
*                           陈苏阳@2024-03-12
* Function Name  :  att_update_cgm_status_char_data
* Description    :  更新ATT表中的CGM状态Char数据
* Input          :  void
* Output         :  None
* Return         :  void
*******************************************************************************/
void att_update_cgm_status_char_data(void)
{
    sl_bt_gatt_server_write_attribute_value(gattdb_cgm_status, 0, sizeof(cs_att_db.CgmStatusValue), (uint8_t*)&(cs_att_db.CgmStatusValue));
}


/*******************************************************************************
*                           陈苏阳@2024-03-12
* Function Name  :  att_update_feature_char_data
* Description    :  更新ATT表中的特征Char数据
* Input          :  void
* Output         :  None
* Return         :  void
*******************************************************************************/
void att_update_feature_char_data(void)
{
    sl_bt_gatt_server_write_attribute_value(gattdb_cgm_feature, 0, sizeof(cs_att_db.CgmFeatureValue), (uint8_t*)&(cs_att_db.CgmFeatureValue));
}


/*******************************************************************************
*                           陈苏阳@2023-10-19
* Function Name  :  att_update_notify_indication
* Description    :
* Input          :  uint16_t handle
* Input          :  uint16_t value
* Output         :  None
* Return         :  void
*******************************************************************************/
void att_update_notify_indication(uint16_t handle, uint16_t value)
{

    if (handle == CS_IDX_CGM_MEASUREMENT_CCC)
    {

        cs_att_db.CgmMeasurementCccdValue[0] = value;
        cs_att_db.CgmMeasurementCccdValue[1] = value >> 8;

    }
    if (handle == CS_IDX_RECORD_ACCESS_CONTROL_CCC)
    {

        cs_att_db.CgmRecordAccessControlCccdValue[0] = value;
        cs_att_db.CgmRecordAccessControlCccdValue[1] = value >> 8;

    }
    if (handle == CS_IDX_CGM_SPECIFIC_OPS_CCC)
    {
        cs_att_db.CgmSpecificOpsCccdValue[0] = value;
        cs_att_db.CgmSpecificOpsCccdValue[1] = value >> 8;

    }

}



/*******************************************************************************
*                           陈苏阳@2023-10-19
* Function Name  :  att_disable_notify_indication
* Description    :
* Input          :  void
* Output         :  None
* Return         :  void
*******************************************************************************/
void att_disable_notify_indication(void)
{
    cs_att_db.CgmMeasurementCccdValue[0] = 0;
    cs_att_db.CgmMeasurementCccdValue[1] = 0;
    cs_att_db.CgmRecordAccessControlCccdValue[0] = 0;
    cs_att_db.CgmRecordAccessControlCccdValue[1] = 0;

    cs_att_db.CgmSpecificOpsCccdValue[0] = 0;
    cs_att_db.CgmSpecificOpsCccdValue[1] = 0;
}


/*******************************************************************************
*                           陈苏阳@2023-10-19
* Function Name  :  customss_clear_cccd
* Description    :
* Input          :  void
* Output         :  None
* Return         :  void
*******************************************************************************/
void customss_clear_cccd(void)
{
    for (uint8_t i = 0; i < 2; i++)
    {
        cs_att_db.CgmMeasurementCccdValue[i] = 0;
        cs_att_db.CgmSpecificOpsCccdValue[i] = 0;
        cs_att_db.CgmRecordAccessControlCccdValue[i] = 0;
        cs_att_db.CgmStatusCccdValue[i] = 0;
    }
}


/******************* (C) COPYRIGHT 2022 陈苏阳 **** END OF FILE ****************/

