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
#define APP_GLUCOSE_MEAS_AVG_ELECTRIC_CURRENT_CAL_TEMP_ARRAY_SIZE       30                // 用于计算平均电流的临时数据数组的最大成员个数("最多N个数据做平均"中的N)
#define APP_GLUCOSE_MEAS_MEAS_INTERVAL                                  (3)               // 固件整体对外的测量间隔(单位:分钟)
#define APP_GLUCOSE_MEAS_SOFTTIMER_INTERVAL                             (60*1000)         // 血糖测量软件定时器时间间隔
#define APP_GLUCOSE_MEAS_ONE_MEAS_SAMPLE_NUM                            (20)              // 单次测量的采样次数
/* Private typedef -----------------------------------------------------------*/
typedef enum
{
    AFE_WORK_FSM_INIT = 0,                          // 初始化状态
    AFE_WORK_FSM_CALIBRATE,                         // 校准状态
    AFE_WORK_FSM_START_MEAS,                        // 开始测量状态
}afe_work_fsm_t;

typedef enum
{
    APP_GLUCOSE_MEAS_TYPE_USER_MEAS = 0,           // 用户测量状态
    APP_GLUCOSE_MEAS_TYPE_FACTORY_MEAS,            // 工厂测量状态
}app_glucose_meas_type_t;


/* Private variables ---------------------------------------------------------*/



/* Private function prototypes -----------------------------------------------*/
uint16_t app_glucose_meas_get_glucose_quality(void);

void app_glucose_meas_stop_session_handler(uint8_t ucStopReason);
void app_glucose_meas_handler(uint32_t uiArg);
bool app_glucose_meas_get_factory_meas_electric_current(uint32_t* pMeasElectricCurrent);
void app_glucose_meas_start(app_glucose_meas_type_t GlucoseMeasType);
void app_glucose_meas_stop(void);
void app_glucose_meas_record_send_start(void);
void app_glucose_meas_record_send_stop(void);
void app_glucose_meas_init(void);
uint16_t app_glucose_get_records_current_offset(void);
#endif /* __APP_GLUCOSE_MEAS_H */

/******************* (C) COPYRIGHT 2022 陈苏阳 **** END OF FILE ****************/

