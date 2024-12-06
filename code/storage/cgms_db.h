/******************** (C) COPYRIGHT 2023 陈苏阳 ********************************
* File Name          :  cgms_db.h
* Author             :  陈苏阳
* CPU Type         	 :  RSL15
* IDE                :  Onsemi IDE
* Version            :  V1.0
* Date               :  16/1/2023
* Description        :  
*******************************************************************************/

/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __CGMS_DB_H
#define __CGMS_DB_H

/* Includes ------------------------------------------------------------------*/
#include "cgms_sst.h"
#include "cgms_meas.h"
#include "app_global.h"
/* Private define ------------------------------------------------------------*/

#define CGMS_DB_MAX_RECORDS                         (6720)                                                      // 设备中最大可存储的历史记录条数

#define MEAS_RECORD_INDEX_ADDR                      (0)                                                         // 历史数据参数存储地址(开始)
#define MEAS_RECORD_INDEX_FLASH_SIZE                (0x2000)                                                    // 用于存储历史数据参数的存储大小(byte)
#define MEAS_RECORD_INDEX_ONE_UNIT_SIZE             (32)                                                        // 单个历史数据参数存储单元所占Flash的大小

#define MEAS_RECORD_ADDR                            (MEAS_RECORD_INDEX_ADDR+MEAS_RECORD_INDEX_FLASH_SIZE)       // 历史数据存储地址(开始)
#define MEAS_RECORD_FLASH_SIZE                      (0x1C000)                                                   // 用于存储历史数据的存储大小(byte)*8192(对齐为112KB)

/* Private typedef -----------------------------------------------------------*/


typedef struct
{
    cgms_meas_t Record;// 历史数据内容
#if (USE_BLE_PROTOCOL==GN_2_PROTOCOL)
    
    #else
    uint8_t ucNone[7];// 填充
    uint16_t usChecksum;// 校验
    #endif
}__attribute__((packed))  one_record_storage_unit_t;// 单个历史数据存储结构体

typedef struct
{
    uint16_t usDataSum;// 效验
    uint32_t uiRecordsNum;// 当前历史数据总数
    ble_cgms_sst_t sst;// CGM会话启动时间
}__attribute__((packed))  record_index_storage_unit_t;// 历史数据参数存储结构体

typedef struct
{
    uint32_t uiPC;
    uint32_t uiPSR;
    uint32_t uiLR;
    uint32_t uiR12;
    uint32_t uiR3;
    uint32_t uiR2;
    uint32_t uiR1;
    uint32_t uiR0;
    uint32_t uiAFSR;
    uint32_t uiDFSR;
    uint32_t uiHFSR;
    uint32_t uiCFSR;
    uint32_t uiMMFAR;
    uint32_t uiMMFSR;
    uint32_t uiBFAR;
    uint32_t uiBFSR;
    uint32_t uiLR_EXC_RETURN;
    uint32_t uiRecordNum;
}__attribute__((packed))  hard_fault_info_t;

/* Private variables ---------------------------------------------------------*/

/* Private function prototypes -----------------------------------------------*/

/***********************历史数据管理***************************/
void cgms_db_reset(void);
ret_code_t cgms_db_init(void);
uint32_t cgms_db_get_records_num(void);
ret_code_t cgms_db_record_get(uint16_t usRecordIndex, cgms_meas_t* pRec);
ret_code_t cgms_db_record_add(cgms_meas_t* pRec);
void cmgs_db_force_write_flash(void);

/*********************历史数据参数管理*************************/
ret_code_t cgms_db_get_flash_record_index(record_index_storage_unit_t* pRecordIndex);
ret_code_t cgms_db_set_flash_record_index(record_index_storage_unit_t* pRecordIndex);
record_index_storage_unit_t* cgms_db_get_record_index(void);
void cgms_db_record_index_update_sst(ble_cgms_sst_t SST);
void cgms_db_update_record_index_records_num(uint32_t uiRecordsNum);

#endif /* __CGMS_DB_H */

/******************* (C) COPYRIGHT 2023 陈苏阳 **** END OF FILE ****************/
