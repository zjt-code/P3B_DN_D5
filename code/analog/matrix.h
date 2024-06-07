#ifndef __MATRIX_H__
#define __MATRIX_H__
#include<stdint.h>
#include<stdbool.h>

#ifdef __cplusplus

    extern "C" 
    {
#endif


typedef struct tag_matrix
{
  uint16_t x;
  uint16_t y;
  float *parray;

} t_matrix;


void Convert2_UnitMatix(t_matrix *mat);
void Matix_Print(t_matrix *mat);

void Transpose_Matrix( t_matrix *mat,t_matrix *mat_T);


float GetElement_Matrix(t_matrix *mat,uint16_t x, uint16_t y);
void  SetElement_Matrix(t_matrix *mat,uint16_t x, uint16_t y, float value);
void  MulConst_Matrix(t_matrix *mat , float val);
bool  MulMatrix_Matrix(t_matrix *mat_a,t_matrix *mat_b, t_matrix *mat_val);


bool Matrix_Add_Matrix(t_matrix *mat_a, t_matrix *mat_b);
bool Matrix_Minus_Matrix(t_matrix *mat_a, t_matrix *mat_b);


#ifdef __cplusplus

    }
#endif

#endif