/******************** (C) COPYRIGHT 2023 陈苏阳 ********************************
* File Name          :  app.c
* Author             :  陈苏阳
* CPU Type         	 :  NRF52832
* IDE                :  Keil
* Version            :  V1.0
* Date               :  24/10/2023
* Description        :  
*******************************************************************************/

/* Includes ------------------------------------------------------------------*/
#if !defined(LOG_TAG)
    #define LOG_TAG                    "app"
#endif
#undef LOG_LVL
#define LOG_LVL                    ELOG_LVL_INFO


#include "em_common.h"
#include "app_assert.h"
#include <elog.h>
#include "sl_bluetooth.h"
#include "gatt_db.h"
#include "app.h"
#include "app_global.h"
#include "ble_adv.h"
#include "cgms_socp.h"
#include "cgms_racp.h"
#include "cgms_prm.h"
#include "ble_customss.h"
#include "app_glucose_meas.h"
#include "cgms_meas.h"
/* Private variables ---------------------------------------------------------*/
static uint8_t g_ucAdvertisingSetHandle = 0xff;     // BLE广播集句柄
static uint8_t g_ucAdvDataBuffer[31];               // BLE广播内容
static uint8_t g_ucAdvDataLen;                      // BLE广播内容长度
        
/* Private function prototypes -----------------------------------------------*/



/* Private functions ---------------------------------------------------------*/




/*******************************************************************************
*                           陈苏阳@2023-10-24
* Function Name  :  sl_bt_on_event
* Description    :  BLE协议栈事件处理函数
* Input          :  sl_bt_msg_t * evt
* Output         :  None
* Return         :  void
*******************************************************************************/
void sl_bt_on_event(sl_bt_msg_t* evt)
{
    sl_status_t sc;
    //log_i("sl_bt_on_event:0x%X", SL_BT_MSG_ID(evt->header));
    switch (SL_BT_MSG_ID(evt->header))
    {
        // BLE协议栈初始化完成事件
    case sl_bt_evt_system_boot_id:
    {
        log_i("sl_bt_on_event:sl_bt_evt_system_boot_id");
        int16_t min_pwr, max_pwr;
        // 设置TX发射功率为0dB
        sc = sl_bt_system_set_tx_power(0, 0, &min_pwr, &max_pwr);
        app_assert_status(sc);

        // 创建一个BLE广播集
        sc = sl_bt_advertiser_create_set(&g_ucAdvertisingSetHandle);
        app_assert_status(sc);

        // 生成BLE广播数据
        ble_adv_generate_adv_data(g_ucAdvDataBuffer, &g_ucAdvDataLen);

        // 设置广播数据
        sc = sl_bt_legacy_advertiser_set_data(g_ucAdvertisingSetHandle, 0, g_ucAdvDataLen, (uint8_t*)g_ucAdvDataBuffer);
        //sc = sl_bt_advertiser_set_data(g_ucAdvertisingSetHandle, sl_bt_advertiser_scan_response_packet, g_ucAdvDataLen, g_ucAdvDataBuffer);
        app_assert_status(sc);

        // 设置广播的时间参数
        sc = sl_bt_advertiser_set_timing(
            g_ucAdvertisingSetHandle,
            1600, // min. adv. interval (milliseconds * 1.6)
            1600, // max. adv. interval (milliseconds * 1.6)
            0,   // adv. duration
            0);  // max. num. adv. events
        app_assert_status(sc);

        // 开始BLE广播
        sc = sl_bt_legacy_advertiser_start(g_ucAdvertisingSetHandle, sl_bt_advertiser_connectable_scannable);
        app_assert_status(sc);


        // 更新CGM状态Char内容
        att_get_cgm_status()->ucRunStatus = app_global_get_app_state()->status;
        att_update_cgm_status_char_data_crc();

        // 更新CGM启动时间char内容
        att_update_start_time_char_data_crc();

        // 工作时间默认14天
        att_get_feature()->ucWorkTime = 14;

        // 数据发送间隔默认3分钟
        att_get_feature()->ucSampleTime = 3;

        // 支持CRC
        att_get_feature()->ucCrcSupported = 0x01;

        // 更新Feature char的内容
        att_update_feature_char_data_crc();

        break;
    }

    // -------------------------------
    // BLE连接事件
    case sl_bt_evt_connection_opened_id:
    {
        log_i("Connection opened.");

        // 更新GATT中的设备名称
        static char cSn[14];
        memset(cSn, 0x00, sizeof(cSn));
        cgms_prm_get_sn(cSn);
        sl_bt_gatt_server_write_attribute_value(gattdb_device_name, 0, sizeof(cSn), cSn);

        // 调用应用层的回调
        app_event_ble_connected_callback(evt->data.evt_connection_opened.connection);
        break;
    }

    // -------------------------------
    // BLE连接参数更新事件
    case sl_bt_evt_connection_parameters_id:
    {
        uint8_t  connection = evt->data.evt_connection_parameters.connection;    // Connection handle
        uint16_t interval = evt->data.evt_connection_parameters.interval;      // Connection interval. Time = Value x 1.25 ms
        uint16_t latency = evt->data.evt_connection_parameters.latency;       // Peripheral latency (how many connection intervals the peripheral can skip)
        uint16_t timeout = evt->data.evt_connection_parameters.timeout;       // Supervision timeout. Time = Value x 10 ms

        // 调用APP层的连接参数更新回调
        app_event_ble_param_updated_callback(connection, interval, latency, timeout);
        log_i("Connection parameter update.:%d,latency:%d,timeout:%d", interval, latency, timeout);

        break;
    }

    // -------------------------------
    // BLE断开连接事件
    case sl_bt_evt_connection_closed_id:
    {
        log_i("Connection closed.");

        // 调用应用层的回调
        app_event_ble_disconnect_callback(evt->data.evt_connection_closed.connection);

        // 设置广播数据
        sc = sl_bt_legacy_advertiser_set_data(g_ucAdvertisingSetHandle, 0, g_ucAdvDataLen, (uint8_t*)g_ucAdvDataBuffer);
        //sc = sl_bt_advertiser_set_data(g_ucAdvertisingSetHandle, sl_bt_advertiser_scan_response_packet, g_ucAdvDataLen, g_ucAdvDataBuffer);
        app_assert_status(sc);

        // 重新开始广播
        sc = sl_bt_legacy_advertiser_start(g_ucAdvertisingSetHandle, sl_bt_advertiser_connectable_scannable);
        app_assert_status(sc);

        break;
    }

    // -------------------------------
    // 远程GATT客户端更改了本地GATT数据库中的属性值事件
    case sl_bt_evt_gatt_server_attribute_value_id:
    {
        uint8_t ucDataRecvBuffer[20];
        size_t ucDataRecvLen = 0;
        uint16_t usAttributeHandle = evt->data.evt_gatt_server_attribute_value.attribute;

        // Read characteristic value.
        sc = sl_bt_gatt_server_read_attribute_value(usAttributeHandle, 0, sizeof(ucDataRecvBuffer), &ucDataRecvLen, ucDataRecvBuffer);
        log_i("weite char:%d", usAttributeHandle);
        elog_hexdump("data", 8, ucDataRecvBuffer, ucDataRecvLen);
        switch (usAttributeHandle)
        {
            // 写SOCP
        case gattdb_cgm_specific_ops_control_point:
        {
            // 创建BLE事件信息结构体,并填充
            ble_event_info_t BleEventInfo;
            BleEventInfo.ucConidx = evt->data.evt_gatt_server_attribute_value.connection;
            BleEventInfo.usHandle = evt->data.evt_gatt_server_attribute_value.attribute;
            // 调用回调
            on_socp_value_write(BleEventInfo, ucDataRecvLen, ucDataRecvBuffer);
            break;
        }
        // 写RACP
        case gattdb_record_access_control_point:
        {
            // 创建BLE事件信息结构体,并填充连接句柄
            ble_event_info_t BleEventInfo;
            BleEventInfo.ucConidx = evt->data.evt_gatt_server_attribute_value.connection;
            BleEventInfo.usHandle = evt->data.evt_gatt_server_attribute_value.attribute;

            // 调用回调
            on_racp_value_write(BleEventInfo, ucDataRecvLen, ucDataRecvBuffer);
            break;
        }
        default:
            break;
        }

        break;
    }
    /*
    case sl_bt_evt_gatt_server_characteristic_status_id:
    {
        uint16_t usCharacteristic = evt->data.evt_gatt_server_characteristic_status.characteristic;
        uint8_t status_flags = evt->data.evt_gatt_server_characteristic_status.status_flags;
        uint16_t client_config_flags = evt->data.evt_gatt_server_characteristic_status.client_config_flags;
        log_i("sl_bt_evt_gatt_server_characteristic_status_id:%d,%d,%d", usCharacteristic, status_flags, client_config_flags);
        break;
    }
    */
    // -------------------------------
    // 当远端设备启用或禁用通知事件
    case sl_bt_evt_gatt_server_characteristic_status_id:
    {
        uint16_t usCharacteristicHandle = evt->data.evt_gatt_server_characteristic_status.characteristic;
        log_i("change char notify enable:%d,%d,%d", usCharacteristicHandle, evt->data.evt_gatt_server_characteristic_status.client_config_flags, evt->data.evt_gatt_server_characteristic_status.status_flags);
        switch (usCharacteristicHandle)
        {
        case gattdb_cgm_measurement:
        {
            // 如果是char配置
            if (evt->data.evt_gatt_server_characteristic_status.status_flags == sl_bt_gatt_server_client_config)
            {
                // 如果notify被启用
                if (evt->data.evt_gatt_server_characteristic_status.client_config_flags != sl_bt_gatt_disable)
                {
                    ble_meas_notify_enable();
                }
                else
                {
                    ble_meas_notify_disable();
                }
            }
            // 如果是接收确认
            else if (evt->data.evt_gatt_server_characteristic_status.status_flags == sl_bt_gatt_server_confirmation)
            {
                app_global_get_app_state()->bSentMeasSuccess = true;
            }
            break;
        }
        case gattdb_cgm_status:
        {

            break;
        }
        case gattdb_record_access_control_point:
        {
            // 如果是char配置
            if (evt->data.evt_gatt_server_characteristic_status.status_flags == sl_bt_gatt_server_client_config)
            {
                // 如果notify被启用
                if (evt->data.evt_gatt_server_characteristic_status.client_config_flags != sl_bt_gatt_disable)
                {
                    ble_racp_notify_enable();
                }
                else
                {
                    ble_racp_notify_disable();
                }
            }
            // 如果是接收确认
            else if (evt->data.evt_gatt_server_characteristic_status.status_flags == sl_bt_gatt_server_confirmation)
            {
                app_global_get_app_state()->bSentRacpSuccess = true;
            }
            break;
        }
        case gattdb_cgm_specific_ops_control_point:
        {
            // 如果是char配置
            if (evt->data.evt_gatt_server_characteristic_status.status_flags == sl_bt_gatt_server_client_config)
            {
                // 如果notify被启用
                if (evt->data.evt_gatt_server_characteristic_status.client_config_flags != sl_bt_gatt_disable)
                {
                    ble_socp_notify_enable();
                }
                else
                {
                    ble_socp_notify_disable();
                }
            }
            // 如果是接收确认
            else if (evt->data.evt_gatt_server_characteristic_status.status_flags == sl_bt_gatt_server_confirmation)
            {
                app_global_get_app_state()->bSentSocpSuccess = true;
            }
            break;
        }
        default:
            break;
        }

        break;
    }
    case sl_bt_evt_system_external_signal_id:
        // 处理事件
        event_handler(evt->data.evt_system_external_signal.extsignals);
    default:
    {
        break;
    }
    }
}




/******************* (C) COPYRIGHT 2023 陈苏阳 **** END OF FILE ****************/

