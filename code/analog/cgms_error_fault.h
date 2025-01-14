/*
 * cgms_error_fault.h
 *
 *  Created on: 2023��4��17��
 *      Author: SZZ0030
 */

/*
 *
 *
 *
 */
/**************define cgms sensor abnormal***************/

#include <stdint.h>

#define CGMS_STATUS_ABNORMAL_NONE				        0X00
#define CGMS_STATUS_ABNORMAL_NEED_CAL   		    0X10	////reserved
#define CGMS_STATUS_ABNORMAL_HIGH_CUR   		    0x20   ////reserved
#define CGMS_STATUS_ABNORMAL_LOW_CUR			      0x21   ////reserved
#define CGMS_STATUS_ABNORMAL_SENSOR_ABNOMAL     0x22
#define CGMS_STATUS_ABNORMAL_SENSOR_FAUL 		    0x23

#define CGMS_STATUS_FAULT_HI                    0X24
#define CGMS_STATUS_FAULT_LOW                   0X25
#define CGMS_STATUS_FAULT_CV                    0X26

#define CGMS_STATUS_ABNORMAL_INVALID_IMPLANTED 	0x31
#define CGMS_STATUS_ABNOARNAL_WARMUP			      0X33
#define CGMS_STATUS_FAIL_WARMUP					        0x34


#define CGMS_STATUS_HI_CUR                      0X41
#define CGMS_STATUS_LOW_CUR                     0X42
#define CGMS_STATUS_CV                          0x43



#define CGMS_STATUS_ABNORMAL_HI_GLUCOSE         0X41   //reserved
#define CGMS_STATUS_ABNORMAL_LOW_GLUCOSE       	0x42   //reserved


/*******************cgms glucose trend definition**********************************/

#define CGMS_TREND_INVILD						0x00
#define CGMS_TREND_FAST_DOWN					0x01
#define CGMS_TREND_DOWN_DOWN					0x02
#define CGMS_TREND_SLOW_DOWN					0x03
#define CGMS_TREND_STABLE						0x04
#define CGMS_TREND_SLOW_UP						0x05
#define CGMS_TREND_UP_UP						0x06
#define CGMS_TREND_FAST_UP						0x07

#define CUR_NORMAL_LOW_LMT						0.1f    //nA
#define CUR_NORMAL_HI_LMT						  50.0f	//nA

#define GLUCOSE_MIN_LMT							  0.1f
#define GLUCOSE_MAX_LMT							  30.0f
//
#define CGMS_TREND_STABLE_LOW_LMT				0.00f
#define CMGS_TREND_STABLE_HI_LMT				0.06f   // mmol/L/minute

#define CGMS_TREND_SLOW_LOW_LMT					0.06f
#define CMGS_TREND_SLOW_HI_LMT					0.11f

#define CGMS_TREND_DOWW_UU_LOW_LMT				0.11f
#define CGMS_TREND_DOWW_UU_HI_LMT				0.17f

#define CGMS_TREND_FAST_LOW_LMT					0.17f
#define CGMS_TREND_FAST_HI_LMT					35.00f


#define CGMS_ABNORMAL_RECOVERY_COUNT_MAX        10
#define WARM_UP_CUR_ERROR_MAX					10


#define CUR_BUFF_MAX     10
#define GLU_BUFF_MAX     2

#define CUR_LOW_COUT_MAX 5
#define CUR_LOW_COUT_LMT 3
#define CUR_HI_COUT_LMT  1


#define CUR_LOW_COUT_MAX 			5
#define SENSOR_PLANTED_CHECK_CNT   CUR_LOW_COUT_MAX


typedef struct
{
			float low_lmt;
			float hi_lmt;
			uint8_t down_trend_val;
			uint8_t up_trend_val;

}CGMS_TREND_STRUCT_LMT;



//uint8_t cgms_error_fault_cal(uint16_t cgms_offset,float glucose,
//						 float cur ,uint8_t *status);
void cgms_error_fault_cal(uint16_t cgms_offset,float glucose,float cur ,uint8_t *status,uint8_t cv);
uint8_t cgms_cal_trend(float glucose, uint16_t cgms_offset);
