#include "app/car_debug.h"

#include "bsp/speed_measure.h"

#define CAR_DEBUG_MIN_MOVING_COUNT           (1)
#define CAR_DEBUG_IMBALANCE_THRESHOLD_PM     (400)
#define CAR_DEBUG_STARTUP_GRACE_CYCLES       (5)

volatile int32_t gCarMotionState = CAR_DEBUG_STATE_STOP;
volatile int32_t gCarSpeedLeft = 0;
volatile int32_t gCarSpeedRight = 0;
volatile int32_t gCarSpeedLeftAbs = 0;
volatile int32_t gCarSpeedRightAbs = 0;
volatile int32_t gCarSpeedDiff = 0;
volatile int32_t gCarSpeedRatioPermille = 0;
volatile int32_t gCarDebugGraceCycles = 0;
volatile uint32_t gCarDebugErrorFlags = CAR_DEBUG_ERROR_NONE;

static uint16_t gStartupGraceCycles = 0;

static int32_t CarDebug_Abs(int32_t value);
static int32_t CarDebug_GetExpectedLeftSign(CarDebugState_t state);
static int32_t CarDebug_GetExpectedRightSign(CarDebugState_t state);
static void CarDebug_CheckDirection(uint32_t *errorFlags, int32_t speed,
    int32_t expectedSign, uint32_t errorFlag);

void CarDebug_Reset(void)
{
    gCarMotionState = CAR_DEBUG_STATE_STOP;
    gCarSpeedLeft = 0;
    gCarSpeedRight = 0;
    gCarSpeedLeftAbs = 0;
    gCarSpeedRightAbs = 0;
    gCarSpeedDiff = 0;
    gCarSpeedRatioPermille = 0;
    gStartupGraceCycles = 0;
    gCarDebugGraceCycles = gStartupGraceCycles;
    gCarDebugErrorFlags = CAR_DEBUG_ERROR_NONE;
}

void CarDebug_SetState(CarDebugState_t state)
{
    gCarMotionState = state;
    if (state == CAR_DEBUG_STATE_STOP) {
        gStartupGraceCycles = 0;
    } else {
        gStartupGraceCycles = CAR_DEBUG_STARTUP_GRACE_CYCLES;
    }
    gCarDebugGraceCycles = gStartupGraceCycles;
    gCarDebugErrorFlags = CAR_DEBUG_ERROR_NONE;
}

void CarDebug_Update(void)
{
    CarDebugState_t state = (CarDebugState_t)gCarMotionState;
    int32_t leftSpeed = gSpeedLeftCount;
    int32_t rightSpeed = gSpeedRightCount;
    int32_t leftAbs = CarDebug_Abs(leftSpeed);
    int32_t rightAbs = CarDebug_Abs(rightSpeed);
    int32_t sumAbs = leftAbs + rightAbs;
    uint32_t errorFlags = CAR_DEBUG_ERROR_NONE;

    gCarSpeedLeft = leftSpeed;
    gCarSpeedRight = rightSpeed;
    gCarSpeedLeftAbs = leftAbs;
    gCarSpeedRightAbs = rightAbs;
    gCarSpeedDiff = leftAbs - rightAbs;

    if (rightAbs > 0) {
        gCarSpeedRatioPermille = (leftAbs * 1000) / rightAbs;
    } else {
        gCarSpeedRatioPermille = 0;
    }

    if (gStartupGraceCycles > 0U) {
        gStartupGraceCycles--;
        gCarDebugGraceCycles = gStartupGraceCycles;
        gCarDebugErrorFlags = CAR_DEBUG_ERROR_NONE;
        return;
    }
    gCarDebugGraceCycles = gStartupGraceCycles;

    if (state != CAR_DEBUG_STATE_STOP) {
        if (leftAbs <= CAR_DEBUG_MIN_MOVING_COUNT) {
            errorFlags |= CAR_DEBUG_ERROR_LEFT_NO_SPEED;
        }
        if (rightAbs <= CAR_DEBUG_MIN_MOVING_COUNT) {
            errorFlags |= CAR_DEBUG_ERROR_RIGHT_NO_SPEED;
        }

        CarDebug_CheckDirection(&errorFlags, leftSpeed,
            CarDebug_GetExpectedLeftSign(state), CAR_DEBUG_ERROR_LEFT_DIR);
        CarDebug_CheckDirection(&errorFlags, rightSpeed,
            CarDebug_GetExpectedRightSign(state), CAR_DEBUG_ERROR_RIGHT_DIR);

        if ((sumAbs > 0) &&
            ((CarDebug_Abs(leftAbs - rightAbs) * 1000) >
                (sumAbs * CAR_DEBUG_IMBALANCE_THRESHOLD_PM))) {
            errorFlags |= CAR_DEBUG_ERROR_SPEED_IMBALANCE;
        }
    }

    gCarDebugErrorFlags = errorFlags;
}

static int32_t CarDebug_Abs(int32_t value)
{
    if (value < 0) {
        value = -value;
    }

    return value;
}

static int32_t CarDebug_GetExpectedLeftSign(CarDebugState_t state)
{
    if ((state == CAR_DEBUG_STATE_FORWARD) ||
        (state == CAR_DEBUG_STATE_TURN_RIGHT)) {
        return 1;
    }
    if ((state == CAR_DEBUG_STATE_BACKWARD) ||
        (state == CAR_DEBUG_STATE_TURN_LEFT)) {
        return -1;
    }

    return 0;
}

static int32_t CarDebug_GetExpectedRightSign(CarDebugState_t state)
{
    if ((state == CAR_DEBUG_STATE_FORWARD) ||
        (state == CAR_DEBUG_STATE_TURN_LEFT)) {
        return 1;
    }
    if ((state == CAR_DEBUG_STATE_BACKWARD) ||
        (state == CAR_DEBUG_STATE_TURN_RIGHT)) {
        return -1;
    }

    return 0;
}

static void CarDebug_CheckDirection(uint32_t *errorFlags, int32_t speed,
    int32_t expectedSign, uint32_t errorFlag)
{
    if (expectedSign > 0) {
        if (speed < 0) {
            *errorFlags |= errorFlag;
        }
    } else if (expectedSign < 0) {
        if (speed > 0) {
            *errorFlags |= errorFlag;
        }
    }
}
