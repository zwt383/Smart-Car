/*
 * Copyright (c) 2021, Texas Instruments Incorporated
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * *  Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *
 * *  Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * *  Neither the name of Texas Instruments Incorporated nor the names of
 *    its contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
 * OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 * OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE,
 * EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "ti_msp_dl_config.h"
#include "app/car.h"
#include "app/car_command.h"
#include "app/car_debug.h"
#include "app/chassis.h"
#include "app/telemetry.h"
#include "bsp/encoder.h"
#include "bsp/speed_measure.h"

/* SysTick: 20ms 控制周期中断 @ 32MHz */
#define CONTROL_SYSTICK_RELOAD      (640000U)   /* 32MHz × 20ms = 640000 */

static volatile uint8_t gControlTick = 0;

void SysTick_Handler(void)
{
    gControlTick = 1;
}

static void ControlTimer_Init(void)
{
    SysTick_Config(CONTROL_SYSTICK_RELOAD);
}

static void ControlTask_20ms(void)
{
    while (gControlTick == 0U) {
        __WFE();
    }
    gControlTick = 0U;

    CarCommand_Process();
    SpeedMeasure_Update();
    Chassis_Update();
    CarDebug_Update();
    Telemetry_SendWaveform();
}

int main(void)
{
    SYSCFG_DL_init();

    ControlTimer_Init();

    Encoder_Init();
    SpeedMeasure_Init();
    CarDebug_Reset();
    Car_Init();
    Chassis_Init();
    CarCommand_Init();
    Chassis_Stop();

    while (1) {
        ControlTask_20ms();
    }
}