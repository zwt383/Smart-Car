#include "app/chassis.h"

#include "bsp/motor.h"
#include "bsp/pid.h"
#include "bsp/speed_measure.h"

#define CHASSIS_PID_KP               (12)
#define CHASSIS_PID_KI               (2)
#define CHASSIS_PID_SCALE            (1)
#define CHASSIS_PID_INTEGRAL_LIMIT   (120)
#define CHASSIS_FEEDFORWARD_MIN      (60)
#define CHASSIS_FEEDFORWARD_GAIN     (4)
#define CHASSIS_FEEDFORWARD_FULL_TARGET  (CHASSIS_SPEED_TARGET_HIGH)
#define CHASSIS_OUTPUT_LIMIT         (350)
#define CHASSIS_TARGET_RAMP_STEP     (2)

volatile int32_t gChassisLeftTarget = 0;
volatile int32_t gChassisRightTarget = 0;
volatile int32_t gChassisLeftFeedback = 0;
volatile int32_t gChassisRightFeedback = 0;
volatile int32_t gChassisLeftOutput = 0;
volatile int32_t gChassisRightOutput = 0;

static int32_t gChassisLeftCommandTarget = 0;
static int32_t gChassisRightCommandTarget = 0;
static PidController_t gChassisLeftPid;
static PidController_t gChassisRightPid;

static int16_t Chassis_GetSpeedTarget(ChassisSpeedLevel_t speedLevel);
static void Chassis_UpdateTargetRamp(void);
static int32_t Chassis_RampTarget(int32_t current, int32_t target);
static int32_t Chassis_GetFeedforward(int32_t target);
static int32_t Chassis_LimitOutput(int32_t output);

void Chassis_Init(void)
{
    Pid_Init(&gChassisLeftPid, CHASSIS_PID_KP, CHASSIS_PID_KI,
        CHASSIS_PID_SCALE, -CHASSIS_OUTPUT_LIMIT, CHASSIS_OUTPUT_LIMIT,
        -CHASSIS_PID_INTEGRAL_LIMIT, CHASSIS_PID_INTEGRAL_LIMIT);
    Pid_Init(&gChassisRightPid, CHASSIS_PID_KP, CHASSIS_PID_KI,
        CHASSIS_PID_SCALE, -CHASSIS_OUTPUT_LIMIT, CHASSIS_OUTPUT_LIMIT,
        -CHASSIS_PID_INTEGRAL_LIMIT, CHASSIS_PID_INTEGRAL_LIMIT);
    Chassis_Stop();
}

void Chassis_SetWheelSpeedTargets(int16_t leftTarget, int16_t rightTarget)
{
    if ((gChassisLeftCommandTarget != leftTarget) ||
        (gChassisRightCommandTarget != rightTarget)) {
        Pid_Reset(&gChassisLeftPid);
        Pid_Reset(&gChassisRightPid);
    }

    gChassisLeftCommandTarget = leftTarget;
    gChassisRightCommandTarget = rightTarget;
}

void Chassis_SetMotion(ChassisMotion_t motion, ChassisSpeedLevel_t speedLevel)
{
    int16_t target = Chassis_GetSpeedTarget(speedLevel);

    switch (motion) {
        case CHASSIS_MOTION_FORWARD:
            Chassis_SetWheelSpeedTargets(target, target);
            break;
        case CHASSIS_MOTION_BACKWARD:
            Chassis_SetWheelSpeedTargets(-target, -target);
            break;
        case CHASSIS_MOTION_TURN_LEFT:
            Chassis_SetWheelSpeedTargets(-target, target);
            break;
        case CHASSIS_MOTION_TURN_RIGHT:
            Chassis_SetWheelSpeedTargets(target, -target);
            break;
        case CHASSIS_MOTION_STOP:
        default:
            Chassis_Stop();
            break;
    }
}

void Chassis_Stop(void)
{
    gChassisLeftCommandTarget = 0;
    gChassisRightCommandTarget = 0;
    gChassisLeftTarget = 0;
    gChassisRightTarget = 0;
    Pid_Reset(&gChassisLeftPid);
    Pid_Reset(&gChassisRightPid);
    gChassisLeftOutput = 0;
    gChassisRightOutput = 0;
    Motor_StopAll();
}

void Chassis_Update(void)
{
    int32_t leftOutput;
    int32_t rightOutput;

    gChassisLeftFeedback = gSpeedLeftCount;
    gChassisRightFeedback = gSpeedRightCount;

    Chassis_UpdateTargetRamp();

    if ((gChassisLeftTarget == 0) && (gChassisRightTarget == 0)) {
        Chassis_Stop();
        return;
    }

    leftOutput = Chassis_GetFeedforward(gChassisLeftTarget) +
        Pid_Update(&gChassisLeftPid, gChassisLeftTarget, gChassisLeftFeedback);
    rightOutput = Chassis_GetFeedforward(gChassisRightTarget) +
        Pid_Update(&gChassisRightPid, gChassisRightTarget, gChassisRightFeedback);

    leftOutput = Chassis_LimitOutput(leftOutput);
    rightOutput = Chassis_LimitOutput(rightOutput);

    gChassisLeftOutput = leftOutput;
    gChassisRightOutput = rightOutput;

    Motor_SetSpeed(MOTOR_LEFT, (int16_t)leftOutput);
    Motor_SetSpeed(MOTOR_RIGHT, (int16_t)rightOutput);
}

static int16_t Chassis_GetSpeedTarget(ChassisSpeedLevel_t speedLevel)
{
    int16_t target;

    switch (speedLevel) {
        case CHASSIS_SPEED_HIGH:
            target = CHASSIS_SPEED_TARGET_HIGH;
            break;
        case CHASSIS_SPEED_MEDIUM:
            target = CHASSIS_SPEED_TARGET_MEDIUM;
            break;
        case CHASSIS_SPEED_LOW:
        default:
            target = CHASSIS_SPEED_TARGET_LOW;
            break;
    }

    return target;
}

static void Chassis_UpdateTargetRamp(void)
{
    gChassisLeftTarget = Chassis_RampTarget(gChassisLeftTarget,
        gChassisLeftCommandTarget);
    gChassisRightTarget = Chassis_RampTarget(gChassisRightTarget,
        gChassisRightCommandTarget);
}

static int32_t Chassis_RampTarget(int32_t current, int32_t target)
{
    if (current < target) {
        current += CHASSIS_TARGET_RAMP_STEP;
        if (current > target) {
            current = target;
        }
    } else if (current > target) {
        current -= CHASSIS_TARGET_RAMP_STEP;
        if (current < target) {
            current = target;
        }
    }

    return current;
}

static int32_t Chassis_GetFeedforward(int32_t target)
{
    int32_t absTarget;
    int32_t feedforward;

    if (target == 0) {
        return 0;
    }

    absTarget = target;
    if (absTarget < 0) {
        absTarget = -absTarget;
    }

    feedforward = CHASSIS_FEEDFORWARD_MIN +
        (CHASSIS_FEEDFORWARD_GAIN * absTarget);
    feedforward = ((feedforward * absTarget) +
        (CHASSIS_FEEDFORWARD_FULL_TARGET / 2)) /
        CHASSIS_FEEDFORWARD_FULL_TARGET;

    if (target < 0) {
        feedforward = -feedforward;
    }

    return feedforward;
}

static int32_t Chassis_LimitOutput(int32_t output)
{
    if (output > CHASSIS_OUTPUT_LIMIT) {
        output = CHASSIS_OUTPUT_LIMIT;
    } else if (output < -CHASSIS_OUTPUT_LIMIT) {
        output = -CHASSIS_OUTPUT_LIMIT;
    }

    return output;
}
