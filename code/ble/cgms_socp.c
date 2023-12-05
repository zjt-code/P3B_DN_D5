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
static bool g_bProduction = false;
extern float sfCurrBg;//add by woo
uint8_t	caliTag;
/* Private function prototypes -----------------------------------------------*/
static ret_code_t socp_send(ble_event_info_t BleEventInfo, ble_socp_rsp_t SocpRspDatapacket);
bool cgms_socp_check_production_cmd(uint8_t* pData, uint16_t usLen);

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
static void ble_socp_decode(uint8_t ucDataLen, uint8_t* pData, ble_cgms_socp_datapacket_t* pSocpDatapacket)
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
			&& (pSocpRsp->ucOpCode != SOCP_READ_PATIENT_HIGH_ALERT_LEVEL_RESPONSE)
			&& (pSocpRsp->ucOpCode != SOCP_READ_PATIENT_LOW_ALERT_LEVEL_RESPONSE)
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

#ifdef CGMS_ENCRYPT_ENABLE
	uint8_t ucCipher[16];
	// 如果当前处理的是生产命令,则跳过加密,如果是正常命令,则走加密流程发包
	if (g_bProduction == false)
	{
		mbedtls_aes_pkcspadding(EncodedRespDatapacketBuffer, 16);
		cgms_aes128_encrpty(EncodedRespDatapacketBuffer, ucCipher);
		memcpy(EncodedRespDatapacketBuffer, ucCipher, 16);

		// 如果加密就是需要凑足16个字节
		ucLen = 16;
	}
#endif
    // 发送数据包
    elog_hexdump("socp_send(encrpty)", 8, EncodedRespDatapacketBuffer, ucLen);

	if ((ble_socp_notify_is_enable()) && (app_global_get_app_state()->bBleConnected == true))
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
		if (app_global_get_app_state()->bBleConnected == false)
		{
			log_w("ble connected is false");
		}
	}
	return RET_CODE_FAIL;
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
	uint8_t ucTempDatapacketBuffer[16];
	ble_socp_rsp_t SocpResponseDatapcket;

	// 如果当前是生产命令,生产标志位置位
	if (cgms_socp_check_production_cmd(pData, usLen))
	{
		log_d("check production cmd.");
		g_bProduction = true;
	}
	else
	{
		// 如果不是生产测试命令

	// 如果设置了密码
		if (att_get_feature()->ucPasswordExist)
		{
			log_d("password is seted.");
			// 走解密流程
#ifdef CGMS_ENCRYPT_ENABLE
			cgms_aes128_decrpty(pData, ucTempDatapacketBuffer);
			memcpy(pData, ucTempDatapacketBuffer, 16);
#endif

			// 如果当前的命令不是密码验证命令,且当前密码还未验证成功
			if ((pData[0] != SOCP_VERIFY_PWD) && (app_global_get_app_state()->bCgmsPwdVerifyOk == false))
			{
				// 返回操作非法回应包
				SocpResponseDatapcket.ucOpCode = SOCP_RESPONSE_ILLEGAL_CODE;
				SocpResponseDatapcket.ucReqOpcode = 0X00;
				SocpResponseDatapcket.ucRspCode = 0X00;
				SocpResponseDatapcket.ucSizeVal = 0;
				socp_send(BleEventInfo, SocpResponseDatapcket);
				return;
			}
		}
		// 如果还没设置密码
		else
		{
			log_d("no valid password");
				// 如果也不是设置密码命令
				if ((pData[0] != SOCP_SET_PASSWORD))
				{
					SocpResponseDatapcket.ucOpCode = SOCP_RESPONSE_ILLEGAL_CODE;
					SocpResponseDatapcket.ucReqOpcode = 0X00;
					SocpResponseDatapcket.ucRspCode = 0X00;
					SocpResponseDatapcket.ucSizeVal = 0;
					socp_send(BleEventInfo, SocpResponseDatapcket);
					return;
				}
				else
				{
					// 如果是设置密码命令
#ifdef CGMS_ENCRYPT_ENABLE

				// 长度不正确,返回报错
					if (usLen != 17)
					{
						log_d("Len is Err");
						SocpResponseDatapcket.ucOpCode = SOCP_RESPONSE_CODE;
						SocpResponseDatapcket.ucReqOpcode = SOCP_SET_PASSWORD;
						SocpResponseDatapcket.ucRspCode = 0X03;
						SocpResponseDatapcket.ucSizeVal = 0;
						socp_send(BleEventInfo, SocpResponseDatapcket);
						return;
					}
					else
					{
						cgms_aes128_decrpty(&pData[1], ucTempDatapacketBuffer);
						memcpy(&pData[1], ucTempDatapacketBuffer, 16);
						elog_hexdump("datapacket decode", 8, pData, 16);
					}
#endif
				}
		}
	}

	ble_cgms_socp_datapacket_t SocpDatapacket;
	// 解码SOCP数据包并填充结构体
	ble_socp_decode(usLen, pData, &SocpDatapacket);

	elog_hexdump("socp_rav", 8, pData, usLen);
	log_i("SocpDatapacket.opcode:%d", SocpDatapacket.ucOpCode);

	SocpResponseDatapcket.ucOpCode = SOCP_RESPONSE_CODE;
	SocpResponseDatapcket.ucReqOpcode = SocpDatapacket.ucOpCode;
	SocpResponseDatapcket.ucRspCode = SOCP_GENERAL_RSP_OP_CODE_NOT_SUPPORTED;
	SocpResponseDatapcket.ucSizeVal = 0;



	// 根据操作码执行不同命令
	switch (SocpDatapacket.ucOpCode)
	{
		// 如果是写入CGM通讯间隔命令
	case SOCP_WRITE_CGM_COMMUNICATION_INTERVAL:
	{
		log_d("SOCP_WRITE_CGM_COMMUNICATION_INTERVAL");
		// 给回应包添加默认的回应码
		SocpResponseDatapcket.ucRspCode = SOCP_GENERAL_RSP_SUCCESS;

		// 提取命令包中携带的通讯间隔并写入结构体
		//p_cgms->comm_interval = SocpDatapacket.pData[0];

		// 如果数据长度不正确
		if (SocpDatapacket.ucDataLen > 3)
		{
			// 返回无效命令
			SocpResponseDatapcket.ucRspCode = SOCP_GENERAL_RSP_INVALID_OPERAND;
		}
		else
		{
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
	}
	// 如果是读取CGM通讯间隔命令
	case SOCP_READ_CGM_COMMUNICATION_INTERVAL:
	{
		log_d("SOCP_READ_CGM_COMMUNICATION_INTERVAL");
		SocpResponseDatapcket.ucOpCode = SOCP_READ_CGM_COMMUNICATION_INTERVAL_RESPONSE;
		// todo:待重构后实现
		//SocpResponseDatapcket.ucRespVal[0] = p_cgms->comm_interval;
		SocpResponseDatapcket.ucSizeVal++;
		break;
	}
	// 如果是开始CGM命令
	case SOCP_START_THE_SESSION:
	{
		log_d("SOCP_START_THE_SESSION");
		// 效验命令的CRC
		if (do_crc(pData, 15) != 0)
		{
			// CRC错误
			SocpResponseDatapcket.ucRspCode = SOCP_START_THE_SESSION_RSP_CODE_CRC_ERR;
			break;
		}

		// 填充结构体
		ble_cgms_socp_start_the_session_datapacket_t SocpStartTheSessionDatapacket;
		memcpy(&SocpStartTheSessionDatapacket, pData, sizeof(SocpStartTheSessionDatapacket));

		// 如果当前CGM已经运行或者处于极化中
		if ((att_get_cgm_status()->ucRunStatus == CGM_MEASUREMENT_SENSOR_STATUS_SESSION_RUNNING) || (att_get_cgm_status()->ucRunStatus == CGM_MEASUREMENT_SENSOR_STATUS_SESSION_WARM_UP))
		{
			// 返回对应错误码
			SocpResponseDatapcket.ucRspCode = SOCP_START_THE_SESSION_RSP_CODE_IS_STARTED;
		}
		else
		{
			// 如果CGM因为APP的停止命令而处于已经停止的状态
			if (att_get_cgm_status()->ucRunStatus == CGM_MEASUREMENT_SENSOR_STATUS_SESSION_COMMAND_STOPPED)
			{
				// 则不能重新启动,返回错误码
				SocpResponseDatapcket.ucRspCode = SOCP_START_THE_SESSION_RSP_CODE_IS_STOPED;
				break;
			}

			// 如果CGM已到期
			if (att_get_cgm_status()->ucRunStatus == CGM_MEASUREMENT_SENSOR_STATUS_SENSION_EXPRIED)
			{
				// 则不能重新启动,返回错误码
				SocpResponseDatapcket.ucRspCode = SOCP_START_THE_SESSION_RSP_CODE_IS_END;
				break;
			}

			// 计算工厂校准码
			float fTmpSensorK = (float)SocpStartTheSessionDatapacket.usFactoryCode / 1000.0f;
			log_d("SensorK:%f", fTmpSensorK);
			// 如果工厂校准码不合法
			if ((fTmpSensorK < SOCP_SET_SENSOR_CODE_MIN_ERR_VAL) || (fTmpSensorK > SOCP_SET_SENSOR_CODE_MAX_ERR_VAL))
			{
				log_w("SensorK err");
				// 返回错误码
				SocpResponseDatapcket.ucRspCode = SOCP_START_THE_SESSION_RSP_CODE_SENSOR_CODE_ERR;
				break;
			}
			else
			{
				// 生效工厂校准码
				// todo: sensorK = fTmpSensorK;

				// 更新CGM Status中的工厂校准码
				att_get_cgm_status()->usFactoryCode = SocpStartTheSessionDatapacket.usFactoryCode;
			}

            // 设置返回结果
			SocpResponseDatapcket.ucRspCode = SOCP_START_THE_SESSION_RSP_CODE_SUCCESS;

			// 更新feature char中的启动来源(高四位)
			att_get_feature()->ucStartBy = SocpStartTheSessionDatapacket.ucFrom<<4;

			// 更新start time char中的启动时间
			att_get_start_time()->uiStartTime = SocpStartTheSessionDatapacket.uiStartTime;

			// 更新start time char中的时区
			att_get_start_time()->ucTimeZone = SocpStartTheSessionDatapacket.ucTimeZone;

			// 更新CGM Status中的运行状态为极化中
			app_global_get_app_state()->status = CGM_MEASUREMENT_SENSOR_STATUS_SESSION_WARM_UP;
			att_get_cgm_status()->ucRunStatus = app_global_get_app_state()->status;

			// 更新Feature char的CRC
			att_update_feature_char_data_crc();

			// 更新Start Time char的CRC
			att_update_start_time_char_data_crc();

			// 更新CGM Status char的CRC
			att_update_cgm_status_char_data_crc();

			// 更新启动时间
			cgms_update_sst_and_time_zone(SocpStartTheSessionDatapacket.uiStartTime, SocpStartTheSessionDatapacket.ucTimeZone);

			// 更新record_index中的SST
			cgms_db_record_index_update_sst(g_mSST);

            // 清空历史数据
            cgms_db_reset();

			// 开始应用层血糖测量
			app_glucose_meas_start();
		}
		break;
	}
	// 如果是停止CGM命令
	case SOCP_STOP_THE_SESSION:
	{
		log_d("SOCP_STOP_THE_SESSION");
		uint8_t ucIsStopedFlag = 0;
		// 判断发射器当前是否已经处于停止状态
		switch (att_get_cgm_status()->ucRunStatus)
		{
		case CGM_MEASUREMENT_SENSOR_STATUS_SESSION_STOPPED:							// CGM结束
		case CGM_MEASUREMENT_SENSOR_STATUS_SESSION_COMMAND_STOPPED:					// 由于APP发送停止命令导致的停止
		case CGM_MEASUREMENT_SENSOR_STATUS_SESSION_HARDFAULT_STOPPED:				// MCU硬故障复位
		case CGM_MEASUREMENT_SENSOR_STATUS_SESSION_M3RESET_STOPPED:					// 由于MCU复位导致的停止
		case CGM_MEASUREMENT_SENSOR_STATUS_SENSION_EXPRIED:							// CGM到期停止
		case CGM_MEASUREMENT_SENSOR_STATUS_SESSION_SENSOR_ABNORMAL:					// 传感器异常,等待恢复(预设3小时)
		case CGM_MEASUREMENT_SENSOR_STATUS_SESSION_INEFFECTIVE_IMPLANTATION:		// 无效植入,请更换传感器(数据停止采样,蓝牙广播继续)
		case CGM_MEASUREMENT_SENSOR_STATUS_UNEXPECTED_STOP1:						// 意外停止(情况1)
		case CGM_MEASUREMENT_SENSOR_STATUS_UNEXPECTED_STOP2:						// 意外停止(情况2)
		{
			// 发射器已经处于停止状态
			ucIsStopedFlag = 1;
			break;
		}
		default:
		{
			break;
		}
		}

		// 发射器已经处于停止状态
		if (ucIsStopedFlag)
		{
			// 返回错误码
			SocpResponseDatapcket.ucRspCode = SOCP_STOP_THE_SESSION_RSP_CODE_IS_STOPED;
			break;
		}

		// 清除算法参数 todo:
		//sfCurrK = 0;
		//sensorK = sfCurrK;

		// 更新CGM Status中的运行状态为停止状态
		att_get_cgm_status()->ucRunStatus = CGM_MEASUREMENT_SENSOR_STATUS_SESSION_COMMAND_STOPPED;

		// 更新CGM Status char的CRC
		att_update_cgm_status_char_data_crc();

		// 停止血糖测量
		app_glucose_meas_stop();
		break;
	}

	// 如果是写入血糖校准值命令
	case SOCP_WRITE_GLUCOSE_CALIBRATION_VALUE:
	{
		log_d("SOCP_WRITE_GLUCOSE_CALIBRATION_VALUE");
		// 判断长度是否正确
		if (usLen < 13)
		{
			// 命令长度错误
			SocpResponseDatapcket.ucRspCode = SOCP_WRITE_GLUCOSE_CALIBRATION_RSP_CODE_COMMAND_LEN_ERR;
			break;
		}

		// 效验命令的CRC
		if (do_crc(pData, sizeof(ble_cgms_socp_write_glucose_calibration_datapacket_t)) != 0)
		{
			// CRC错误
			SocpResponseDatapcket.ucRspCode = SOCP_WRITE_GLUCOSE_CALIBRATION_RSP_CODE_CRC_ERR;
			break;
		}

		// 填充结构体
		ble_cgms_socp_write_glucose_calibration_datapacket_t SocpWriteGlucoseCalibrationDatapacket;
		memcpy(&SocpWriteGlucoseCalibrationDatapacket, pData, sizeof(SocpWriteGlucoseCalibrationDatapacket));

		// 判断当前血糖趋势是否稳定
		if ((app_global_get_app_state()->ucCgmTrend != CGM_TREND_DOWN_DOWN) && (app_global_get_app_state()->ucCgmTrend != CGM_TREND_STABLE) && (app_global_get_app_state()->ucCgmTrend != CGM_TREND_SLOW_UP))
		{
			SocpResponseDatapcket.ucRspCode = SOCP_WRITE_GLUCOSE_CALIBRATION_RSP_CODE_GLUCOSE_FLUCTUATE;
			break;
		}

		// 判断当前运行状态是否为正常
		if (att_get_cgm_status()->ucRunStatus != CGM_MEASUREMENT_SENSOR_STATUS_SESSION_RUNNING)
		{
			SocpResponseDatapcket.ucRspCode = SOCP_WRITE_GLUCOSE_CALIBRATION_RSP_CODE_SENSOR_ABNORMAL;
			break;
		}

		// 判断校准值是否在合理区间
		if ((SocpWriteGlucoseCalibrationDatapacket.usCalibration >= 222) || (SocpWriteGlucoseCalibrationDatapacket.usCalibration <= 22))
		{
			SocpResponseDatapcket.ucRspCode = SOCP_WRITE_GLUCOSE_CALIBRATION_RSP_CODE_RANGE_OUT;
			break;
		}

		/*
		 * todo:
		// VD_X7是校准停止时间60 60*3 = 180
		if (app_glucose_get_records_current_offset() >= vD_X7 - 1)
		{
			sfCurrBg = SocpWriteGlucoseCalibrationDatapacket.usCalibration / 10.0;
			usBfFlg = 1;
			// 校准次数++
			caliTag++;
		}
		else
		{
			SocpResponseDatapcket.ucRspCode = SOCP_WRITE_GLUCOSE_CALIBRATION_RSP_CODE_WARM_UP;
		}
		*/
		SocpResponseDatapcket.ucRspCode = SOCP_WRITE_GLUCOSE_CALIBRATION_RSP_CODE_SUCCESS;

		break;
	}
	// 如果是写入参数命令
	case SOCP_WRITE_PRM:
	{
		log_d("SOCP_WRITE_PRM");
		if (SocpDatapacket.ucDataLen > 16)
		{
			SocpResponseDatapcket.ucRspCode = SOCP_GENERAL_RSP_INVALID_OPERAND;
		}
		else
		{
			uint8_t ucPrmNo = 0;
			ucPrmNo = SocpDatapacket.pData[0];
			switch (ucPrmNo)
			{
			case BLE_CGMS_SOCP_WRITE_PRM_TYPE_1:
			{
				// 填充结构体
				ble_cgms_socp_write_prm_type_1_t BleCgmsSocpWritePrmType1Datapacket;
				memcpy(&BleCgmsSocpWritePrmType1Datapacket, pData, sizeof(BleCgmsSocpWritePrmType1Datapacket));

				g_PrmDb.Pone.prmVD1 = (int32_t)BleCgmsSocpWritePrmType1Datapacket.sPrmVD1;
				g_PrmDb.Pone.prmRL1 = (int32_t)BleCgmsSocpWritePrmType1Datapacket.sPrmRL1;
				g_PrmDb.Pone.prmOffset = (int32_t)BleCgmsSocpWritePrmType1Datapacket.sPrmOffset;
				g_PrmDb.Pone.prmAD1 = (int32_t)BleCgmsSocpWritePrmType1Datapacket.sPrmAD1;
				g_PrmDb.Pone.prmRL2 = (int32_t)BleCgmsSocpWritePrmType1Datapacket.sPrmRL2;
				g_PrmDb.Pone.prmAD2 = (int32_t)BleCgmsSocpWritePrmType1Datapacket.sPrmAD2;
				g_PrmDb.Pone.rsvd = 0;

				// 计算CRC
				g_PrmDb.Pone.prmCrc1 = do_crc((uint8_t*)&g_PrmDb.Pone, sizeof(P1_t) - 2);

				SocpResponseDatapcket.ucRspCode = SOCP_GENERAL_RSP_SUCCESS;
				break;
			}
			case BLE_CGMS_SOCP_WRITE_PRM_TYPE_3:
			{
				// 填充结构体
				ble_cgms_socp_write_prm_type_3_t BleCgmsSocpWritePrmType3Datapacket;
				memcpy(&BleCgmsSocpWritePrmType3Datapacket, pData, sizeof(BleCgmsSocpWritePrmType3Datapacket));

				g_PrmDb.P3.prmDX0 = BleCgmsSocpWritePrmType3Datapacket.ucPrmDX0;
				g_PrmDb.P3.prmDX1 = BleCgmsSocpWritePrmType3Datapacket.ucPrmDX1;
				g_PrmDb.P3.prmDX2 = BleCgmsSocpWritePrmType3Datapacket.ucPrmDX2;
				g_PrmDb.P3.prmDX3 = BleCgmsSocpWritePrmType3Datapacket.ucPrmDX3;
				g_PrmDb.P3.prmDX4 = BleCgmsSocpWritePrmType3Datapacket.ucPrmDX4;
				g_PrmDb.P3.prmDX5 = BleCgmsSocpWritePrmType3Datapacket.ucPrmDX5;
				g_PrmDb.P3.prmDX6 = BleCgmsSocpWritePrmType3Datapacket.ucPrmDX6;
				g_PrmDb.P3.prmDX7 = BleCgmsSocpWritePrmType3Datapacket.ucPrmDX7;
				g_PrmDb.P3.prmDX8 = BleCgmsSocpWritePrmType3Datapacket.ucPrmDX8;
				g_PrmDb.P3.prmDX9 = BleCgmsSocpWritePrmType3Datapacket.ucPrmDX9;
				g_PrmDb.P3.prmDXA = BleCgmsSocpWritePrmType3Datapacket.ucPrmDXA;
				g_PrmDb.P3.prmDXB = BleCgmsSocpWritePrmType3Datapacket.ucPrmDXB;
				g_PrmDb.P3.prmDXC = BleCgmsSocpWritePrmType3Datapacket.usPrmDXC;

				g_PrmDb.P3.prmCrc3 = do_crc((uint8_t*)&g_PrmDb.P3, sizeof(P3_t) - 2);

				SocpResponseDatapcket.ucRspCode = SOCP_GENERAL_RSP_SUCCESS;
				break;
			}
			case BLE_CGMS_SOCP_WRITE_PRM_TYPE_4:
			{
				// 填充结构体
				ble_cgms_socp_write_prm_type_4_t BleCgmsSocpWritePrmType4Datapacket;
				memcpy(&BleCgmsSocpWritePrmType4Datapacket, pData, sizeof(BleCgmsSocpWritePrmType4Datapacket));

				g_PrmDb.P4.prmWMY[0] = BleCgmsSocpWritePrmType4Datapacket.ucPrmWMY[0];
				g_PrmDb.P4.prmWMY[1] = BleCgmsSocpWritePrmType4Datapacket.ucPrmWMY[1];
				g_PrmDb.P4.prmWMY[2] = BleCgmsSocpWritePrmType4Datapacket.ucPrmWMY[2];
				g_PrmDb.P4.prmWMY[3] = 0;
				g_PrmDb.P4.SN = BleCgmsSocpWritePrmType4Datapacket.usSN;

				g_PrmDb.P4.prmCrc4 = do_crc((uint8_t*)&g_PrmDb.P4, sizeof(P4_t) - 2);

				SocpResponseDatapcket.ucRspCode = SOCP_GENERAL_RSP_SUCCESS;
				break;
			}
			default:
			{
				SocpResponseDatapcket.ucRspCode = SOCP_GENERAL_RSP_INVALID_OPERAND;
				break;
			}
			}

		}
		break;
	}
	// 如果是读取参数命令
	case SOCP_READ_PRM:
	{
		log_d("SOCP_READ_PRM");
		if (SocpDatapacket.ucDataLen > 4)
		{
			SocpResponseDatapcket.ucRspCode = SOCP_GENERAL_RSP_INVALID_OPERAND;
		}
		else
		{
			uint8_t ucPrmNo = 0;
			ucPrmNo = SocpDatapacket.pData[0];
			switch (ucPrmNo)
			{
			case BLE_CGMS_SOCP_WRITE_PRM_TYPE_1:
			{
				ble_cgms_socp_write_prm_type_1_t BleCgmsSocpWritePrmType1Datapacket;

				BleCgmsSocpWritePrmType1Datapacket.ucPrmNo = ucPrmNo;
				BleCgmsSocpWritePrmType1Datapacket.sPrmVD1 = g_PrmDb.Pone.prmVD1;
				BleCgmsSocpWritePrmType1Datapacket.sPrmRL1 = g_PrmDb.Pone.prmRL1;
				BleCgmsSocpWritePrmType1Datapacket.sPrmOffset = g_PrmDb.Pone.prmOffset;
				BleCgmsSocpWritePrmType1Datapacket.sPrmAD1 = g_PrmDb.Pone.prmAD1;
				BleCgmsSocpWritePrmType1Datapacket.sPrmRL2 = g_PrmDb.Pone.prmRL2;
				BleCgmsSocpWritePrmType1Datapacket.sPrmAD2 = g_PrmDb.Pone.prmAD2;

				// 结构体内容填充到回应包数据buffer中
				memcpy(SocpResponseDatapcket.ucRespVal, &BleCgmsSocpWritePrmType1Datapacket, sizeof(BleCgmsSocpWritePrmType1Datapacket));
				SocpResponseDatapcket.ucOpCode = SOCP_READ_PRM_RESPONSE;
				break;
			}

			case BLE_CGMS_SOCP_WRITE_PRM_TYPE_3:
			{
				ble_cgms_socp_write_prm_type_3_t BleCgmsSocpWritePrmType3Datapacket;

				BleCgmsSocpWritePrmType3Datapacket.ucPrmNo = ucPrmNo;

				BleCgmsSocpWritePrmType3Datapacket.ucPrmDX0 = g_PrmDb.P3.prmDX0;
				BleCgmsSocpWritePrmType3Datapacket.ucPrmDX1 = g_PrmDb.P3.prmDX1;
				BleCgmsSocpWritePrmType3Datapacket.ucPrmDX2 = g_PrmDb.P3.prmDX2;
				BleCgmsSocpWritePrmType3Datapacket.ucPrmDX3 = g_PrmDb.P3.prmDX3;
				BleCgmsSocpWritePrmType3Datapacket.ucPrmDX4 = g_PrmDb.P3.prmDX4;
				BleCgmsSocpWritePrmType3Datapacket.ucPrmDX5 = g_PrmDb.P3.prmDX5;
				BleCgmsSocpWritePrmType3Datapacket.ucPrmDX6 = g_PrmDb.P3.prmDX6;
				BleCgmsSocpWritePrmType3Datapacket.ucPrmDX7 = g_PrmDb.P3.prmDX7;
				BleCgmsSocpWritePrmType3Datapacket.ucPrmDX8 = g_PrmDb.P3.prmDX8;
				BleCgmsSocpWritePrmType3Datapacket.ucPrmDX9 = g_PrmDb.P3.prmDX9;
				BleCgmsSocpWritePrmType3Datapacket.ucPrmDXA = g_PrmDb.P3.prmDXA;
				BleCgmsSocpWritePrmType3Datapacket.ucPrmDXB = g_PrmDb.P3.prmDXB;
				BleCgmsSocpWritePrmType3Datapacket.usPrmDXC = g_PrmDb.P3.prmDXC;

				// 结构体内容填充到回应包数据buffer中
				memcpy(SocpResponseDatapcket.ucRespVal, &BleCgmsSocpWritePrmType3Datapacket, sizeof(BleCgmsSocpWritePrmType3Datapacket));
				SocpResponseDatapcket.ucOpCode = SOCP_READ_PRM_RESPONSE;
				break;
			}
			default:
			{
				SocpResponseDatapcket.ucRspCode = SOCP_GENERAL_RSP_INVALID_OPERAND;
				break;
			}
			}
		}
		break;
	}

	// 如果是设置密码命令
	case SOCP_SET_PASSWORD:
	{
		log_d("SOCP_SET_PASSWORD");
		// 如果当前已经被设置过密码
		if (att_get_feature()->ucPasswordExist)
		{
			SocpResponseDatapcket.ucOpCode = SOCP_RESPONSE_CODE;
			SocpResponseDatapcket.ucReqOpcode = SocpDatapacket.ucOpCode;
			SocpResponseDatapcket.ucRspCode = SOCP_SET_PASSWORD_RSP_CODE_HAVE_A_PASSWORD;
			SocpResponseDatapcket.ucSizeVal = 0;
		}
		else
		{
			// 设置过密码标识位置位
			att_get_feature()->ucPasswordExist = 0x01;

			// 更新CRC
			att_update_feature_char_data_crc();

			// 填充结构体
			ble_cgms_socp_set_password_datapacket_t SocpSetPasswordDatapacket;
			memcpy(&SocpSetPasswordDatapacket, pData, sizeof(SocpSetPasswordDatapacket));

            SocpResponseDatapcket.ucOpCode = SOCP_RESPONSE_CODE;
            SocpResponseDatapcket.ucReqOpcode = SocpDatapacket.ucOpCode;
            SocpResponseDatapcket.ucRspCode = SOCP_VERIFY_PASSWORD_RSP_CODE_SUCCESS;
            SocpResponseDatapcket.ucSizeVal = 0;

			// 发送回复包
            socp_send(BleEventInfo, SocpResponseDatapcket);

			// 更新秘钥
			cgms_aes128_update_key((uint8_t*)&(SocpSetPasswordDatapacket.usPassword));

			// 保存密码
			app_global_get_app_state()->usPasswordSaved = SocpSetPasswordDatapacket.usPassword;

			// 直接返回,不发函数末尾的回复包
			return;
		}
		break;
	}

	// 如果是验证密码命令
	case SOCP_VERIFY_PWD:
	{
		log_d("SOCP_VERIFY_PWD");
		SocpResponseDatapcket.ucSizeVal = 0;

		// 验证CRC
		if (do_crc(pData, 6) != 0)
		{
			SocpResponseDatapcket.ucRspCode = SOCP_VERIFY_PASSWORD_RSP_CODE_OTHER_ERR;

		}
		else
		{

			// 填充结构体
			ble_cgms_socp_verify_password_datapacket_t SocpVerifyPasswordDatapacket;
			memcpy(&SocpVerifyPasswordDatapacket, pData, sizeof(SocpVerifyPasswordDatapacket));

			// 判断密码是否正确
			if (app_global_get_app_state()->usPasswordSaved != SocpVerifyPasswordDatapacket.usPassword)
			{
				SocpResponseDatapcket.ucRspCode = SOCP_VERIFY_PASSWORD_RSP_CODE_PASSWORD_ERR;
			}
			else
			{
				// 判断数据包长度是否正确
				if (usLen != 16)
				{
					SocpResponseDatapcket.ucRspCode = SOCP_VERIFY_PASSWORD_RSP_CODE_COMMAND_LEN_ERR;
				}
				else
				{
					SocpResponseDatapcket.ucRspCode = SOCP_VERIFY_PASSWORD_RSP_CODE_SUCCESS;
					app_global_get_app_state()->bCgmsPwdVerifyOk = true;
				}
			}
		}
		break;
	}
	default:
	{
		SocpResponseDatapcket.ucRspCode = SOCP_GENERAL_RSP_OP_CODE_NOT_SUPPORTED;
		break;
	}
    }
	socp_send(BleEventInfo, SocpResponseDatapcket);
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

    //sec bond command
    if ((pData[0] == SOCP_WRITE_BOND_SECU) && (pData[1] == 0xf1) && (usLen == 18))return true;//Bond_step1
    if ((pData[0] == SOCP_WRITE_BOND_SECU) && (pData[1] == 0xf2) && (usLen == 18))return true;//Bond_step2
    if ((pData[0] == SOCP_NOMAL_AUTH) && (pData[1] == 0xf1) && (usLen == 18))return true;//sec normal step1
    if ((pData[0] == SOCP_NOMAL_AUTH) && (pData[1] == 0xf2) && (usLen == 18))return true;//sec normal step2

    if ((pData[0] == SOCP_START_THE_SESSION) && (pData[1] == 0) && (usLen == 4))return true;// start senssor
    if ((pData[0] == SOCP_STOP_THE_SESSION) && (pData[1] == 0) && (usLen == 4))return true;// STOP senssor
    if ((pData[0] == SOCP_START_AD_CALI) && (pData[1] == 0) && (usLen == 4))return true;
    return false;
}


/******************* (C) COPYRIGHT 2023 陈苏阳 **** END OF FILE ****************/
