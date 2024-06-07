
#ifdef __cplusplus
extern "C"
{
#endif

#include<math.h>

#define MAX_POINTS 15
#define DEGREE 2 // 多项式的阶数

typedef struct {
    double x;
    double y;
} Point;


float polyfit(float *x, float *y, int n, int degree, float *coefficients, float *fit) ;


#ifdef __cplusplus
}
#endif