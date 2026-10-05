#include "bsp/motor.h"

#include <stdbool.h>

#include "ti_msp_dl_config.h"

#define MOTOR_PWM_PERIOD_COUNTS    (1600U)
#define MOTOR_RIGHT_INVERTED       (1)

static uint16_t Motor_AbsSpeed(int16_t speed);
static uint16_t Motor_SpeedToCompareValue(uint16_t speed);
static void Motor_SetDirection(Motor_t motor, bool forward);
static void Motor_SetPwm(Motor_t motor, uint16_t speed);

void Motor_Init(void)
{
    Motor_StopAll();
    DL_GPIO_setPins(GPIO_GRP_MOTOR_PORT, GPIO_GRP_MOTOR_STBY_PIN);
}

void Motor_SetSpeed(Motor_t motor, int16_t speed)
{
    if ((motor != MOTOR_LEFT) && (motor != MOTOR_RIGHT)) {
        return;
    }

#if MOTOR_RIGHT_INVERTED
    if (motor == MOTOR_RIGHT) {
        speed = (int16_t)-speed;
    }
#endif

    if (speed == 0) {
        Motor_Stop(motor);
        return;
    }

    Motor_SetDirection(motor, speed > 0);
    Motor_SetPwm(motor, Motor_AbsSpeed(speed));
}

void Motor_Stop(Motor_t motor)
{
    if (motor == MOTOR_LEFT) {
        DL_TimerA_setCaptureCompareValue(
            PWM_MOTOR_INST, 0U, GPIO_PWM_MOTOR_C0_IDX);
        DL_GPIO_clearPins(GPIO_GRP_MOTOR_PORT,
            GPIO_GRP_MOTOR_AIN1_PIN | GPIO_GRP_MOTOR_AIN2_PIN);
    } else if (motor == MOTOR_RIGHT) {
        DL_TimerA_setCaptureCompareValue(
            PWM_MOTOR_INST, 0U, GPIO_PWM_MOTOR_C1_IDX);
        DL_GPIO_clearPins(GPIO_GRP_MOTOR_PORT,
            GPIO_GRP_MOTOR_BIN1_PIN | GPIO_GRP_MOTOR_BIN2_PIN);
    }
}

void Motor_StopAll(void)
{
    DL_TimerA_setCaptureCompareValue(PWM_MOTOR_INST, 0U, GPIO_PWM_MOTOR_C0_IDX);
    DL_TimerA_setCaptureCompareValue(PWM_MOTOR_INST, 0U, GPIO_PWM_MOTOR_C1_IDX);
    DL_GPIO_clearPins(GPIO_GRP_MOTOR_PORT,
        GPIO_GRP_MOTOR_AIN1_PIN | GPIO_GRP_MOTOR_AIN2_PIN |
        GPIO_GRP_MOTOR_BIN1_PIN | GPIO_GRP_MOTOR_BIN2_PIN);
}

static uint16_t Motor_AbsSpeed(int16_t speed)
{
    if (speed < 0) {
        speed = (int16_t)-speed;
    }

    if (speed > MOTOR_SPEED_MAX) {
        speed = MOTOR_SPEED_MAX;
    }

    return (uint16_t)speed;
}

static uint16_t Motor_SpeedToCompareValue(uint16_t speed)
{
    return (uint16_t)(((uint32_t)speed * MOTOR_PWM_PERIOD_COUNTS) /
        MOTOR_SPEED_MAX);
}

static void Motor_SetDirection(Motor_t motor, bool forward)
{
    if (motor == MOTOR_LEFT) {
        if (forward) {
            DL_GPIO_setPins(GPIO_GRP_MOTOR_PORT, GPIO_GRP_MOTOR_AIN1_PIN);
            DL_GPIO_clearPins(GPIO_GRP_MOTOR_PORT, GPIO_GRP_MOTOR_AIN2_PIN);
        } else {
            DL_GPIO_clearPins(GPIO_GRP_MOTOR_PORT, GPIO_GRP_MOTOR_AIN1_PIN);
            DL_GPIO_setPins(GPIO_GRP_MOTOR_PORT, GPIO_GRP_MOTOR_AIN2_PIN);
        }
    } else {
        if (forward) {
            DL_GPIO_setPins(GPIO_GRP_MOTOR_PORT, GPIO_GRP_MOTOR_BIN1_PIN);
            DL_GPIO_clearPins(GPIO_GRP_MOTOR_PORT, GPIO_GRP_MOTOR_BIN2_PIN);
        } else {
            DL_GPIO_clearPins(GPIO_GRP_MOTOR_PORT, GPIO_GRP_MOTOR_BIN1_PIN);
            DL_GPIO_setPins(GPIO_GRP_MOTOR_PORT, GPIO_GRP_MOTOR_BIN2_PIN);
        }
    }
}

static void Motor_SetPwm(Motor_t motor, uint16_t speed)
{
    uint16_t compareValue = Motor_SpeedToCompareValue(speed);

    if (motor == MOTOR_LEFT) {
        DL_TimerA_setCaptureCompareValue(
            PWM_MOTOR_INST, compareValue, GPIO_PWM_MOTOR_C0_IDX);
    } else {
        DL_TimerA_setCaptureCompareValue(
            PWM_MOTOR_INST, compareValue, GPIO_PWM_MOTOR_C1_IDX);
    }
}
