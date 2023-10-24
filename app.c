/******************** (C) COPYRIGHT 2023 ������ ********************************
* File Name          :  app.c
* Author             :  ������
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






/******************* (C) COPYRIGHT 2023 ������ **** END OF FILE ****************/






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
*                           ������@2023-10-24
* Function Name  :  sl_bt_on_event
* Description    :  BLEЭ��ջ�¼�������
* Input          :  sl_bt_msg_t * evt
* Output         :  None
* Return         :  void
*******************************************************************************/
void sl_bt_on_event(sl_bt_msg_t *evt)
{
  sl_status_t sc;

  switch (SL_BT_MSG_ID(evt->header)) 
  {
    // BLEЭ��ջ��ʼ������¼�
    case sl_bt_evt_system_boot_id:
    {
        // ����һ��BLE�㲥��
        sc = sl_bt_advertiser_create_set(&advertising_set_handle);
        app_assert_status(sc);

        // ����BLE�㲥����
        sc = sl_bt_legacy_advertiser_generate_data(advertising_set_handle,sl_bt_advertiser_general_discoverable);
        app_assert_status(sc);

        // ���ù㲥��ʱ�����
        sc = sl_bt_advertiser_set_timing(
            advertising_set_handle,
            160, // min. adv. interval (milliseconds * 1.6)
            160, // max. adv. interval (milliseconds * 1.6)
            0,   // adv. duration
            0);  // max. num. adv. events
        app_assert_status(sc);

        // ��ʼBLE�㲥
        sc = sl_bt_legacy_advertiser_start(advertising_set_handle,sl_bt_advertiser_connectable_scannable);
        app_assert_status(sc);

        break;
    }

    // -------------------------------
    // BLE�����¼�
    case sl_bt_evt_connection_opened_id:
    {
        app_log_info("Connection opened.\n");
        break;
    }

    // -------------------------------
    // BLE�Ͽ������¼�
    case sl_bt_evt_connection_closed_id:
    {
        app_log_info("Connection closed.\n");

        // ���ɹ㲥���ݰ�
        sc = sl_bt_legacy_advertiser_generate_data(advertising_set_handle,
            sl_bt_advertiser_general_discoverable);
        app_assert_status(sc);

        // ���¿�ʼ�㲥
        sc = sl_bt_legacy_advertiser_start(advertising_set_handle,sl_bt_advertiser_connectable_scannable);
        app_assert_status(sc);

        break;
    }

    // -------------------------------
    // Զ��GATT�ͻ��˸����˱���GATT���ݿ��е�����ֵ�¼�
    case sl_bt_evt_gatt_server_attribute_value_id:
    {
        // The value of the gattdb_led_control characteristic was changed.
        break;
    }

    // -------------------------------
    // ��Զ���豸���û����֪ͨ�¼�
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

