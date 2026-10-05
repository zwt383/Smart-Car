#include "bsp/pid.h"

static int32_t Pid_Limit(int32_t value, int32_t minValue, int32_t maxValue);

void Pid_Init(PidController_t *pid, int32_t kp, int32_t ki, int32_t scale,
    int32_t outputMin, int32_t outputMax, int32_t integralMin,
    int32_t integralMax)
{
    pid->kp = kp;
    pid->ki = ki;
    pid->scale = scale;
    pid->integral = 0;
    pid->integralMin = integralMin;
    pid->integralMax = integralMax;
    pid->outputMin = outputMin;
    pid->outputMax = outputMax;
}

void Pid_Reset(PidController_t *pid)
{
    pid->integral = 0;
}

int32_t Pid_Update(PidController_t *pid, int32_t target, int32_t feedback)
{
    int32_t error = target - feedback;
    int32_t output;

    pid->integral += error;
    pid->integral = Pid_Limit(pid->integral,
        pid->integralMin, pid->integralMax);

    output = ((pid->kp * error) + (pid->ki * pid->integral)) / pid->scale;
    return Pid_Limit(output, pid->outputMin, pid->outputMax);
}

static int32_t Pid_Limit(int32_t value, int32_t minValue, int32_t maxValue)
{
    if (value > maxValue) {
        value = maxValue;
    } else if (value < minValue) {
        value = minValue;
    }

    return value;
}
