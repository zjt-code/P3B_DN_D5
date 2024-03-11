/******************** (C) COPYRIGHT 2023 陈苏阳 ********************************
* File Name          :  cgms_socp.c
* Author             :  陈苏阳
* CPU Type         	 :  RSL15
* IDE                :  Onsemi IDE
* Version            :  V1.0
* Date               :  27/3/2023
* Description        :
*******************************************************************************/

/* Includes ------------------------------------------------------------------*/

#if !defined(LOG_TAG)
#define LOG_TAG                   "CGMS_SOCP"
#endif
#undef LOG_LVL
#define LOG_LVL                    ELOG_LVL_DEBUG


#include <stdint.h>
#include "stdio.h"
#include <string.h>
#include <app.h>
#include "cgms_sst.h"
#include "cgms_socp.h"
#include "cgms_crc.h"
#include "app_util.h"
#include "app_global.h"
#include "cgms_aes128.h"
#include "ble_customss.h"
#include "sl_bt_api.h"
#include "app_glucose_meas.h"
#include "cgms_prm.h"
#include <elog.h>
/* Private variables ---------------------------------------------------------*/
#define NRF_BLE_CGMS_PLUS_INFINTE                     0x07FE
#define NRF_BLE_CGMS_MINUS_INFINTE                    0x0802
static bool g_bBleSocpNotifyIsEnableFlag = false;						// BLE SOCP通知使能标志位
//extern float sfCurrBg;//add by woo
uint8_t	caliTag;
uint8_t calData[15];
uint8_t calDataFlag = 0;
uint32_t simDelta = 0;
uint32_t simMax = 0;
uint32_t simMin = 0;
/* Private function prototypes -----------------------------------------------*/
static ret_code_t socp_send(ble_event_info_t BleEventInfo, ble_socp_rsp_t SocpRspDatapacket);
bool cgms_socp_check_production_cmd(uint8_t* pData, uint16_t usLen);
uint8_t ble_socp_encode(const ble_socp_rsp_t* pSocpRsp, uint8_t* pData);
/* Private functions ---------------------------------------------------------*/


/*******************************************************************************
*                           陈苏阳@2023-11-14
* Function Name  :  ble_socp_notify_enable
* Description    :  通知被使能
* Input          :  void
* Output         :  None
* Return         :  void
*******************************************************************************/
void ble_socp_notify_enable(void)
{
    g_bBleSocpNotifyIsEnableFlag = true;
}

/*******************************************************************************
*                           陈苏阳@2023-11-14
* Function Name  :  ble_socp_notify_disable
* Description    :  通知被失能
* Input          :  void
* Output         :  None
* Return         :  void
*******************************************************************************/
void ble_socp_notify_disable(void)
{
    g_bBleSocpNotifyIsEnableFlag = false;
}

/*******************************************************************************
*                           陈苏阳@2023-11-14
* Function Name  :  ble_socp_notify_is_enable
* Description    :  判断当前是否使能通知
* Input          :  void
* Output         :  None
* Return         :  bool
*******************************************************************************/
bool ble_socp_notify_is_enable(void)
{
    return g_bBleSocpNotifyIsEnableFlag;
}


/*******************************************************************************
*                           陈苏阳@2023-03-27
* Function Name  :  ble_socp_decode
* Description    :  解码SOCP特征收到的数据包
* Input          :  uint8_t ucDataLen
* Input          :  uint8_t * pData
* Input          :  ble_cgms_socp_datapacket_t * pSocpDatapacket
* Output         :  None
* Return         :  void
*******************************************************************************/
static void ble_socp_decode(uint8_t ucDataLen, uint8_t* pData, ble_socp_datapacket_t* pSocpDatapacket)
{
    pSocpDatapacket->ucOpCode = 0xFF;
    pSocpDatapacket->ucDataLen = 0;
    pSocpDatapacket->pData = NULL;

    // 如果数据包长度大于0
    if (ucDataLen > 0)
    {
        // 数据包第一个字节为操作码
        pSocpDatapacket->ucOpCode = pData[0];
    }
    if (ucDataLen > 1)
    {
        // 如果数据包长度大于1,则除了第一个字节是操作码以外,后面的都是携带的数据
        pSocpDatapacket->ucDataLen = ucDataLen - 1;
        pSocpDatapacket->pData = &pData[1];
    }
}


/*******************************************************************************
*                           陈苏阳@2023-03-27
* Function Name  :  ble_socp_encode
* Description    :  编码要发送的SOCP数据包
* Input          :  const ble_socp_rsp_t * pSocpRsp
* Input          :  uint8_t * p_data
* Output         :  None
* Return         :  uint8_t
*******************************************************************************/
uint8_t ble_socp_encode(const ble_socp_rsp_t* pSocpRsp, uint8_t* pData)
{
    uint8_t ucLen = 0;
    if (pData != NULL)
    {
        // 提取操作码并放到数据包中
        pData[ucLen++] = pSocpRsp->ucOpCode;

        // 如果是以下操作码(回应包中不含数据)
        if (
            (pSocpRsp->ucOpCode != SOCP_READ_CGM_COMMUNICATION_INTERVAL_RESPONSE)
            && (pSocpRsp->ucOpCode != SOCP_READ_GLUCOSE_CALIBRATION_VALUE_RESPONSE)
            && (pSocpRsp->ucOpCode != SOCP_READ_PRM_RESPONSE)
            )
        {
            // 则直接在后面添加回应操作码以及回应结果码
            pData[ucLen++] = pSocpRsp->ucReqOpcode;
            pData[ucLen++] = pSocpRsp->ucRspCode;
        }

        // 否则在后面添加数据
        for (uint8_t i = 0; i < pSocpRsp->ucSizeVal; i++)
        {
            pData[ucLen++] = pSocpRsp->ucRespVal[i];
        }
    }
    // 计算并添加CRC
    ucLen += uint16_encode(do_crc(pData, ucLen), &pData[ucLen]);

    return ucLen;
}

/*******************************************************************************
*                           陈苏阳@2023-03-27
* Function Name  :  socp_send
* Description    :  发送SOCP数据包
* Input          :  ble_event_info_t BleEventInfo
* Input          :  ble_socp_rsp_t SocpRspDatapacket
* Output         :  None
* Return         :  ret_code_t
*******************************************************************************/
static ret_code_t socp_send(ble_event_info_t BleEventInfo, ble_socp_rsp_t SocpRspDatapacket)
{
    uint8_t EncodedRespDatapacketBuffer[25];
    uint8_t ucLen;
    // 编码数据包
    ucLen = ble_socp_encode(&SocpRspDatapacket, EncodedRespDatapacketBuffer);
    // 发送数据包
    elog_hexdump("socp_send", 8, EncodedRespDatapacketBuffer, ucLen);

    // 发送数据包
    elog_hexdump("socp_send(encrpty)", 8, EncodedRespDatapacketBuffer, ucLen);

	if ((ble_socp_notify_is_enable()) && app_have_a_active_ble_connect())
	{
		sl_status_t sc;
		sc = sl_bt_gatt_server_send_notification(BleEventInfo.ucConidx, BleEventInfo.usHandle, ucLen, EncodedRespDatapacketBuffer);
		if (sc == SL_STATUS_OK)
		{
			//app_global_get_app_state()->bSentSocpSuccess = false;
			return RET_CODE_SUCCESS;
		}
		else
		{
			log_w("sl_bt_gatt_server_send_notification fail:%d", sc);
		}
	}
	else
	{
		if (ble_socp_notify_is_enable() == false)
		{
			log_w("ble socp notify not enable");
		}
        if (!app_have_a_active_ble_connect())
		{
			log_w("ble connected is false");
		}
	}
	return RET_CODE_FAIL;
}

/*******************************************************************************
*                           陈苏阳@2024-02-20
* Function Name  :  cgms_socp_stop_session_event_callback
* Description    :  停止测量事件回调
* Input          :  void
* Output         :  None
* Return         :  void
*******************************************************************************/
void cgms_socp_stop_session_event_callback(void)
{
    app_glucose_meas_stop();
    cmgs_db_force_write_flash();
    // 设置电极两端电压
    //cem102_set_we1_dac(0);
}

/*******************************************************************************
*                           陈苏阳@2024-02-22
* Function Name  :  cgms_socp_start_session_event_callback
* Description    :  开始测量事件回调
* Input          :  void
* Output         :  None
* Return         :  void
*******************************************************************************/
void cgms_socp_start_session_event_callback(void)
{
    // 血糖测量开始
    app_glucose_meas_start();

    // 清空历史数据
    cgms_db_reset();
}

/*******************************************************************************
*                           陈苏阳@2024-02-22
* Function Name  :  cgms_socp_write_cgm_communication_interval_event_callback
* Description    :  写CGM通讯间隔事件回调
* Input          :  void
* Output         :  None
* Return         :  void
*******************************************************************************/
void cgms_socp_write_cgm_communication_interval_event_callback(void)
{

}

/*******************************************************************************
*                           陈苏阳@2023-10-18
* Function Name  :  on_socp_value_write
* Description    :  SOCP收到数据
* Input          :  ble_event_info_t BleEventInfo
* Input          :  uint16_t usLen
* Input          :  uint8_t* pData
* Output         :  None
* Return         :  void
*******************************************************************************/
void on_socp_value_write(ble_event_info_t BleEventInfo, uint16_t usLen, uint8_t* pData)
{
    ble_socp_datapacket_t  SocpRequest;
    ble_socp_rsp_t  RspRequest;

    ble_socp_decode(usLen, pData, &SocpRequest);
    elog_hexdump("socp_rav", 8, pData, usLen);
    log_i("socp_request.opcode:%d", SocpRequest.ucOpCode);

    RspRequest.ucOpCode = SOCP_RESPONSE_CODE;
    RspRequest.ucReqOpcode = SocpRequest.ucOpCode;
    RspRequest.ucRspCode = SOCP_RSP_OP_CODE_NOT_SUPPORTED;
    RspRequest.ucSizeVal = 0;


    switch (SocpRequest.ucOpCode)
    {
        // 如果是写入CGM通讯间隔命令
    case SOCP_WRITE_CGM_COMMUNICATION_INTERVAL:
        if (SocpRequest.ucDataLen > 3)
    {
        	RspRequest.ucRspCode = SOCP_RSP_INVALID_OPERAND;
        }
        else
        {
        	RspRequest.ucRspCode = SOCP_RSP_SUCCESS;
            /*
                        // 如果写入间隔为0xFF
                        if (p_cgms->comm_interval == 0xFF)
                        {
                            // 改回1
                            p_cgms->comm_interval = 1;
                        }
                        // 如果当前CGM已经启动
                        if (p_cgms->is_session_started)
                        {
                            //softStop(SENSOR_MANUAL_STOP);
                        }
                        // 如果当前要设置的通讯间隔不为0
                        if (p_cgms->comm_interval != 0)
                        {
                            // todo:待重构后实现
                            //app_env_cs.glucoseCommInterval = p_cgms->comm_interval;
                        }
                        */
        }
        break;
    // 如果是读取CGM通讯间隔命令
    case SOCP_READ_CGM_COMMUNICATION_INTERVAL:
    	RspRequest.ucRspCode = SOCP_READ_CGM_COMMUNICATION_INTERVAL_RESPONSE;
        // todo:待重构后实现
    	RspRequest.ucSizeVal++;
        break;
    // 如果是开始CGM命令
    case SOCP_START_THE_SESSION:
    {
        if (app_global_get_app_state()->is_session_started)
        {
            // CRC错误
        	RspRequest.ucRspCode = SOCP_RSP_PROCEDURE_NOT_COMPLETED;
        }
        else
        {
            log_i("!!SOCP_START_THE_SESSION   OK!!");
            RspRequest.ucRspCode = SOCP_RSP_SUCCESS;
            event_push(MAIN_LOOP_EVENT_SOCP_START_SESSION_EVENT);
            }
                break;
            }
    case SOCP_SENSOR_CODE:
            {
        uint16_t usSensorCode;
        usSensorCode = uint16_decode(SocpRequest.pData + 1);

                // 更新CGM Status中的工厂校准码
        att_get_cgm_status()->usFactoryCode = usSensorCode;
        //设置传感器Code
        //sensorK = ((float)usSensorCode / 1000);
        log_i("sensorcode update:%d", usSensorCode);

        // 如果Code为0,则说明Code无效
        if (usSensorCode == 0)
        {
        	RspRequest.ucRspCode = SOCP_RSP_INVALID_OPERAND;
            }
        else
        {
            log_i("sensor code update done");
        	RspRequest.ucRspCode = SOCP_RSP_SUCCESS;
        	app_global_get_app_state()->isfs = true;
          //cur_get_cur_error_value(sensorK);

            // 在CGM status Char中设置code码
            att_get_cgm_status()->usFactoryCode = usSensorCode;
        }
        break;
    }
    case SOCP_START_FOTA:
    {
        // 触发进入OTA
        //Sys_Fota_StartDfu(1);
        RspRequest.ucRspCode = SOCP_RSP_SUCCESS;
        break;
    }
    case SOCP_READ_RESET_REG:// 读取复位原因寄存器
    {
      /*
        extern uint32_t rest_dig_status;
        extern uint32_t acs_reset_status;
        memcpy(&(RspRequest.ucRespVal[0]), &rest_dig_status, 4);
        memcpy(&(RspRequest.ucRespVal[4]), &acs_reset_status, 4);
        RspRequest.ucSizeVal = 8;
        */
        break;
        }
    case SOCP_READ_HARD_FAULT_INFO:// 读取硬错误信息
    {


        break;
    }
    // 如果是停止CGM命令
    case SOCP_STOP_THE_SESSION:
    {
        RspRequest.ucOpCode = SOCP_RSP_SUCCESS;
        event_push(MAIN_LOOP_EVENT_SOCP_STOP_SESSION_EVENT);
            break;
        }
    case SOCP_WRITE_GLUCOSE_CALIBRATION_VALUE: //write calibration
        if (usLen >= 13)
        {
            memcpy((void*)calData, (void*)(pData + 1), 10);
            calDataFlag = 1;
            if (usLen == 13)
        {
                uint16_t concentration = uint16_decode(SocpRequest.pData);
                //usBfFlg = 1;
                //sfCurrBg = (float)concentration / 100.0f;
                //log_i("sfCurrBg:%f\r\n", sfCurrBg);
                RspRequest.ucOpCode = SOCP_RSP_SUCCESS;  //response is 1C 04(opcode) 01(response) EC 6F (CRC)

        }
        else
        {
            	RspRequest.ucOpCode = SOCP_RSP_INVALID_OPERAND;
            }
        }
        break;
    case SOCP_READ_GLUCOSE_CALIBRATION_VALUE:

        break;
    case SOCP_WRITE_PRM://add by woo set parameters 0x61
    // 如果是写入参数命令
        if (SocpRequest.ucDataLen > 16)
        {
        	RspRequest.ucRspCode = SOCP_RSP_INVALID_OPERAND;
        }
        else
        {
            uint8_t index = 0;
            uint8_t PrmNo = 0;
            int16_t tmp;
            PrmNo = SocpRequest.pData[index++];

            if (PrmNo == 0x01) //This is for accurracy calibration  0x 61 01 XX XX CRC
            {
            	RspRequest.ucRspCode = SOCP_RSP_SUCCESS;
                //p_cgms->comm_interval          = socp_request.p_operand[0];

                tmp = uint16_decode(SocpRequest.pData + index);
                g_PrmDb.Pone.prmVD1 = (int32_t)tmp;  //Calibration VD1
                index += 2;

                tmp = uint16_decode(SocpRequest.pData + index);
                g_PrmDb.Pone.prmRL1 = (int32_t)tmp;
                index += 2;

                tmp = uint16_decode(SocpRequest.pData + index);
                g_PrmDb.Pone.prmOffset = (int32_t)tmp;
                index += 2;

                tmp = uint16_decode(SocpRequest.pData + index);
                g_PrmDb.Pone.prmAD1 = (int32_t)tmp;
                index += 2;

                tmp = uint16_decode(SocpRequest.pData + index);
                g_PrmDb.Pone.prmRL2 = (int32_t)tmp;
                index += 2;

                tmp = uint16_decode(SocpRequest.pData + index);
                g_PrmDb.Pone.prmAD2 = (int32_t)tmp;
                index += 2;

                g_PrmDb.Pone.rsvd = 0;

                // 计算CRC
                g_PrmDb.Pone.prmCrc1 = do_crc((uint8_t*)&g_PrmDb.Pone, sizeof(P1_t) - 2);
            }
            else if (PrmNo == 0x03)//cmd = 0x 61 03 XX XX CRC can be hidden if necessary
            {
            	RspRequest.ucRspCode = SOCP_RSP_SUCCESS;

                g_PrmDb.P3.prmDX0 = *(SocpRequest.pData + index);
                index += 1;
                g_PrmDb.P3.prmDX1 = *(SocpRequest.pData + index);
                index += 1;
                g_PrmDb.P3.prmDX2 = *(SocpRequest.pData + index);
                index += 1;
                g_PrmDb.P3.prmDX3 = *(SocpRequest.pData + index);
                index += 1;
                g_PrmDb.P3.prmDX4 = *(SocpRequest.pData + index);
                index += 1;
                g_PrmDb.P3.prmDX5 = *(SocpRequest.pData + index);
                index += 1;
                g_PrmDb.P3.prmDX6 = *(SocpRequest.pData + index);
                index += 1;
                g_PrmDb.P3.prmDX7 = *(SocpRequest.pData + index);
                index += 1;
                g_PrmDb.P3.prmDX8 = *(SocpRequest.pData + index);
                index += 1;
                g_PrmDb.P3.prmDX9 = *(SocpRequest.pData + index);
                index += 1;
                g_PrmDb.P3.prmDXA = *(SocpRequest.pData + index);
                index += 1;
                g_PrmDb.P3.prmDXB = *(SocpRequest.pData + index);
                index += 1;

                g_PrmDb.P3.prmCrc3 = do_crc((uint8_t*)&g_PrmDb.P3, sizeof(P3_t) - 2);
            }
            else if (PrmNo == 0x04)//write SN time part
            {
            	RspRequest.ucRspCode = SOCP_RSP_SUCCESS;

            	g_PrmDb.P4.prmWMY[0] = *(SocpRequest.pData + index);
                index += 1;
                g_PrmDb.P4.prmWMY[1] = *(SocpRequest.pData + index);
                index += 1;
                g_PrmDb.P4.prmWMY[2] = *(SocpRequest.pData + index);
                index += 1;
                g_PrmDb.P4.prmWMY[3] = 0;

                tmp = uint16_decode(SocpRequest.pData + index);
                g_PrmDb.P4.SN = (int16_t)tmp;
                index += 2;

                g_PrmDb.P4.prmCrc4 = do_crc((uint8_t*)&g_PrmDb.P4, sizeof(P4_t) - 2);
            }
            else if (PrmNo == 0xFA)// 写入启动时间
            {
            	ble_cgms_socp_write_start_time_datapacket_t SetStartTimeDatapcket;
                memcpy(&SetStartTimeDatapcket, pData, sizeof(SetStartTimeDatapcket));

                RspRequest.ucRspCode = SOCP_RSP_SUCCESS;
                
                cgms_update_sst_and_time_zone(SetStartTimeDatapcket.usYear, SetStartTimeDatapcket.ucMonth, SetStartTimeDatapcket.ucDay, SetStartTimeDatapcket.ucHour, SetStartTimeDatapcket.ucMinute, SetStartTimeDatapcket.ucSecond, SetStartTimeDatapcket.ucTimeZone, SetStartTimeDatapcket.ucDataSaveingTime);
                }
            else if (PrmNo == 0xFB)//write debug variable ������������õ�ͨ��Э��
                {
                RspRequest.ucRspCode = SOCP_RSP_SUCCESS;

                app_glucose_meas_set_glucose_meas_interval(*(SocpRequest.pData + index)); //change measure interval
                index += 1;
                simMax = *(SocpRequest.pData + index); //��what is this
                index += 1;
                simMin = *(SocpRequest.pData + index);
                index += 1;
                simDelta = *(SocpRequest.pData + index);
                index += 1;

        }
            else if (PrmNo == 0xFC)//cmd = 0x 61 FC XX XX CRC
    {
                RspRequest.ucRspCode = SOCP_RSP_SUCCESS;
                uint16_t usBitCmd = uint16_decode(SocpRequest.pData + index);
                if (usBitCmd & 0x0001)
        {
                    //cem102_set_we1_dac(ANALOG_ELECTRODE_POLARIZATION_VOL_LOW);
        }
        else
        {
                    //cem102_set_we1_dac(ANALOG_ELECTRODE_POLARIZATION_VOL_HIGH);
                }	// high volts //不必等待极化可以直接设置高低电压

                if (usBitCmd & 0x0004)
            {

            }
                else
            {


                }//VPER_SHDN	 shut down //测试软件中的“关闭探头”
            }
            else if (PrmNo == 0xFD)//cmd = 0x 61 FD 5F A0
            {
                RspRequest.ucRspCode = SOCP_RSP_SUCCESS;
            }
            else if (PrmNo == 0xFE)// 固化参数
    {
                cgms_prm_db_write_flash();
                RspRequest.ucRspCode = SOCP_RSP_SUCCESS;
        }
        else
        {
                RspRequest.ucRspCode = SOCP_RSP_INVALID_OPERAND;
            }

        }
        break;
    case SOCP_READ_PRM://add by woo // ��ȡ���� 0x62
        if (SocpRequest.ucDataLen > 4)
        {
            RspRequest.ucRspCode = SOCP_RSP_INVALID_OPERAND;

        }
        else
        {
            uint8_t index = 0;
            uint8_t PrmNo = 0;
            PrmNo = SocpRequest.pData[0];
            if (PrmNo == 0x01) //"��ȡ��һ�����"
            {
            	RspRequest.ucOpCode = SOCP_READ_PRM_RESPONSE;

                *(RspRequest.ucRespVal) = PrmNo;
                index += 1;

                uint16_encode(g_PrmDb.Pone.prmVD1, RspRequest.ucRespVal + index);
                index += 2;
                uint16_encode(g_PrmDb.Pone.prmRL1, RspRequest.ucRespVal + index);
                index += 2;
                uint16_encode(g_PrmDb.Pone.prmOffset, RspRequest.ucRespVal + index);
                index += 2;
                uint16_encode(g_PrmDb.Pone.prmAD1, RspRequest.ucRespVal + index);
                index += 2;
                uint16_encode(g_PrmDb.Pone.prmRL2, RspRequest.ucRespVal + index);
                index += 2;
                uint16_encode(g_PrmDb.Pone.prmAD2, RspRequest.ucRespVal + index);
                index += 2;
                RspRequest.ucSizeVal = index;
            }
            else if (PrmNo == 0x02)//read cgm algorithm coff ////difference in i3 used to be A4 ��ȫ���ȡ�㷨������APP���ܲ����ã����ǿ��Ը�
            {
            	RspRequest.ucOpCode = SOCP_READ_PRM_RESPONSE;

                *(RspRequest.ucRespVal) = PrmNo;
                index += 1;
                /*
                *(RspRequest.ucRespVal + index) = (uint8_t)(vD_X0);
                index += 1;
                *(RspRequest.ucRespVal + index) = (uint8_t)(vD_X1 * 100);
                index += 1;
                *(RspRequest.ucRespVal + index) = vD_X2;
                index += 1;
                *(RspRequest.ucRespVal + index) = (uint8_t)(vD_X3 * 10);
                index += 1;
                *(RspRequest.ucRespVal + index) = (uint8_t)(vD_X4 * 10);
                index += 1;
                *(RspRequest.ucRespVal + index) = (uint8_t)(vD_X5 * 10);
                index += 1;
                *(RspRequest.ucRespVal + index) = vD_X6;
                index += 1;
                *(RspRequest.ucRespVal + index) = vD_X7;
                index += 1;
                *(RspRequest.ucRespVal + index) = (uint8_t)(vD_X8 * 1000);
                index += 1;
                *(RspRequest.ucRespVal + index) = (uint8_t)(vD_X9 * 10);
                index += 1;
                *(RspRequest.ucRespVal + index) = (uint8_t)(vD_XA * 10);
                index += 1;
                *(RspRequest.ucRespVal + index) = vD_XB;
                index += 1;
                */

                RspRequest.ucSizeVal = index;
            }
            else if (PrmNo == 0xA0)//read sst
            {
            	RspRequest.ucOpCode = SOCP_READ_PRM_RESPONSE;

                *(RspRequest.ucRespVal) = PrmNo;
                index += 1;

                uint16_encode(g_mSST.date_time.time_info.year, RspRequest.ucRespVal + index);
                index += 2;
                *(RspRequest.ucRespVal + index) = g_mSST.date_time.time_info.month;
                index += 1;
                *(RspRequest.ucRespVal + index) = g_mSST.date_time.time_info.day;
                index += 1;
                *(RspRequest.ucRespVal + index) = g_mSST.date_time.time_info.hour;
                index += 1;
                *(RspRequest.ucRespVal + index) = g_mSST.date_time.time_info.minute;
                index += 1;
                *(RspRequest.ucRespVal + index) = g_mSST.date_time.time_info.sec;
                index += 1;
                *(RspRequest.ucRespVal + index) = g_mSST.date_time.time_zone;
                index += 1;
                *(RspRequest.ucRespVal + index) = g_mSST.date_time.time_info.month;
                index += 1;
                *(RspRequest.ucRespVal + index) = g_mSST.dst;
                index += 1;

                RspRequest.ucSizeVal = index;
            }
            else if (PrmNo == 0xA3)//read sim parameters ����
            {
            	RspRequest.ucOpCode = SOCP_READ_PRM_RESPONSE;

                *(RspRequest.ucRespVal) = PrmNo;
                index += 1;

                *(RspRequest.ucRespVal + index) = simMax;//
                index += 1;
                *(RspRequest.ucRespVal + index) = simMin;//
                index += 1;
                *(RspRequest.ucRespVal + index) = simDelta;//
                index += 1;

                RspRequest.ucSizeVal = index;
            }
            else if (PrmNo == 0xA4)////difference in i3 //"��ȡ�����" ����ǰ�ȫ��
            {
            	RspRequest.ucOpCode = SOCP_READ_PRM_RESPONSE;

                *(RspRequest.ucRespVal) = PrmNo;
                index += 1;

                //Get_TRNG(p_cgms->socp_response.resp_val+index, 4, 1);
                RspRequest.ucRespVal[1] = 0x11;
                RspRequest.ucRespVal[2] = 0x22;
                RspRequest.ucRespVal[3] = 0x33;
                RspRequest.ucRespVal[4] = 0x44;

                index += 4;
                RspRequest.ucSizeVal = index;
            }

            else
            {
                RspRequest.ucRspCode = SOCP_RSP_INVALID_OPERAND;
            }
        }
        break;
    default:
        RspRequest.ucRspCode = SOCP_RSP_OP_CODE_NOT_SUPPORTED;
        break;
    }

    socp_send(BleEventInfo,RspRequest);
}


/*******************************************************************************
*                           陈苏阳@2023-10-18
* Function Name  :  cgms_socp_check_production_cmd
* Description    :  检查是否为生产命令
* Input          :  uint8_t * pData
* Input          :  uint16_t usLen
* Output         :  None
* Return         :  bool
*******************************************************************************/
bool cgms_socp_check_production_cmd(uint8_t* pData, uint16_t usLen)
{
    if ((pData[0] == SOCP_WRITE_PRM) && (pData[1] == 0x01) && (usLen == 16))return true;//write cal parameters
    if ((pData[0] == SOCP_WRITE_PRM) && (pData[1] == 0x04) && (usLen == 9))return true;//write sn
    if ((pData[0] == SOCP_WRITE_PRM) && (pData[1] == 0xfc) && (usLen == 6))return true;//write hi low voltage
    if ((pData[0] == SOCP_WRITE_PRM) && (pData[1] == 0xFE) && (usLen == 4))return true;//Set parameter permanently
    if ((pData[0] == SOCP_READ_PRM) && (pData[1] == 0x01) && (usLen == 4))return true;//read cal parameters
    if ((pData[0] == SOCP_READ_PRM) && (pData[1] == 0xA4) && (usLen == 4))return true;//Get radom num

    if ((pData[0] == SOCP_START_THE_SESSION) && (pData[1] == 0) && (usLen == 4))return true;// start senssor
    if ((pData[0] == SOCP_STOP_THE_SESSION) && (pData[1] == 0) && (usLen == 4))return true;// STOP senssor
    if ((pData[0] == SOCP_START_AD_CALI) && (pData[1] == 0) && (usLen == 4))return true;
    return false;
}


/******************* (C) COPYRIGHT 2023 陈苏阳 **** END OF FILE ****************/
