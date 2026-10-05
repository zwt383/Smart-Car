#include "bsp/speed_measure.h"

#include "bsp/encoder.h"
#include "ti_msp_dl_config.h"

volatile int32_t gSpeedLeftCount = 0;
volatile int32_t gSpeedRightCount = 0;
volatile int32_t gSpeedLeftAbsCount = 0;
volatile int32_t gSpeedRightAbsCount = 0;
volatile int32_t gSpeedLeftTotalCount = 0;
volatile int32_t gSpeedRightTotalCount = 0;

static int32_t SpeedMeasure_Abs(int32_t value);

void SpeedMeasure_Init(void)
{
    SpeedMeasure_Reset();
}

void SpeedMeasure_Reset(void)
{
    __disable_irq();
    gSpeedLeftCount = 0;
    gSpeedRightCount = 0;
    gSpeedLeftAbsCount = 0;
    gSpeedRightAbsCount = 0;
    gSpeedLeftTotalCount = 0;
    gSpeedRightTotalCount = 0;
    __enable_irq();

    Encoder_Reset();
}

void SpeedMeasure_Update(void)
{
    int32_t leftCount = Encoder_GetDeltaAndReset(ENCODER_LEFT);
    int32_t rightCount = Encoder_GetDeltaAndReset(ENCODER_RIGHT);

    __disable_irq();
    gSpeedLeftCount = leftCount;
    gSpeedRightCount = rightCount;
    gSpeedLeftAbsCount = SpeedMeasure_Abs(leftCount);
    gSpeedRightAbsCount = SpeedMeasure_Abs(rightCount);
    gSpeedLeftTotalCount += leftCount;
    gSpeedRightTotalCount += rightCount;
    __enable_irq();
}

static int32_t SpeedMeasure_Abs(int32_t value)
{
    if (value < 0) {
        value = -value;
    }

    return value;
}
