#include "bsp/encoder.h"

#include <stdbool.h>

#include "ti_msp_dl_config.h"

#define ENCODER_LEFT_A_PORT       (GPIOB)
#define ENCODER_LEFT_A_PIN        (DL_GPIO_PIN_10)
#define ENCODER_LEFT_A_IOMUX      (IOMUX_PINCM27)
#define ENCODER_LEFT_B_PORT       (GPIOB)
#define ENCODER_LEFT_B_PIN        (DL_GPIO_PIN_11)
#define ENCODER_LEFT_B_IOMUX      (IOMUX_PINCM28)

#define ENCODER_RIGHT_A_PORT      (GPIOA)
#define ENCODER_RIGHT_A_PIN       (DL_GPIO_PIN_21)
#define ENCODER_RIGHT_A_IOMUX     (IOMUX_PINCM46)
#define ENCODER_RIGHT_B_PORT      (GPIOA)
#define ENCODER_RIGHT_B_PIN       (DL_GPIO_PIN_22)
#define ENCODER_RIGHT_B_IOMUX     (IOMUX_PINCM47)

#define ENCODER_LEFT_PINS         (ENCODER_LEFT_A_PIN | ENCODER_LEFT_B_PIN)
#define ENCODER_RIGHT_PINS        (ENCODER_RIGHT_A_PIN | ENCODER_RIGHT_B_PIN)

#define ENCODER_LEFT_INVERTED     (0)
#define ENCODER_RIGHT_INVERTED    (0)

volatile int32_t gEncoderLeftCount = 0;
volatile int32_t gEncoderRightCount = 0;
volatile int32_t gEncoderLeftDelta = 0;
volatile int32_t gEncoderRightDelta = 0;

static uint8_t gEncoderLeftLastState = 0;
static uint8_t gEncoderRightLastState = 0;

static uint8_t Encoder_ReadLeftState(void);
static uint8_t Encoder_ReadRightState(void);
static void Encoder_Update(volatile int32_t *count, uint8_t *lastState,
    uint8_t currentState, bool inverted);

void Encoder_Init(void)
{
    DL_GPIO_initDigitalInput(ENCODER_LEFT_A_IOMUX);
    DL_GPIO_initDigitalInput(ENCODER_LEFT_B_IOMUX);
    DL_GPIO_initDigitalInput(ENCODER_RIGHT_A_IOMUX);
    DL_GPIO_initDigitalInput(ENCODER_RIGHT_B_IOMUX);

    DL_GPIO_setLowerPinsPolarity(GPIOB,
        DL_GPIO_PIN_10_EDGE_RISE_FALL | DL_GPIO_PIN_11_EDGE_RISE_FALL);
    DL_GPIO_setUpperPinsPolarity(GPIOA,
        DL_GPIO_PIN_21_EDGE_RISE_FALL | DL_GPIO_PIN_22_EDGE_RISE_FALL);

    DL_GPIO_clearInterruptStatus(GPIOB, ENCODER_LEFT_PINS);
    DL_GPIO_clearInterruptStatus(GPIOA, ENCODER_RIGHT_PINS);
    DL_GPIO_enableInterrupt(GPIOB, ENCODER_LEFT_PINS);
    DL_GPIO_enableInterrupt(GPIOA, ENCODER_RIGHT_PINS);

    gEncoderLeftLastState = Encoder_ReadLeftState();
    gEncoderRightLastState = Encoder_ReadRightState();
    Encoder_Reset();

    NVIC_EnableIRQ(GPIOB_INT_IRQn);
}

void Encoder_Reset(void)
{
    __disable_irq();
    gEncoderLeftCount = 0;
    gEncoderRightCount = 0;
    gEncoderLeftDelta = 0;
    gEncoderRightDelta = 0;
    __enable_irq();
}

int32_t Encoder_GetCount(Encoder_t encoder)
{
    int32_t count;

    __disable_irq();
    if (encoder == ENCODER_LEFT) {
        count = gEncoderLeftCount;
    } else {
        count = gEncoderRightCount;
    }
    __enable_irq();

    return count;
}

int32_t Encoder_GetDeltaAndReset(Encoder_t encoder)
{
    int32_t delta;

    __disable_irq();
    if (encoder == ENCODER_LEFT) {
        delta = gEncoderLeftCount;
        gEncoderLeftCount = 0;
    } else {
        delta = gEncoderRightCount;
        gEncoderRightCount = 0;
    }
    __enable_irq();

    return delta;
}

void Encoder_SnapshotAndReset(void)
{
    __disable_irq();
    gEncoderLeftDelta = gEncoderLeftCount;
    gEncoderRightDelta = gEncoderRightCount;
    gEncoderLeftCount = 0;
    gEncoderRightCount = 0;
    __enable_irq();
}

void GROUP1_IRQHandler(void)
{
    uint32_t gpioB = DL_GPIO_getEnabledInterruptStatus(GPIOB, ENCODER_LEFT_PINS);
    uint32_t gpioA = DL_GPIO_getEnabledInterruptStatus(GPIOA, ENCODER_RIGHT_PINS);

    if ((gpioB & ENCODER_LEFT_PINS) != 0U) {
        Encoder_Update(&gEncoderLeftCount, &gEncoderLeftLastState,
            Encoder_ReadLeftState(), ENCODER_LEFT_INVERTED != 0);
        DL_GPIO_clearInterruptStatus(GPIOB, gpioB & ENCODER_LEFT_PINS);
    }

    if ((gpioA & ENCODER_RIGHT_PINS) != 0U) {
        Encoder_Update(&gEncoderRightCount, &gEncoderRightLastState,
            Encoder_ReadRightState(), ENCODER_RIGHT_INVERTED != 0);
        DL_GPIO_clearInterruptStatus(GPIOA, gpioA & ENCODER_RIGHT_PINS);
    }
}

static uint8_t Encoder_ReadLeftState(void)
{
    uint32_t pins = DL_GPIO_readPins(GPIOB, ENCODER_LEFT_PINS);
    uint8_t state = 0;

    if ((pins & ENCODER_LEFT_A_PIN) != 0U) {
        state |= 0x02U;
    }
    if ((pins & ENCODER_LEFT_B_PIN) != 0U) {
        state |= 0x01U;
    }

    return state;
}

static uint8_t Encoder_ReadRightState(void)
{
    uint32_t pins = DL_GPIO_readPins(GPIOA, ENCODER_RIGHT_PINS);
    uint8_t state = 0;

    if ((pins & ENCODER_RIGHT_A_PIN) != 0U) {
        state |= 0x02U;
    }
    if ((pins & ENCODER_RIGHT_B_PIN) != 0U) {
        state |= 0x01U;
    }

    return state;
}

static void Encoder_Update(volatile int32_t *count, uint8_t *lastState,
    uint8_t currentState, bool inverted)
{
    static const int8_t stepTable[16] = {
        0, -1, 1, 0,
        1, 0, 0, -1,
        -1, 0, 0, 1,
        0, 1, -1, 0
    };
    uint8_t index = (uint8_t)((*lastState << 2) | currentState);
    int8_t step = stepTable[index];

    *lastState = currentState;

    if (inverted) {
        step = (int8_t)-step;
    }

    *count += step;
}
