#ifndef BSP_MOTOR_H_
#define BSP_MOTOR_H_

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define MOTOR_SPEED_MAX    (1000)

typedef enum
{
    MOTOR_LEFT = 0,
    MOTOR_RIGHT
} Motor_t;

void Motor_Init(void);
void Motor_SetSpeed(Motor_t motor, int16_t speed);
void Motor_Stop(Motor_t motor);
void Motor_StopAll(void);

#ifdef __cplusplus
}
#endif

#endif /* BSP_MOTOR_H_ */
