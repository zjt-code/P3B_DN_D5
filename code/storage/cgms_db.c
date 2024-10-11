/******************** (C) COPYRIGHT 2023 陈苏阳 ********************************
* File Name          :  cgms_db.c
* Author             :  陈苏阳
* CPU Type         	 :  RSL15
* IDE                :  Onsemi IDE
* Version            :  V1.0
* Date               :  17/1/2023
* Description        :
*******************************************************************************/

/* Includes ------------------------------------------------------------------*/
#if !defined(LOG_TAG)
#define LOG_TAG                "CGMS_DB"
#endif
#undef LOG_LVL
#define LOG_LVL                ELOG_LVL_DEBUG


#include <stdbool.h>
#include <stdint.h>
#include "stdlib.h"
#include <app.h>
#include "cgms_db.h"
#include "cgms_crc.h"
#include "string.h"
#include "cgms_prm.h"
#include "cgms_db_port.h"
#include "elog.h"
/* Private variables ---------------------------------------------------------*/

uint8_t* g_pSavedMeas = NULL;                       // 用于存储即将写入Flash的数据
static uint32_t g_uiRecordsNum = 0;                 // 当前存储的历史数据条数(值从1开始)
__attribute__((aligned(4))) record_index_storage_unit_t	g_mRecordIndex;         // 历史数据参数信息
/* Private function prototypes -----------------------------------------------*/
uint16_t cgms_db_get_min_align_size(uint16_t usTrgSize);


/* Private functions ---------------------------------------------------------*/

/*******************************************************************************
*                           陈苏阳@2023-04-28
* Function Name  :  cgms_db_get_record_index
* Description    :  获取record_index
* Input          :  void
* Output         :  None
* Return         :  record_index_storage_unit_t*
*******************************************************************************/
record_index_storage_unit_t* cgms_db_get_record_index(void)
{
    return &g_mRecordIndex;
}

/*******************************************************************************
*                           陈苏阳@2023-04-28
* Function Name  :  cgms_db_record_index_update_sst
* Description    :  更新record_index中的会话开始时间
* Input          :  ble_cgms_sst_t SST
* Output         :  None
* Return         :  void
*******************************************************************************/
void cgms_db_record_index_update_sst(ble_cgms_sst_t SST)
{
    g_mRecordIndex.sst = SST;
}


/*******************************************************************************
*                           陈苏阳@2023-04-28
* Function Name  :  cgms_db_update_record_index_records_num
* Description    :  更新record_index中的记录总数
* Input          :  uint32_t uiRecordsNum
* Output         :  None
* Return         :  void
*******************************************************************************/
void cgms_db_update_record_index_records_num(uint32_t uiRecordsNum)
{
    g_mRecordIndex.uiRecordsNum = uiRecordsNum;
}

/*******************************************************************************
*                           陈苏阳@2023-01-19
* Function Name  :  cgms_db_calculation_record_pos
* Description    :  计算历史数据记录的位置
* Input          :  uint32_t uiRecordIndex(值从0开始)       // 当前历史数据条目的索引值
* Input          :  uint16_t * pPageIndex(值从0开始)        // 该历史数据条目所在Page的索引值
* Input          :  uint16_t * pPosInPage(值从0开始)        // 该历史数据条目所在的Page中的字节位置的索引值
* Output         :  None
* Return         :
*******************************************************************************/
static void cgms_db_calculation_record_pos(uint32_t uiRecordIndex, uint16_t* pPageIndex, uint16_t* pPosInPage)
{
    // 计算一条历史数据结构体在flash中占用的大小
    uint16_t usOneRecordStorageUnitSize = cgms_db_get_min_align_size(sizeof(one_record_storage_unit_t));

    // 计算在一个page中可以存下的历史数据条数
    uint16_t usOnePageRecNum = cgm_db_flash_get_info()->usSectorByteSize / usOneRecordStorageUnitSize;

    // 先计算目标历史数据是在哪个page中(不考虑page循环)
    uint16_t usTmpPageIndex = uiRecordIndex / usOnePageRecNum;

    // 考虑page循环
    usTmpPageIndex = (usTmpPageIndex % (MEAS_RECORD_FLASH_SIZE / cgm_db_flash_get_info()->usSectorByteSize));

    // 计算目标历史数据是在page中的哪个位置
    uint16_t usTmpPosInPage = uiRecordIndex % usOnePageRecNum;

    if (pPageIndex)*pPageIndex = usTmpPageIndex;
    if (pPosInPage)*pPosInPage = usTmpPosInPage;
}



/*******************************************************************************
*                           陈苏阳@2023-01-16
* Function Name  :  cgms_db_reset
* Description    :  复位历史数据存储
* Output         :  None
* Return         :  void
*******************************************************************************/
void cgms_db_reset(void)
{
    log_i("cgms_db_reset");

    g_uiRecordsNum = 0;
    // 擦除全部数据
    cgms_db_flash_erase(MEAS_RECORD_INDEX_ADDR, MEAS_RECORD_INDEX_FLASH_SIZE);
    cgms_db_flash_erase(MEAS_RECORD_ADDR, MEAS_RECORD_FLASH_SIZE);
}


/*******************************************************************************
*                           陈苏阳@2023-10-23
* Function Name  :  cgms_db_get_min_align_size
* Description    :  获取最小对齐大小
* Input          :  uint16_t usTrgSize
* Output         :  None
* Return         :  uint16_t
*******************************************************************************/
uint16_t cgms_db_get_min_align_size(uint16_t usTrgSize)
{
    uint16_t usTmpSize = 0;
    uint8_t ucAlignAtNByte = cgm_db_flash_get_info()->ucAlignAtNByte;
    // 计算出一个临时大小
    do
    {
        // 如果目标长度小于或者等于最小对齐字节数,则直接按最小对齐字节数返回
        if (ucAlignAtNByte >= usTrgSize)
        {
            usTmpSize = ucAlignAtNByte;
            break;
        }
        // 如果目标长度正好能被最小对齐字节数整除,则直接返回目标长度
        else if ((usTrgSize % ucAlignAtNByte) == 0)
        {
            usTmpSize = usTrgSize;
            break;
        }
        else
        {
            // 否则返回目标长度+余数部分
            usTmpSize = usTrgSize + (ucAlignAtNByte - (usTrgSize % ucAlignAtNByte));
            break;
        }
    } while (1);

    /*
    // 如果这个临时大小不能整除flash的一个page大小(说明可能出现跨page存储的情况)
    if (CgmsDbPortInfo.usSectorByteSize % usTmpSize)
    {
        // todo:考虑做一些处理,但是如果为了整除page扩展size,可能会和上面的对齐条件冲突

    }
    else
    {
        // 直接返回
        return usTmpSize;
    }
    */
    // 直接返回
    return usTmpSize;
}


/*******************************************************************************
*                           陈苏阳@2023-01-17
* Function Name  :  cgms_db_init
* Description    :  初始化历史数据存储
* Input          :  void
* Output         :  None
* Return         :  ret_code_t
*******************************************************************************/
ret_code_t cgms_db_init(void)
{
    g_uiRecordsNum = 0;
    g_pSavedMeas = NULL;

    // 初始化历史数据的flash接口
    cgms_db_flash_init();

    // 申请一段扇区字节数+对齐字节数的RAM空间
    uint8_t* pTmpMalloc = malloc(cgm_db_flash_get_info()->usSectorByteSize + cgm_db_flash_get_info()->ucAlignAtNByte);

    // 如果申请成功
    if (pTmpMalloc)
    {
        log_d("AlignAtNByte:%d", cgm_db_flash_get_info()->ucAlignAtNByte);

        g_pSavedMeas = 0x00000000;
        for (uint8_t i = 0; i < cgm_db_flash_get_info()->ucAlignAtNByte; i++)
        {
            // 判断是否对齐,如果对齐则按这个地址输出buffer指针
            if ((((uint32_t)pTmpMalloc + i) % cgm_db_flash_get_info()->ucAlignAtNByte) == 0)
            {
                g_pSavedMeas = (uint8_t*)((uint32_t)pTmpMalloc + i);

                log_d("g_pSavedMeas:0x%X", (uint32_t)g_pSavedMeas);
                break;
            }
        }
        if (g_pSavedMeas == 0x00000000)log_e("g_pSavedMeas is Null");

    }
    else
    {
        log_e("g_pSavedMeas malloc fail");
        return RET_CODE_FAIL;
    }
    if (g_pSavedMeas == NULL)return RET_CODE_FAIL;

    // 清空buffer
    memset(g_pSavedMeas, 0x00, cgm_db_flash_get_info()->usSectorByteSize);

    log_d("cgms_db_init  start:0x%x,size:%d", MEAS_RECORD_ADDR, MEAS_RECORD_FLASH_SIZE);

    // 获取Flash中存储的RecordIndex
    if (cgms_db_get_flash_record_index(&g_mRecordIndex) == 0)
    {
        // 恢复历史数据数量
        g_uiRecordsNum = g_mRecordIndex.uiRecordsNum;

        // 恢复CGM会话启动时间
        cgms_sst_recover(g_mRecordIndex.sst);


        log_d("cgms_db_get_flash_record_index g_uiRecordsNum:%d", g_uiRecordsNum);
#if (USE_BLE_PROTOCOL==GN_2_PROTOCOL)
        log_d("cgms_db_get_flash_record_index sst_time_zone:%d", g_mRecordIndex.sst.time_zone);
        #else
        log_d("cgms_db_get_flash_record_index sst_time_zone:%d", g_mRecordIndex.sst.date_time.time_zone);
        #endif
        log_d("cgms_db_get_flash_record_index sst:%d/%d/%d   %d:%d:%d", g_mRecordIndex.sst.date_time.time_info.year, \
            g_mRecordIndex.sst.date_time.time_info.month, \
            g_mRecordIndex.sst.date_time.time_info.day, \
            g_mRecordIndex.sst.date_time.time_info.hour, \
            g_mRecordIndex.sst.date_time.time_info.minute, \
            g_mRecordIndex.sst.date_time.time_info.sec);
    }
    else
    {
        log_e("cgms_db_get_flash_record_index fail");
    }
    return RET_CODE_SUCCESS;
}


/*******************************************************************************
*                           陈苏阳@2023-01-17
* Function Name  :  cgms_db_get_records_num
* Description    :  获取存储的记录数量
* Input          :  void
* Output         :  None
* Return         :  uint32_t
*******************************************************************************/
uint32_t cgms_db_get_records_num(void)
{
    return g_uiRecordsNum;
}

/*******************************************************************************
*                           陈苏阳@2023-04-28
* Function Name  :  cgms_db_get_flash_record_index
* Description    :  从Flash中获取有效的record_index数据
* Input          :  record_index_storage_unit_t * pRecordIndex
* Output         :  None
* Return         :  ret_code_t
*******************************************************************************/
ret_code_t cgms_db_get_flash_record_index(record_index_storage_unit_t* pRecordIndex)
{
    uint16_t usRecordIndexStorageUnitSize = cgms_db_get_min_align_size(sizeof(record_index_storage_unit_t));

    // 遍历Flash中所有的record_index_storage_unit_t结构体
    for (uint32_t i = 0; i < (MEAS_RECORD_INDEX_FLASH_SIZE / usRecordIndexStorageUnitSize); i++)
    {
        __attribute__((aligned(4))) record_index_storage_unit_t TmpRecordIndex;

        // 读取RecordIndex
        cgms_db_flash_read(MEAS_RECORD_INDEX_ADDR + (i * usRecordIndexStorageUnitSize), (uint8_t*)&TmpRecordIndex, sizeof(record_index_storage_unit_t));

        // 如果校验和不等于0x00或者0xFFFF,则说明可能是有效的,开始计算正确性.
        if (TmpRecordIndex.usDataSum != 0x00 && TmpRecordIndex.usDataSum != 0xFFFF)
        {
            // 计算校验和
            TmpRecordIndex.usDataSum = 0;
            uint8_t* pRecordIndexCheck = (uint8_t*)&TmpRecordIndex;
            uint16_t usDataSum = 0;
            for (uint8_t k = 0; k < sizeof(record_index_storage_unit_t); k++)usDataSum += pRecordIndexCheck[k];

            // 如果校验和正确,且参数指针合法,则对外输出RecordIndex
            if (TmpRecordIndex.usDataSum == usDataSum && pRecordIndex)
            {
                log_d("cgms_db_get_flash_record_index index:%d", i);
                *pRecordIndex = TmpRecordIndex;
                return RET_CODE_SUCCESS;
            }
            else if (TmpRecordIndex.usDataSum != usDataSum)
            {
                // 不正确,则继续查找
                continue;
            }

        }
    }
    return RET_CODE_FAIL;
}


/*******************************************************************************
*                           陈苏阳@2023-04-28
* Function Name  :  cgms_db_set_flash_record_index
* Description    :  向Flash中设置record_index数据
* Input          :  record_index_storage_unit_t * pRecordIndex
* Output         :  None
* Return         :  ret_code_t
*******************************************************************************/
ret_code_t cgms_db_set_flash_record_index(record_index_storage_unit_t* pRecordIndex)
{
    ret_code_t Ret;
    if (pRecordIndex == NULL)return RET_CODE_FAIL;

    uint16_t usRecordIndexStorageUnitSize = cgms_db_get_min_align_size(sizeof(record_index_storage_unit_t) > MEAS_RECORD_INDEX_ONE_UNIT_SIZE ? sizeof(record_index_storage_unit_t) : MEAS_RECORD_INDEX_ONE_UNIT_SIZE);
    // 遍历Flash中所有的record_index_storage_unit_t结构体
    for (uint32_t i = 0; i < (MEAS_RECORD_INDEX_FLASH_SIZE / usRecordIndexStorageUnitSize); i++)
    {
        __attribute__((aligned(4))) record_index_storage_unit_t TmpRecordIndex;

        // 读取RecordIndex
        cgms_db_flash_read(MEAS_RECORD_INDEX_ADDR + (i * usRecordIndexStorageUnitSize), (uint8_t*)&TmpRecordIndex, sizeof(record_index_storage_unit_t));

        // 如果校验和等于0xFFFF,则说明可能是一个空位,可用来写入.
        if (TmpRecordIndex.usDataSum == 0xFFFF)
        {
            uint8_t ucIsEmpty = 1;
            for (uint8_t k = 0; k < sizeof(record_index_storage_unit_t); k++)
            {
                if (((uint8_t*)&TmpRecordIndex)[k] != 0xFF)
                {
                    ucIsEmpty = 0;
                }
            }
            // 如果结构体所在的Flash内容不是全0XFF,则跳过.
            if (ucIsEmpty == 0)continue;
            TmpRecordIndex = *pRecordIndex;

            // 计算校验和
            TmpRecordIndex.usDataSum = 0;
            uint8_t* pRecordIndexCheck = (uint8_t*)&TmpRecordIndex;
            uint16_t usDataSum = 0;
            for (uint8_t k = 0; k < sizeof(record_index_storage_unit_t); k++)usDataSum += pRecordIndexCheck[k];

            // 赋值校验和
            TmpRecordIndex.usDataSum = usDataSum;

            // 如果下一个RecordIndex的地址正好是一个扇区的开始,而且现在这个扇区不是最后的扇区.说明目前写的这个RecordIndex是这个扇区的最后一个RecordIndex了
            if ((MEAS_RECORD_INDEX_ADDR + ((i + 1) * usRecordIndexStorageUnitSize)) % cgm_db_flash_get_info()->usSectorByteSize == 0)
            {
                // 如果后面还有空余扇区
                if ((MEAS_RECORD_INDEX_ADDR + ((i + 1) * usRecordIndexStorageUnitSize)) < (MEAS_RECORD_INDEX_ADDR + MEAS_RECORD_INDEX_FLASH_SIZE))
                {
                    // 擦除下一个扇区,为后续写入RecordIndex做准备
                    Ret = cgms_db_flash_erase(MEAS_RECORD_INDEX_ADDR + ((i + 1) * usRecordIndexStorageUnitSize), cgm_db_flash_get_info()->usSectorByteSize);
                    if (Ret == RET_CODE_SUCCESS)
                    {
                        log_d("Flash_EraseSector %x ok", MEAS_RECORD_INDEX_ADDR + ((i + 1) * usRecordIndexStorageUnitSize));
                    }
                    else
                    {
                        log_e("Flash_EraseSector %x error", MEAS_RECORD_INDEX_ADDR + ((i + 1) * usRecordIndexStorageUnitSize));
                    }

                }
                // 如果没有下一个扇区了
                else
                {
                    // 擦除第一个扇区,为后续写入RecordIndex做准备
                    Ret = cgms_db_flash_erase(MEAS_RECORD_INDEX_ADDR, cgm_db_flash_get_info()->usSectorByteSize);
                    if (Ret == RET_CODE_SUCCESS)
                    {
                        log_d("Flash_EraseSector 0x%x ok", MEAS_RECORD_INDEX_ADDR);
                    }
                    else
                    {
                        log_e("Flash_EraseSector 0x%x error:%d", MEAS_RECORD_INDEX_ADDR, Ret);
                    }
                }
            }
            Ret = cgms_db_flash_write(MEAS_RECORD_INDEX_ADDR + (i * usRecordIndexStorageUnitSize), (uint8_t*)&TmpRecordIndex, sizeof(record_index_storage_unit_t) > MEAS_RECORD_INDEX_ONE_UNIT_SIZE ? sizeof(record_index_storage_unit_t) : MEAS_RECORD_INDEX_ONE_UNIT_SIZE);
            if (Ret == RET_CODE_SUCCESS)
            {
                log_d("Flash_WriteBuffer 0x%x ok", MEAS_RECORD_INDEX_ADDR + (i * usRecordIndexStorageUnitSize));

                if (i != 0)
                {
                    // 将上一个RecordIndex的内容清空,以标记为无效(生成连续4个成员的空白记录,防止sizoef(record_index_storage_unit_t)<MEAS_RECORD_INDEX_ONE_UNIT_SIZE的情况.
                    __attribute__((aligned(4))) record_index_storage_unit_t EmptyRecordIndex[4];
                    memset(EmptyRecordIndex, 0x00, sizeof(record_index_storage_unit_t) * 4);
                    uint32_t uiWriteAddr = MEAS_RECORD_INDEX_ADDR + ((i - 1) * usRecordIndexStorageUnitSize);
                    // 将历史数据标记为无效
                    Ret = cgms_db_flash_write(uiWriteAddr, (uint8_t*)&EmptyRecordIndex, sizeof(record_index_storage_unit_t) > MEAS_RECORD_INDEX_ONE_UNIT_SIZE ? sizeof(record_index_storage_unit_t) : MEAS_RECORD_INDEX_ONE_UNIT_SIZE);
                    if (Ret == RET_CODE_SUCCESS)
                    {
                        log_d("Invalid Mark-Flash_WriteBuffer 0x%x ok", MEAS_RECORD_INDEX_ADDR + ((i - 1) * usRecordIndexStorageUnitSize));
                    }
                    else
                    {
                        log_e("Flash_WriteBuffer 0x%x error:%d", MEAS_RECORD_INDEX_ADDR + ((i - 1) * usRecordIndexStorageUnitSize), Ret);
                    }
                }
                return RET_CODE_SUCCESS;
            }
            else
            {
                log_e("Flash_WriteBuffer 0x%x error:%d", MEAS_RECORD_INDEX_ADDR + (i * usRecordIndexStorageUnitSize), Ret);
                return RET_CODE_FAIL;
            }

        }
    }
    return RET_CODE_FAIL;
}



/*******************************************************************************
*                           陈苏阳@2023-01-19
* Function Name  :  cgms_db_record_read_and_check
* Description    :  历史数据读取&检查
* Input          :  one_record_storage_unit_t * pRecSrc
* Input          :  nrf_ble_cgms_meas_t * p_rec
* Output         :  None
* Return         :  ret_code_t
*******************************************************************************/
static ret_code_t cgms_db_record_read_and_check(one_record_storage_unit_t* pRecSrc, cgms_meas_t* p_rec)
{
    // 如果CRC效验正确
    if (do_crc((uint8_t*)pRecSrc, sizeof(one_record_storage_unit_t)) == 0)
    {
        // 读取历史数据
        if (p_rec)*p_rec = pRecSrc->Record;
        return RET_CODE_SUCCESS;
    }
    else
    {
        log_e("cgms_db_record_read_and_check crc error");

        return RET_CODE_FAIL;
    }
}



/*******************************************************************************
*                           陈苏阳@2023-01-19
* Function Name  :  cgms_db_record_get_raw_data
* Description    :  根据下标获取原始的历史数据
* Input          :  uint16_t usRecordIndex
* Input          :  nrf_ble_cgms_meas_t * pRec
* Output         :  None
* Return         :  ret_code_t
*******************************************************************************/
static ret_code_t cgms_db_record_get_raw_data(uint16_t usRecordIndex, cgms_meas_t* pRec)
{
    // 计算当前要读取的历史数据的位置
    uint16_t usPageIndex, usPosInPage, usNewRecordPageIndex, usNewRecordPosInPage;
    __attribute__((aligned(4))) one_record_storage_unit_t TmpRecord;

    // 根据历史数据索引值计算历史数据在Flash中的位置
    cgms_db_calculation_record_pos(usRecordIndex, &usPageIndex, &usPosInPage);

    // 计算一条历史数据结构体在flash中占用的大小
    uint16_t usOneRecordStorageUnitSize = cgms_db_get_min_align_size(sizeof(one_record_storage_unit_t));


    // 计算在一个page中可以存下的历史数据条数
    uint16_t usOnePageRecNum = cgm_db_flash_get_info()->usSectorByteSize / usOneRecordStorageUnitSize;

    log_i("usOneRecordStorageUnitSize:%d  usOnePageRecNum:%d", usOneRecordStorageUnitSize, usOnePageRecNum);

    // 如果当前所有的历史数据都在Flash中
    if (g_uiRecordsNum % usOnePageRecNum == 0)
    {
        // 拷贝目标历史数据
        uint32_t uiReadAddr = MEAS_RECORD_ADDR + (usPageIndex * cgm_db_flash_get_info()->usSectorByteSize) + (usPosInPage * usOneRecordStorageUnitSize);

        cgms_db_flash_read(uiReadAddr, (uint8_t*)&TmpRecord, sizeof(one_record_storage_unit_t));
        log_i("cgms_db_record_get_raw_data1  0x%x", uiReadAddr);
        elog_hexdump("data", 8, (uint8_t*)(&TmpRecord), sizeof(one_record_storage_unit_t));
    }
    // 如果当前有历史数据在RAM中
    else
    {
        // 计算当前最新的历史数据所在位置
        cgms_db_calculation_record_pos(g_uiRecordsNum - 1, &usNewRecordPageIndex, &usNewRecordPosInPage);

        // 如果最新的历史数据与要获取的历史数据在同一个page,则说明历史数据还在RAM中
        if (usNewRecordPageIndex == usPageIndex)
        {
            // 计算目标历史数据指针,并拷贝数据
            uint32_t uiReadAddr = (uint32_t)(&g_pSavedMeas[0]) + (usPosInPage * usOneRecordStorageUnitSize);

            memcpy(&TmpRecord, (void*)uiReadAddr, sizeof(one_record_storage_unit_t));

            log_i("cgms_db_record_get_raw_data2  0x%x", uiReadAddr);
            elog_hexdump("data", 8, (uint8_t*)&TmpRecord, sizeof(one_record_storage_unit_t));
        }
        else
        {
            // 如果不是同一个page,则在Flash中
            log_i("usPageIndex:%d", usPageIndex);
            // 计算目标历史数据地址
            uint32_t uiReadAddr = MEAS_RECORD_ADDR + (usPageIndex * cgm_db_flash_get_info()->usSectorByteSize) + (usPosInPage * usOneRecordStorageUnitSize);
#if (USE_BLE_PROTOCOL==GN_2_PROTOCOL)
			cgms_db_flash_read(uiReadAddr, &TmpRecord, sizeof(one_record_storage_unit_t));
            log_i("cgms_db_record_get_raw_data3  0x%x", uiReadAddr);
            elog_hexdump("data", 8, (uint8_t*)uiReadAddr, sizeof(one_record_storage_unit_t));
            #else
            cgms_db_flash_read(uiReadAddr, (uint8_t*)&TmpRecord, sizeof(one_record_storage_unit_t));
            log_i("cgms_db_record_get_raw_data3  0x%x", uiReadAddr);
            elog_hexdump("data", 8, (uint8_t*)&TmpRecord, sizeof(one_record_storage_unit_t));
            #endif
        }
    }

    // 读取&检查历史数据
    return cgms_db_record_read_and_check(&TmpRecord, pRec);
}

/*******************************************************************************
*                           陈苏阳@2023-01-16
* Function Name  :  cgms_db_record_get
* Description    :  获取指定记录
* Input          :  uint16_t usRecordIndex
* Input          :  nrf_ble_cgms_meas_t * pRec
* Output         :  None
* Return         :  ret_code_t
*******************************************************************************/
ret_code_t cgms_db_record_get(uint16_t usRecordIndex, cgms_meas_t* pRec)
{
    log_i("cgms_db_record_get %d", usRecordIndex);
    if (cgms_db_record_get_raw_data(usRecordIndex, pRec) == RET_CODE_SUCCESS)
    {
#if (USE_BLE_PROTOCOL==GN_2_PROTOCOL)
        // 设置最高位为1,用于标识本条数据为历史数据而不是实时数据.
        pRec->usHistoryFlag = CGMS_MEAS_HISTORY_FLAG_HISTORY;
#else
        // 设置最高位为1,用于标识本条数据为历史数据而不是实时数据.
        pRec->usHistoryFlag = CGMS_MEAS_HISTORY_FLAG_HISTORY;
#endif
        return RET_CODE_SUCCESS;
    }
    return RET_CODE_FAIL;
}

/*******************************************************************************
*                           陈苏阳@2023-01-16
* Function Name  :  cgms_db_record_add
* Description    :  添加记录
* Input          :  nrf_ble_cgms_meas_t * pRec
* Output         :  None
* Return         :  ret_code_t
*******************************************************************************/
ret_code_t cgms_db_record_add(cgms_meas_t* pRec)
{
    if (cgms_db_get_records_num() == CGMS_DB_MAX_RECORDS)
    {
        return RET_CODE_FAIL;
    }

    // 计算一条历史数据结构体在flash中占用的大小
    uint16_t usOneRecordStorageUnitSize = cgms_db_get_min_align_size(sizeof(one_record_storage_unit_t));

    // 计算在一个page中可以存下的历史数据条数
    uint16_t usOnePageRecNum = cgm_db_flash_get_info()->usSectorByteSize / usOneRecordStorageUnitSize;


    // 拷贝测量数据
    one_record_storage_unit_t TmpBuffer;
    memcpy(&TmpBuffer.Record, pRec, sizeof(TmpBuffer.Record));

    // 计算CRC
    uint8_t* p = (uint8_t*)&TmpBuffer;
    uint16_t usTmpCrc = do_crc(p, sizeof(TmpBuffer) - 2);
    TmpBuffer.usChecksum = usTmpCrc;

    // 计算当前要写入的历史数据的位置
    uint16_t usPageIndex, usPosInPage;
    cgms_db_calculation_record_pos(g_uiRecordsNum, &usPageIndex, &usPosInPage);

    log_i("cgms_db_record_add %d , %d , %d", g_uiRecordsNum, usPageIndex, usPosInPage);
    elog_hexdump("data", 8, (uint8_t*)&TmpBuffer, sizeof(TmpBuffer));


    // 如果当前要写入的是这个page中第一个数据,清空整个buffer
    if (usPosInPage == 0)memset((uint8_t*)(&g_pSavedMeas[0]), 0xFF, cgm_db_flash_get_info()->usSectorByteSize);

    uint32_t uiWriteAddr = (uint32_t)(&g_pSavedMeas[0]) + usPosInPage * usOneRecordStorageUnitSize;

    // 拷贝历史数据
    memcpy((uint8_t*)uiWriteAddr, &TmpBuffer, sizeof(one_record_storage_unit_t));

    log_i("memcpy %x", uiWriteAddr);

    // 如果当前要写入的是这个page中最后一个数据
    if (usPosInPage == usOnePageRecNum - 1)
    {
        uiWriteAddr = MEAS_RECORD_ADDR + (usPageIndex * cgm_db_flash_get_info()->usSectorByteSize);

        // 擦除目标page
        if (cgms_db_flash_erase(uiWriteAddr, cgm_db_flash_get_info()->usSectorByteSize) == RET_CODE_SUCCESS)
        {
            log_d("Flash_EraseSector %x ok", uiWriteAddr);
        }
        else
        {
            log_e("Flash_EraseSector %x error", uiWriteAddr);
        }
        // 写入历史数据
        if (cgms_db_flash_write(uiWriteAddr, g_pSavedMeas, cgm_db_flash_get_info()->usSectorByteSize) == RET_CODE_SUCCESS)
        {
            log_d("Flash_WriteBuffer %x ok", uiWriteAddr);
        }
        else
        {
            log_e("Flash_WriteBuffer %x error", uiWriteAddr);
        }

        // 写入RecordIndex
        if (cgms_db_set_flash_record_index(&g_mRecordIndex) != RET_CODE_SUCCESS)
        {
            log_e("cgms_db_set_flash_record_index fail");
        }
        else
        {
            log_d("cgms_db_set_flash_record_index ok");
        }
    }
    // 如果这是最后一条数据,则也写一次record_index
    else if (g_uiRecordsNum == CGMS_DB_MAX_RECORDS - 1)
    {
        // 写入RecordIndex
        if (cgms_db_set_flash_record_index(&g_mRecordIndex) != RET_CODE_SUCCESS)
        {
            log_e("cgms_db_set_flash_record_index fail");
        }
        else
        {
            log_d("cgms_db_set_flash_record_index ok");
        }
    }


    // 历史数据条数++
    g_uiRecordsNum++;

    // 更新g_mRecordIndex中的RecordsNum
    cgms_db_update_record_index_records_num(g_uiRecordsNum);
    return RET_CODE_SUCCESS;
}


/*******************************************************************************
*                           陈苏阳@2023-01-19
* Function Name  :  cmgs_db_force_write_flash
* Description    :  当前RAM中的数据强制写入Flash
* Input          :  void
* Output         :  None
* Return         :  void
*******************************************************************************/
void cmgs_db_force_write_flash(void)
{
    log_d("cmgs_db_force_write_flash");
    // 计算当前历史数据的位置
    uint16_t usPageIndex, usPosInPage;
    cgms_db_calculation_record_pos(g_uiRecordsNum - 1, &usPageIndex, &usPosInPage);

    // 计算一条历史数据结构体在flash中占用的大小
    uint16_t usOneRecordStorageUnitSize = cgms_db_get_min_align_size(sizeof(one_record_storage_unit_t));

    // 计算在一个page中可以存下的历史数据条数
    uint16_t usOnePageRecNum = cgm_db_flash_get_info()->usSectorByteSize / usOneRecordStorageUnitSize;

    // 如果当前历史数据的位置不在一个page中的最后一个(说明历史数据还在ram中)
    if (usPosInPage != usOnePageRecNum - 1)
    {
        // 擦除目标page
        cgms_db_flash_erase(MEAS_RECORD_ADDR + (usPageIndex * cgm_db_flash_get_info()->usSectorByteSize), cgm_db_flash_get_info()->usSectorByteSize);

        // 写入历史数据
        uint32_t uiWriteAddr = MEAS_RECORD_ADDR + (usPageIndex * cgm_db_flash_get_info()->usSectorByteSize);
        if (cgms_db_flash_write(uiWriteAddr, g_pSavedMeas, cgm_db_flash_get_info()->usSectorByteSize) == RET_CODE_SUCCESS)
        {
            log_d("Flash_WriteBuffer %x ok", uiWriteAddr);
        }
        else
        {
            log_e("Flash_WriteBuffer %x error", uiWriteAddr);
        }
    }
    else
    {
        // 如果历史数据已经在ram中,则不操作
    }
}

/******************* (C) COPYRIGHT 2023 陈苏阳 **** END OF FILE ****************/
