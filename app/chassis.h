#ifndef APP_CHASSIS_H_
#define APP_CHASSIS_H_

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define CHASSIS_SPEED_TARGET_LOW       (8)
#define CHASSIS_SPEED_TARGET_MEDIUM    (12)
#define CHASSIS_SPEED_TARGET_HIGH      (16)

typedef enum {
    CHASSIS_MOTION_STOP = 0,
    CHASSIS_MOTION_FORWARD,
    CHASSIS_MOTION_BACKWARD,
    CHASSIS_MOTION_TURN_LEFT,
    CHASSIS_MOTION_TURN_RIGHT
} ChassisMotion_t;

typedef enum {
    CHASSIS_SPEED_LOW = 0,
    CHASSIS_SPEED_MEDIUM,
    CHASSIS_SPEED_HIGH
} ChassisSpeedLevel_t;

extern volatile int32_t gChassisLeftTarget;
extern volatile int32_t gChassisRightTarget;
extern volatile int32_t gChassisLeftFeedback;
extern volatile int32_t gChassisRightFeedback;
extern volatile int32_t gChassisLeftOutput;
extern volatile int32_t gChassisRightOutput;

void Chassis_Init(void);
void Chassis_SetMotion(ChassisMotion_t motion, ChassisSpeedLevel_t speedLevel);
void Chassis_SetWheelSpeedTargets(int16_t leftTarget, int16_t rightTarget);
void Chassis_Stop(void);
void Chassis_Update(void);

#ifdef __cplusplus
}
#endif

#endif /* APP_CHASSIS_H_ */
