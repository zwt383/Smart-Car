#ifndef BSP_ENCODER_H_
#define BSP_ENCODER_H_

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum
{
    ENCODER_LEFT = 0,
    ENCODER_RIGHT
} Encoder_t;

extern volatile int32_t gEncoderLeftCount;
extern volatile int32_t gEncoderRightCount;
extern volatile int32_t gEncoderLeftDelta;
extern volatile int32_t gEncoderRightDelta;

void Encoder_Init(void);
void Encoder_Reset(void);
int32_t Encoder_GetCount(Encoder_t encoder);
int32_t Encoder_GetDeltaAndReset(Encoder_t encoder);
void Encoder_SnapshotAndReset(void);

#ifdef __cplusplus
}
#endif

#endif /* BSP_ENCODER_H_ */
