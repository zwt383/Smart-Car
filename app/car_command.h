#ifndef APP_CAR_COMMAND_H_
#define APP_CAR_COMMAND_H_

#include <stdint.h>

#include "app/chassis.h"

#ifdef __cplusplus
extern "C" {
#endif

extern volatile int32_t gCarCommandLastChar;
extern volatile int32_t gCarCommandSpeedLevel;
extern volatile int32_t gCarCommandTimeoutCycles;
extern volatile int32_t gCarCommandLeftTarget;
extern volatile int32_t gCarCommandRightTarget;
extern volatile uint32_t gCarCommandRxOverflowCount;
extern volatile int32_t gCarCommandDistanceTarget;
extern volatile int32_t gCarCommandDistanceRemaining;
extern volatile int32_t gCarCommandAngleTarget;
extern volatile int32_t gCarCommandAngleRemaining;

void CarCommand_Init(void);
void CarCommand_Process(void);

#ifdef __cplusplus
}
#endif

#endif /* APP_CAR_COMMAND_H_ */
