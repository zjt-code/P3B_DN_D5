#include<stdint.h>

#ifdef __cplusplus
extern "C"
{
#endif    /* ifdef __cplusplus */


#define FILTER_VER01    1
#define FILTER_VER02    2

#define FILTER_VER      FILTER_VER02 

#define FILTER_CUR_DEBUG_LOG  0



typedef struct 
{
         
          float Sensor_K_Low_Lmt;
          float Sensor_K_Hi_Lmt;          
          float I_Error_Min_Value;
          float I_Error_Max_Value;

}t_Sensor_k_I_Data;

extern float cur_filter(float *cur_buff, uint32_t count_offset);
extern void cur_get_cur_error_value(float sensor_K);
float Cal_CV(float *v, uint32_t size_n);

extern uint8_t cgms_i_cv(float i, uint32_t offset);
float average_f(float *v, uint32_t size_n);
float average_minmax(float *v, uint32_t size_n);


extern float cur_error_min_value;
extern float cur_error_max_value;


#ifdef __cplusplus
}
#endif    /* ifdef __cplusplus */
