/**************************************************************************************************
  Filename:       simpleGluco.h
  Revised:        $Date: 2016-08-01 $
  Revision:       $Revision: 23256 	$

  Description:    This file contains the SimpleGluco sample application definitions and prototypes.

  Copyright: 			Bioshall. All rights reserved.

  IMPORTANT: 
**************************************************************************************************/

#ifndef SIMPLEBLEGLUCO_H
#define SIMPLEBLEGLUCO_H

#ifdef __cplusplus
extern "C"
{
#endif

/*********************************************************************
 * INCLUDES
 */
#include <math.h>
//#include "logRTT.h"//ADB.WOO


/*********************************************************************
 * CONSTANTS
 */

/*********************************************************************
 * MACROS
 */

#ifndef DX_PET
    #define DX_PET
#else
    #define D3_HUM
#endif


#define     BRD_N52832_SDK1702   1
#define     BRD_RSL10_SDK34154   2
#define     BRD_N52832_SDK1530   3
#define     BOARD                BRD_RSL10_SDK34154

#define     SENSOR_GN1           1
#define     SENSOR_GN2           2
#define     SENSOR_P3            3
#define     SENSOR_I3            4//same with sensor_GN1
#define     SENSOR_TYPE          SENSOR_P3

#if( defined(BOARD) && ((BOARD == BRD_N52832_SDK1702)||(BOARD == BRD_N52832_SDK1530)) )
  #define Sys_Watchdog_Refresh() // to compate with GN-2
#endif
#if( defined(SENSOR_TYPE) && (SENSOR_TYPE == SENSOR_GN1) )
  #define D_X0         3
  #define D_X1         0
  #define D_X2         32
  #define D_X3         1
  #define D_X4         0.5
  #define D_X5         0
  #define D_X6         40
  #define D_X7         60
  #define D_X8         0.002//Not use
  #define D_X9         0.8
  #define D_XA         0    //Not use
  #define D_XB         0    //Not use
  #define D_XC         3.500//Not use

  #define D_P500       0.1
  #define D_P501       0.1
#elif( defined(SENSOR_TYPE) && (SENSOR_TYPE == SENSOR_P3) )
  #define D_X0         3    //Not use
  #define D_X1         0
  #define D_X2         32
  #define D_X3         1
  //#define D_X4         0.9       //smoothCoeff t2e_01,t2e_02,t2e_03
  //#define D_X4         0.5       //smoothCoeff t2e_04
  // #define D_X4         0.7      //smoothCoeff add rls_filter ,lag ,
  //#define D_X5         0.3 //1.0.1t2e_01)
  #define D_X4         0.8      // 2025.01.02 v16

  #define D_X5         0//1.0.1t2e_02)
  #define D_X6         20   //Not use
  #define D_X7         19
  #define D_X8         0.002//Not use
  #define D_X9         0.8
  #define D_XA         0    //Not use
  #define D_XB         0    //Not use
  #define D_XC         3.500//Not use

  #define D_P500       0.1
  #define D_P501       0.1

  #define ORIGNAL_DATA_LEN 15


#define RSL_ORDER      6
#define X_ROW_MAX      3
#define Y_COL_MAX      2

#define ROW_P  6
#define COL_P  6
#define ROW_X  6
#define COL_X  1
#define GLUCOSE_DATA_LEN 15

#define RLS_FILTER_DATA_LEN 10

#define lms  0.3

#define GLUCOSE_DATA_LEN 15

#define RLS_FILTER_DATA_LEN 10

#define lms  0.3

#define CV_IO_BUFF_MAX            4
#define CV_THRESOLD               0.20f
#define DIFF_THRESOLD             2.0f



#define GVP_SAMPLE_MAX          	10
#define GVP_SAMPLE_TIME_INTEVAL 	3.0f  // minutes
#define GVP_ERROR_MAX           	1.0f

#define SENSOR_RATIO_MAX         	1f
//#define SENSOR_RATIO_MIN         	0.6f
#define SENSOR_RATIO_MIN         	0.5f


#elif( defined(SENSOR_TYPE) && (SENSOR_TYPE == SENSOR_GN2) )
  #define D_X0         3    //Not use
  #define D_X1         0    //--tempCorrect
  #define D_X2         32   //--temperRef
  #define D_X3         1    //--currdata limit
  #define D_X4         0.5  //--smooth coef
  #define D_X5         0    //--curr background
  #define D_X6         40   //Not use
  #define D_X7         40   //--warmup number
  #define D_X8         0.002//Not use
  #define D_X9         0.8  //--SKfilt
  #define D_XA         0    //Not use
  #define D_XB         0    //Not use
  #define D_XC         3.500//Not use

#endif




#define MOVE_FIL_WIN 4
#define EXPF(x) expf(x)

#if( defined(BOARD) && (BOARD == BRD_N52832_SDK1702) )
  #define app_env_isfs CodeRecvFlag
  extern unsigned char CodeRecvFlag;
#elif( defined(BOARD) && (BOARD == BRD_RSL10_SDK34154) )
  #define app_env_isfs app_env.isfs
#elif( defined(BOARD) && (BOARD == BRD_N52832_SDK1530) )
  #define app_env_isfs CodeRecvFlag
#endif

#if( defined(BOARD) && (BOARD == BRD_N52832_SDK1530) )
  #define Cgms_GlucoseSimpleInit() simpleGlucoInit()
  #define Cgms_GlucoseSimpleClai(x) simpleGlucoCalc(x)
#endif
/*********************************************************************
 * GLOBAL VARIABLES
 */
struct corrK
{
	  float corr1;
	  float corr2;
	  float corr3;
	  float corr4;
	  float corr5;
};
extern struct corrK stCorrK;
/* ----------------------------------------------------------------------------
 * Global variables and types
 * ------------------------------------------------------------------------- */
extern signed char vD_X0;
extern float vD_X1;
extern unsigned char vD_X2;
extern float vD_X3;
extern float vD_X4;
extern float vD_X5;
extern unsigned char vD_X6;
extern unsigned char vD_X7;
extern float vD_X8;
extern float vD_X9;
extern float vD_XA;
extern unsigned char vD_XB;
extern float vD_XC;

extern float vP500;
extern float vP501;
extern float vP502;
extern float vP503;
extern float vP504;
extern float vP505;
extern float vP506;
extern float vP507;
extern float vP508;
extern float vP509;
extern float vP510;
extern float vP511;
extern float vP512;
extern float vP513;
extern float vP514;

extern float fD_temperReal;
extern float sfCurrI0Max;         //��λ��nA����������Ч��Χ
extern float sfCurrI0Min;

extern unsigned short simpleGlucoCalcCnt; //reset whenever sensor started
extern unsigned short usSampleCnt;  //����������n��
extern unsigned short usBgCnt;      //�α�Ѫ�ǵ�n��
extern unsigned short usBfFlg;
extern unsigned short usCurrDbg; // simpleGluco_B.h�� extern����һ��

extern float sfCurrI0; //������������λ��nA
extern float sfCurrI5; //������������λ��nA
extern float sfCurrI3;

extern float sfCurrSum;
extern float sfCurrBg; //�α�Ѫ�ǣ���λ��mmol/L
extern float sfCurrK;
extern float sensorK;  //sensor sensitivity in the unit of nA/mmol/L

/*********************************************************************
 * FUNCTIONS
 */

void simpleGlucoInit(void);//run when start sensor
void simpleGlucoCalc(float* pResult, unsigned short uSample);
void calcGlucoInit_D03(void);
void calcGluco_D03(float* pGluco,unsigned short tCnt);
void calcGluco_StateBackup(void);
void calcGluco_StateInit(void);
void calcGluco_StateRecovery(void);
/*********************************************************************
*********************************************************************/

#ifdef __cplusplus
}
#endif

#endif /* SIMPLEBLEGLUCO_H */
