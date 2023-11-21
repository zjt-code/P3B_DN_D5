/******************** (C) COPYRIGHT 2022 陈苏阳 ********************************
* File Name          :  app_glucose_meas.h
* Author             :  陈苏阳
* CPU Type         	 :  NRF52832
* IDE                :  Keil
* Version            :  V1.0
* Date               :  22/12/2022
* Description        :
*******************************************************************************/

/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __APP_GLUCOSE_MEAS_H
#define __APP_GLUCOSE_MEAS_H

/* Includes ------------------------------------------------------------------*/


/* Private define ------------------------------------------------------------*/
#define APP_GLUCOSE_MEAS_AVG_ELECTRIC_CURRENT_CAL_TEMP_ARRAY_SIZE       20                 // 用于计算平均电流的临时数据数组的最大成员个数("最多N个数据做平均"中的N)
#define APP_GLUCOSE_MEAS_MEAS_INTERVAL_MIN                              (3*60)               // 固件整体对外的最小测量间隔(单位:秒)(必须大于APP_GLUCOSE_MEAS_AFE_MEAS_INTERVAL_MIN)
#define APP_GLUCOSE_MEAS_SOFTIMER_INTERVAL                              (1000)             // 血糖测量软件定时器时间间隔
/* Private typedef -----------------------------------------------------------*/
typedef enum _afe_work_fsm_t
{
    AFE_WORK_FSM_INIT = 0,                          // 初始化状态
    AFE_WORK_FSM_CALIBRATE,                         // 校准状态
    AFE_WORK_FSM_START_MEAS,                        // 开始测量状态
}AFE_WORK_FSM_t;

/* Private variables ---------------------------------------------------------*/



/* Private function prototypes -----------------------------------------------*/
void app_glucose_meas_set_glucose_meas_interval(uint8_t ucGlucoseMeasInterval);
uint16_t app_glucose_meas_get_glucose_meas_interval(void);
uint16_t app_glucose_meas_get_glucose_quality(void);

void app_glucose_meas_handler(void);
void app_glucose_meas_start(void);
void app_glucose_meas_stop(void);
void app_glucose_meas_record_send_start(void);
void app_glucose_meas_record_send_stop(void);
void app_glucose_meas_init(void);
uint16_t app_glucose_get_records_current_offset(void);
#endif /* __APP_GLUCOSE_MEAS_H */

/******************* (C) COPYRIGHT 2022 陈苏阳 **** END OF FILE ****************/

