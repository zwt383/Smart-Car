#include "app/car.h"

#include "bsp/motor.h"

static int16_t Car_LimitSpeed(uint16_t speed);

void Car_Init(void)
{
    Motor_Init();
    Car_Stop();
}

void Car_Forward(uint16_t speed)
{
    int16_t limitedSpeed = Car_LimitSpeed(speed);

    Motor_SetSpeed(MOTOR_LEFT, limitedSpeed);
    Motor_SetSpeed(MOTOR_RIGHT, limitedSpeed);
}

void Car_Backward(uint16_t speed)
{
    int16_t limitedSpeed = Car_LimitSpeed(speed);

    Motor_SetSpeed(MOTOR_LEFT, -limitedSpeed);
    Motor_SetSpeed(MOTOR_RIGHT, -limitedSpeed);
}

void Car_TurnLeft(uint16_t speed)
{
    int16_t limitedSpeed = Car_LimitSpeed(speed);

    Motor_SetSpeed(MOTOR_LEFT, -limitedSpeed);
    Motor_SetSpeed(MOTOR_RIGHT, limitedSpeed);
}

void Car_TurnRight(uint16_t speed)
{
    int16_t limitedSpeed = Car_LimitSpeed(speed);

    Motor_SetSpeed(MOTOR_LEFT, limitedSpeed);
    Motor_SetSpeed(MOTOR_RIGHT, -limitedSpeed);
}

void Car_Stop(void)
{
    Motor_StopAll();
}

static int16_t Car_LimitSpeed(uint16_t speed)
{
    if (speed > MOTOR_SPEED_MAX) {
        speed = MOTOR_SPEED_MAX;
    }

    return (int16_t)speed;
}
