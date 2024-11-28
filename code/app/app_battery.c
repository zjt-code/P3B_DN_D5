/******************** (C) COPYRIGHT 2023 陈苏阳 ********************************
* File Name          :  app_battery.c
* Author             :  陈苏阳
* CPU Type         	 :  NRF52832
* IDE                :  Keil
* Version            :  V1.0
* Date               :  15/11/2023
* Description        :  
*******************************************************************************/

/* Includes ------------------------------------------------------------------*/
#if !defined(LOG_TAG)
#define LOG_TAG                "APP_BATTERY"
#endif
#undef LOG_LVL
#define LOG_LVL                ELOG_LVL_DEBUG

#include <elog.h>
#include "app_battery.h"
#include "em_iadc.h"
#include "em_device.h"
#include "em_chip.h"
#include "em_cmu.h"
#include "em_emu.h"
#include "sl_sleeptimer.h"
#include "cgms_prm.h"
/* Private variables ---------------------------------------------------------*/

// Set CLK_ADC to 10MHz
#define CLK_SRC_ADC_FREQ          5000000 // CLK_SRC_ADC
#define CLK_ADC_FREQ               100 // CLK_ADC - 10MHz max in normal mode

static uint16_t g_usBattaryVol = 1600;              // 当前电池电压
uint8_t g_ucBatteryLevel = 99;                     // 电池电量

sl_sleeptimer_timer_handle_t g_BatteryMeasTimer;

IADC_Init_t init = IADC_INIT_DEFAULT;
IADC_AllConfigs_t initAllConfigs = IADC_ALLCONFIGS_DEFAULT;
IADC_InitSingle_t initSingle = IADC_INITSINGLE_DEFAULT;
IADC_SingleInput_t initSingleInput = IADC_SINGLEINPUT_DEFAULT;

app_battery_vol_level_point_t g_BatteryVolToLevelArray[12] = {
    {.ucLevel = 100,.usVol = 1630},
    {.ucLevel = 100,.usVol = 1530},
    {.ucLevel = 90,.usVol = 1500},
    {.ucLevel = 80,.usVol = 1470},
    {.ucLevel = 70,.usVol = 1450},
    {.ucLevel = 60,.usVol = 1430},
    {.ucLevel = 50,.usVol = 1400},
    {.ucLevel = 40,.usVol = 1370},
    {.ucLevel = 30,.usVol = 1350},
    {.ucLevel = 20,.usVol = 1350},
    {.ucLevel = 10,.usVol = 1300},
    {.ucLevel = 5,.usVol = 1270}
};

/* Private function prototypes -----------------------------------------------*/



/* Private functions ---------------------------------------------------------*/



/*******************************************************************************
*                           陈苏阳@2024-03-18
* Function Name  :  app_battery_close_adc
* Description    :  关闭ADC
* Input          :  void
* Output         :  None
* Return         :  void
*******************************************************************************/
void app_battery_close_adc(void)
{
  IADC_command(IADC0, iadcCmdStopSingle);

  IADC_reset(IADC0);

  // 失能IADC时钟
  CMU_ClockEnable(cmuClock_IADC0, false);
}

/*******************************************************************************
*                           陈苏阳@2024-03-13
* Function Name  :  app_battery_trigger_read_battery_vol
* Description    :  触发电池电压读取
* Input          :  void
* Output         :  None
* Return         :  None
*******************************************************************************/
void app_battery_trigger_read_battery_vol(void)
{
    // 使能IADC时钟
    CMU_ClockEnable(cmuClock_IADC0, true);

    // 重置IADC
    IADC_reset(IADC0);

    // 配置IADC时钟源
    CMU_ClockSelectSet(cmuClock_IADCCLK, cmuSelect_FSRCO);// 20MHz

    // 设置HFSCLK预刻度值
    init.srcClkPrescale = IADC_calcSrcClkPrescale(IADC0, CLK_SRC_ADC_FREQ, 0);

    // 默认情况下，扫描转换和单个转换都使用配置0
    //使用内部带隙(以mV为单位的电源电压)作为参考
    initAllConfigs.configs[0].reference = iadcCfgReferenceInt1V2;
    initAllConfigs.configs[0].vRef = 1210;
    initAllConfigs.configs[0].analogGain = iadcCfgAnalogGain1x;

    //除CLK_SRC_ADC设置CLK_ADC频率
    //默认过采样(OSR)为2x，转换时间= ((4 * OSR) + 2) / fCLK_ADC
    initAllConfigs.configs[0].adcClkPrescale = IADC_calcAdcClkPrescale(IADC0,
        CLK_ADC_FREQ,
        0,
        iadcCfgModeNormal,
        init.srcClkPrescale);

    //配置输入源
    initSingleInput.posInput = (_IADC_SCAN_PORTPOS_SUPPLY << (_IADC_SCAN_PORTPOS_SHIFT - _IADC_SCAN_PINPOS_SHIFT)) | 2;
    initSingleInput.negInput = iadcNegInputGnd;

    // 初始化IADC
    IADC_init(IADC0, &init, &initAllConfigs);
    IADC_initSingle(IADC0, &initSingle, &initSingleInput);

    IADC_command(IADC0, iadcCmdStartSingle);
}


/*******************************************************************************
*                           陈苏阳@2024-03-13
* Function Name  :  app_battery_read_battery_vol_is_complete
* Description    :  电压读取是否完成
* Input          :  void
* Output         :  None
* Return         :  bool
*******************************************************************************/
bool app_battery_read_battery_vol_is_complete(void)
{
    if ((IADC0->STATUS & (_IADC_STATUS_CONVERTING_MASK | _IADC_STATUS_SINGLEFIFODV_MASK)) != IADC_STATUS_SINGLEFIFODV)
    {
        return false;
    }
    return true;
}

/*******************************************************************************
*                           陈苏阳@2024-03-13
* Function Name  :  app_battery_read_battery_vol
* Description    :  读取电压
* Input          :  void
* Output         :  None
* Return         :  uint16_t
*******************************************************************************/
uint16_t app_battery_read_battery_vol(void)
{
    return g_usBattaryVol;
}


/*******************************************************************************
*                           陈苏阳@2022-12-23
* Function Name  :  app_battery_get_run_time
* Description    :  获取电池的运行时间(单位:S)
* Input          :  void
* Output         :  None
* Return         :  uint32_t
*******************************************************************************/
uint32_t app_battery_get_run_time(void)
{
    return g_BatteryInfo.uiBatteryRunTime==0?1:g_BatteryInfo.uiBatteryRunTime;
}


/*******************************************************************************
*                           陈苏阳@2024-03-14
* Function Name  :  app_battery_calculate_battery_level
* Description    :  计算电池电量
* Input          :  uint16_t usVol
* Input          :  uint32_t uiRunTime
* Output         :  None
* Return         :  uint8_t
*******************************************************************************/
uint8_t app_battery_calculate_battery_level(uint16_t usVol, uint32_t uiRunTime)
{
	// 先固定返回100
	return 100;
	
    // 限制最大电压
    uint16_t usBatteryVol = usVol > 1600 ? 1600 : usVol;
    uint8_t ucBatteryLevel = 0;

    for (uint8_t i = 0; i < sizeof(g_BatteryVolToLevelArray); i++)
    {
        if (g_BatteryVolToLevelArray[i].usVol <= usBatteryVol)
        {
            ucBatteryLevel = g_BatteryVolToLevelArray[i].ucLevel + (usBatteryVol - g_BatteryVolToLevelArray[i].usVol) / ((g_BatteryVolToLevelArray[i - 1].usVol / g_BatteryVolToLevelArray[i].usVol) / 10);
            break;
        }
    }
    ucBatteryLevel = ucBatteryLevel > 100 ? 100 : ucBatteryLevel;
    return ucBatteryLevel;
}

/*******************************************************************************
*                           陈苏阳@2024-03-14
* Function Name  :  sl_gatt_service_battery_get_level
* Description    :  获取电池电量的SDK回调
* Input          :  void
* Output         :  None
* Return         :  uint8_t
*******************************************************************************/
uint8_t sl_gatt_service_battery_get_level(void)
{
    log_d("g_ucBatteryLevel:%d", g_ucBatteryLevel);
    return g_ucBatteryLevel;
}


/*******************************************************************************
*                           陈苏阳@2024-03-18
* Function Name  :  battery_meas_timer_callback
* Description    :  电量转换定时器回调函数
* Input          :  sl_sleeptimer_timer_handle_t * handle
* Input          :  void * data
* Output         :  None
* Return         :  void
*******************************************************************************/
void battery_meas_timer_callback(sl_sleeptimer_timer_handle_t* handle, void* data)
{
    // 如果电量转换完成
    if (app_battery_read_battery_vol_is_complete())
    {
        // 读取电压
        int32_t sample = IADC_pullSingleFifoResult(IADC0).data;
        log_d("idac:%d", sample);
        g_usBattaryVol = (uint16_t)((sample * 1210 / 0xFFF) * 4);
        log_d("g_usBattaryVol:%d", g_usBattaryVol);

        // 关闭ADC
        app_battery_close_adc();

        // 计算电量
        g_ucBatteryLevel = app_battery_calculate_battery_level(g_usBattaryVol, g_BatteryInfo.uiBatteryRunTime);

        // 关闭定时器
        sl_sleeptimer_stop_timer(&g_BatteryMeasTimer);
    }
    else
    {
        log_e("app_battery_read_battery_vol fail");
        // 关闭ADC
        app_battery_close_adc();

        // 关闭定时器
        sl_sleeptimer_stop_timer(&g_BatteryMeasTimer);
    }
}


/*******************************************************************************
*                           陈苏阳@2022-12-23
* Function Name  :  app_battery_timer_handler
* Description    :  电量采集定时器回调函数
* Input          :  uint16_t usInterval(与上次调用时的间隔秒数)
* Output         :  None
* Return         :  None
*******************************************************************************/
void app_battery_timer_handler(uint16_t usInterval)
{
    log_i("app_battery_timer_handler(%d)", usInterval);
    g_BatteryInfo.uiBatteryRunTime += usInterval;

    bool bRunFsmFlag = false;
    if (g_usBattaryVol >=1600)
    {
        bRunFsmFlag = true;
    }
    else
    {
        if (g_BatteryInfo.uiBatteryRunTime % 1800 == 0)
        {
            bRunFsmFlag = true;
        }
        else
        {
            bRunFsmFlag = false;
        }
    }

    if (bRunFsmFlag == false)return;

    log_i("app_battery_trigger_read_battery_vol");
    // 触发电压读取
    app_battery_trigger_read_battery_vol();

    // 启动一个1mS后的单次定时器
    sl_status_t status = sl_sleeptimer_start_timer(&g_BatteryMeasTimer, sl_sleeptimer_ms_to_tick(10), battery_meas_timer_callback, (void*)NULL, 0, 0);
    if (status != SL_STATUS_OK)
    {
        log_e("sl_sleeptimer_start_timer fail:%d", status);
    }
}

/*******************************************************************************
*                           陈苏阳@2024-11-27
* Function Name  :  app_battery_save_battery_info_to_flash
* Description    :  保存电池信息到flash
* Input          :  void
* Output         :  None
* Return         :  void
*******************************************************************************/
void app_battery_save_battery_info_to_flash(void)
{
    cgms_prm_db_write_battery_info(&g_BatteryInfo);
}







/******************* (C) COPYRIGHT 2023 陈苏阳 **** END OF FILE ****************/




