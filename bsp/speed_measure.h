#ifndef BSP_SPEED_MEASURE_H_
#define BSP_SPEED_MEASURE_H_

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define SPEED_MEASURE_PERIOD_MS    (20U)

extern volatile int32_t gSpeedLeftCount;
extern volatile int32_t gSpeedRightCount;
extern volatile int32_t gSpeedLeftAbsCount;
extern volatile int32_t gSpeedRightAbsCount;
extern volatile int32_t gSpeedLeftTotalCount;
extern volatile int32_t gSpeedRightTotalCount;

void SpeedMeasure_Init(void);
void SpeedMeasure_Reset(void);
void SpeedMeasure_Update(void);

#ifdef __cplusplus
}
#endif

#endif /* BSP_SPEED_MEASURE_H_ */
