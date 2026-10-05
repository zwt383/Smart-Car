#ifndef BSP_PID_H_
#define BSP_PID_H_

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct
{
    int32_t kp;
    int32_t ki;
    int32_t scale;
    int32_t integral;
    int32_t integralMin;
    int32_t integralMax;
    int32_t outputMin;
    int32_t outputMax;
} PidController_t;

void Pid_Init(PidController_t *pid, int32_t kp, int32_t ki, int32_t scale,
    int32_t outputMin, int32_t outputMax, int32_t integralMin,
    int32_t integralMax);
void Pid_Reset(PidController_t *pid);
int32_t Pid_Update(PidController_t *pid, int32_t target, int32_t feedback);

#ifdef __cplusplus
}
#endif

#endif /* BSP_PID_H_ */
