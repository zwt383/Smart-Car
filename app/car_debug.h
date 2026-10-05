#ifndef APP_CAR_DEBUG_H_
#define APP_CAR_DEBUG_H_

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum
{
    CAR_DEBUG_STATE_STOP = 0,
    CAR_DEBUG_STATE_TURN_RIGHT,
    CAR_DEBUG_STATE_TURN_LEFT,
    CAR_DEBUG_STATE_BACKWARD,
    CAR_DEBUG_STATE_FORWARD
} CarDebugState_t;

#define CAR_DEBUG_ERROR_NONE                 (0x00000000U)
#define CAR_DEBUG_ERROR_LEFT_NO_SPEED        (0x00000001U)
#define CAR_DEBUG_ERROR_RIGHT_NO_SPEED       (0x00000002U)
#define CAR_DEBUG_ERROR_LEFT_DIR             (0x00000004U)
#define CAR_DEBUG_ERROR_RIGHT_DIR            (0x00000008U)
#define CAR_DEBUG_ERROR_SPEED_IMBALANCE      (0x00000010U)

extern volatile int32_t gCarMotionState;
extern volatile int32_t gCarSpeedLeft;
extern volatile int32_t gCarSpeedRight;
extern volatile int32_t gCarSpeedLeftAbs;
extern volatile int32_t gCarSpeedRightAbs;
extern volatile int32_t gCarSpeedDiff;
extern volatile int32_t gCarSpeedRatioPermille;
extern volatile int32_t gCarDebugGraceCycles;
extern volatile uint32_t gCarDebugErrorFlags;

void CarDebug_Reset(void);
void CarDebug_SetState(CarDebugState_t state);
void CarDebug_Update(void);

#ifdef __cplusplus
}
#endif

#endif /* APP_CAR_DEBUG_H_ */
