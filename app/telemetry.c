#include "app/telemetry.h"

#include <stdio.h>
#include <stdint.h>

#include "app/car_debug.h"
#include "app/chassis.h"
#include "ti_msp_dl_config.h"

static void Telemetry_SendChar(char data);
static void Telemetry_SendString(const char *data);

void Telemetry_SendWaveform(void)
{
    char buffer[128];
    int length = snprintf(buffer, sizeof(buffer),
        "{B%ld:%ld:%ld:%ld:%ld:%ld:%lu:%ld}$\r\n",
        (long)gChassisLeftTarget,
        (long)gChassisLeftFeedback,
        (long)gChassisLeftOutput,
        (long)gChassisRightTarget,
        (long)gChassisRightFeedback,
        (long)gChassisRightOutput,
        (unsigned long)gCarDebugErrorFlags,
        (long)gCarMotionState);

    if ((length > 0) && (length < (int)sizeof(buffer))) {
        Telemetry_SendString(buffer);
    }
}

static void Telemetry_SendChar(char data)
{
    DL_UART_Main_transmitDataBlocking(UART_DEBUG_INST, (uint8_t)data);
}

static void Telemetry_SendString(const char *data)
{
    while (*data != '\0') {
        Telemetry_SendChar(*data);
        data++;
    }
}
