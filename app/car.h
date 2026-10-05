#ifndef APP_CAR_H_
#define APP_CAR_H_

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

void Car_Init(void);
void Car_Forward(uint16_t speed);
void Car_Backward(uint16_t speed);
void Car_TurnLeft(uint16_t speed);
void Car_TurnRight(uint16_t speed);
void Car_Stop(void);

#ifdef __cplusplus
}
#endif

#endif /* APP_CAR_H_ */
