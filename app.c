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

#include "em_common.h"
#include "app_assert.h"
#include "app_log.h"
#include "sl_bluetooth.h"
#include "gatt_db.h"
#include "app.h"

/* Private variables ---------------------------------------------------------*/
static uint8_t advertising_set_handle = 0xff;

        
/* Private function prototypes -----------------------------------------------*/



/* Private functions ---------------------------------------------------------*/






/******************* (C) COPYRIGHT 2023 陈苏阳 **** END OF FILE ****************/






// The advertising set handle allocated from Bluetooth stack.

/**************************************************************************//**
 * Application Init.
 *****************************************************************************/
SL_WEAK void app_init(void)
{
  /////////////////////////////////////////////////////////////////////////////
  // Put your additional application init code here!                         //
  // This is called once during start-up.                                    //
  /////////////////////////////////////////////////////////////////////////////
}

/**************************************************************************//**
 * Application Process Action.
 *****************************************************************************/
SL_WEAK void app_process_action(void)
{


  /////////////////////////////////////////////////////////////////////////////
  // Put your additional application code here!                              //
  // This is called infinitely.                                              //
  // Do not call blocking functions from here!                               //
  /////////////////////////////////////////////////////////////////////////////
}

/*******************************************************************************
*                           陈苏阳@2023-10-24
* Function Name  :  sl_bt_on_event
* Description    :  BLE协议栈事件处理函数
* Input          :  sl_bt_msg_t * evt
* Output         :  None
* Return         :  void
*******************************************************************************/
void sl_bt_on_event(sl_bt_msg_t *evt)
{
  sl_status_t sc;

  switch (SL_BT_MSG_ID(evt->header)) 
  {
    // BLE协议栈初始化完成事件
    case sl_bt_evt_system_boot_id:
    {
        // 创建一个BLE广播集
        sc = sl_bt_advertiser_create_set(&advertising_set_handle);
        app_assert_status(sc);

        // 生成BLE广播数据
        sc = sl_bt_legacy_advertiser_generate_data(advertising_set_handle,sl_bt_advertiser_general_discoverable);
        app_assert_status(sc);

        // 设置广播的时间参数
        sc = sl_bt_advertiser_set_timing(
            advertising_set_handle,
            160, // min. adv. interval (milliseconds * 1.6)
            160, // max. adv. interval (milliseconds * 1.6)
            0,   // adv. duration
            0);  // max. num. adv. events
        app_assert_status(sc);

        // 开始BLE广播
        sc = sl_bt_legacy_advertiser_start(advertising_set_handle,sl_bt_advertiser_connectable_scannable);
        app_assert_status(sc);

        break;
    }

    // -------------------------------
    // BLE连接事件
    case sl_bt_evt_connection_opened_id:
    {
        app_log_info("Connection opened.\n");
        break;
    }

    // -------------------------------
    // BLE断开连接事件
    case sl_bt_evt_connection_closed_id:
    {
        app_log_info("Connection closed.\n");

        // 生成广播数据包
        sc = sl_bt_legacy_advertiser_generate_data(advertising_set_handle,
            sl_bt_advertiser_general_discoverable);
        app_assert_status(sc);

        // 重新开始广播
        sc = sl_bt_legacy_advertiser_start(advertising_set_handle,sl_bt_advertiser_connectable_scannable);
        app_assert_status(sc);

        break;
    }

    // -------------------------------
    // 远程GATT客户端更改了本地GATT数据库中的属性值事件
    case sl_bt_evt_gatt_server_attribute_value_id:
    {
        // The value of the gattdb_led_control characteristic was changed.
        break;
    }

    // -------------------------------
    // 当远端设备启用或禁用通知事件
    case sl_bt_evt_gatt_server_characteristic_status_id:
    {
        break;
    }

    ///////////////////////////////////////////////////////////////////////////
    // Add additional event handlers here as your application requires!      //
    ///////////////////////////////////////////////////////////////////////////

    // -------------------------------
    // Default event handler.
    default:
      break;
  }
}

