#include "app/car_command.h"

#include "ti_msp_dl_config.h"
#include "app/car_debug.h"
#include "bsp/speed_measure.h"

#define CAR_COMMAND_SWITCH_DELAY_CYCLES    (10)
#define CAR_COMMAND_TIMEOUT_CYCLES         (250)
#define CAR_COMMAND_LINE_MAX_LEN           (24)
#define CAR_COMMAND_WHEEL_TARGET_LIMIT     (CHASSIS_SPEED_TARGET_HIGH)
#define CAR_COMMAND_RX_BUFFER_SIZE         (64U)
#define CAR_COMMAND_DISTANCE_TARGET_LIMIT  (100000)
#define CAR_COMMAND_ANGLE_TARGET_LIMIT     (100000)

volatile int32_t gCarCommandLastChar = 0;
volatile int32_t gCarCommandSpeedLevel = CHASSIS_SPEED_LOW;
volatile int32_t gCarCommandTimeoutCycles = 0;
volatile int32_t gCarCommandLeftTarget = 0;
volatile int32_t gCarCommandRightTarget = 0;
volatile int32_t gCarCommandDistanceTarget = 0;
volatile int32_t gCarCommandDistanceRemaining = 0;
volatile int32_t gCarCommandAngleTarget = 0;
volatile int32_t gCarCommandAngleRemaining = 0;
volatile uint32_t gCarCommandRxOverflowCount = 0;

static ChassisSpeedLevel_t gCurrentSpeedLevel = CHASSIS_SPEED_LOW;
static ChassisMotion_t gCurrentMotion = CHASSIS_MOTION_STOP;
static ChassisMotion_t gPendingMotion = CHASSIS_MOTION_STOP;
static uint16_t gSwitchDelayCycles = 0;
static uint16_t gCommandTimeoutCycles = 0;
static char gCommandLine[CAR_COMMAND_LINE_MAX_LEN];
static uint8_t gCommandLineLength = 0;
static uint8_t gCommandLineActive = 0;
static volatile uint8_t gRxBuffer[CAR_COMMAND_RX_BUFFER_SIZE];
static volatile uint16_t gRxWriteIndex = 0;
static volatile uint16_t gRxReadIndex = 0;

static int32_t gActiveDistanceTarget = 0;
static uint8_t gDistanceModeActive = 0;

static int32_t gActiveAngleTarget = 0;
static uint8_t gAngleModeActive = 0;

static void CarCommand_UartRxInit(void);
static void CarCommand_StoreRxByte(uint8_t data);
static uint8_t CarCommand_ReadRxByte(uint8_t *data);
static void CarCommand_RequestMotion(ChassisMotion_t motion);
static void CarCommand_ApplySpeedLevel(ChassisSpeedLevel_t speedLevel);
static void CarCommand_ServicePendingMotion(void);
static void CarCommand_ServiceTimeout(void);
static void CarCommand_StopMotion(void);
static void CarCommand_ResetTimeout(void);
static void CarCommand_ApplyActiveMotion(ChassisMotion_t motion);
static void CarCommand_ApplyWheelTargets(int16_t leftTarget,
    int16_t rightTarget);
static void CarCommand_SetDebugStateForTargets(int16_t leftTarget,
    int16_t rightTarget);
static void CarCommand_StartLine(uint8_t data);
static void CarCommand_AppendLine(uint8_t data);
static void CarCommand_ProcessLine(void);
static uint8_t CarCommand_ParseWheelTargetLine(const char *line,
    int16_t *leftTarget, int16_t *rightTarget);
static uint8_t CarCommand_ParseDistanceLine(const char *line,
    int32_t *distance);
static uint8_t CarCommand_ParseAngleLine(const char *line,
    int32_t *angle);
static uint8_t CarCommand_ParseInt16(const char **cursor, int16_t *value);
static uint8_t CarCommand_ParseInt32(const char **cursor, int32_t *value);
static void CarCommand_SkipSeparators(const char **cursor);
static int16_t CarCommand_LimitWheelTarget(int16_t target);
static void CarCommand_ProcessChar(uint8_t data);
static void CarCommand_RequestDistanceMotion(int32_t counts);
static void CarCommand_ServiceDistanceStop(void);
static void CarCommand_RequestAngleMotion(int32_t counts);
static void CarCommand_ServiceAngleStop(void);

void CarCommand_Init(void)
{
    gCurrentSpeedLevel = CHASSIS_SPEED_LOW;
    gCurrentMotion = CHASSIS_MOTION_STOP;
    gPendingMotion = CHASSIS_MOTION_STOP;
    gSwitchDelayCycles = 0;
    gCommandTimeoutCycles = 0;
    gCommandLineLength = 0;
    gCommandLineActive = 0;
    gCarCommandLastChar = 0;
    gCarCommandSpeedLevel = gCurrentSpeedLevel;
    gCarCommandTimeoutCycles = gCommandTimeoutCycles;
    gCarCommandLeftTarget = 0;
    gCarCommandRightTarget = 0;
    gCarCommandRxOverflowCount = 0;
    gCarCommandDistanceTarget = 0;
    gCarCommandDistanceRemaining = 0;
    gCarCommandAngleTarget = 0;
    gCarCommandAngleRemaining = 0;
    gActiveDistanceTarget = 0;
    gDistanceModeActive = 0;
    gActiveAngleTarget = 0;
    gAngleModeActive = 0;
    gRxWriteIndex = 0;
    gRxReadIndex = 0;
    Chassis_Stop();
    CarDebug_SetState(CAR_DEBUG_STATE_STOP);
    CarCommand_UartRxInit();
}

void CarCommand_Process(void)
{
    uint8_t data;

    while (CarCommand_ReadRxByte(&data) != 0U) {
        CarCommand_ProcessChar(data);
    }

    CarCommand_ServicePendingMotion();
    CarCommand_ServiceTimeout();
    CarCommand_ServiceDistanceStop();
    CarCommand_ServiceAngleStop();
}

void UART_DEBUG_INST_IRQHandler(void)
{
    switch (DL_UART_Main_getPendingInterrupt(UART_DEBUG_INST)) {
        case DL_UART_IIDX_RX:
        case DL_UART_IIDX_RX_TIMEOUT_ERROR:
            while (!DL_UART_Main_isRXFIFOEmpty(UART_DEBUG_INST)) {
                CarCommand_StoreRxByte(DL_UART_Main_receiveData(UART_DEBUG_INST));
            }
            break;
        case DL_UART_IIDX_OVERRUN_ERROR:
        case DL_UART_IIDX_BREAK_ERROR:
        case DL_UART_IIDX_PARITY_ERROR:
        case DL_UART_IIDX_FRAMING_ERROR:
            while (!DL_UART_Main_isRXFIFOEmpty(UART_DEBUG_INST)) {
                (void)DL_UART_Main_receiveData(UART_DEBUG_INST);
            }
            gCarCommandRxOverflowCount++;
            break;
        default:
            break;
    }
}

static void CarCommand_UartRxInit(void)
{
    while (!DL_UART_Main_isRXFIFOEmpty(UART_DEBUG_INST)) {
        (void)DL_UART_Main_receiveData(UART_DEBUG_INST);
    }

    DL_UART_Main_setRXFIFOThreshold(UART_DEBUG_INST,
        DL_UART_RX_FIFO_LEVEL_ONE_ENTRY);
    DL_UART_Main_enableInterrupt(UART_DEBUG_INST,
        DL_UART_INTERRUPT_RX |
        DL_UART_INTERRUPT_RX_TIMEOUT_ERROR |
        DL_UART_INTERRUPT_OVERRUN_ERROR |
        DL_UART_INTERRUPT_BREAK_ERROR |
        DL_UART_INTERRUPT_PARITY_ERROR |
        DL_UART_INTERRUPT_FRAMING_ERROR);
    NVIC_EnableIRQ(UART_DEBUG_INST_INT_IRQN);
}

static void CarCommand_StoreRxByte(uint8_t data)
{
    uint16_t nextWriteIndex = gRxWriteIndex + 1U;

    if (nextWriteIndex >= CAR_COMMAND_RX_BUFFER_SIZE) {
        nextWriteIndex = 0U;
    }

    if (nextWriteIndex == gRxReadIndex) {
        gCarCommandRxOverflowCount++;
        return;
    }

    gRxBuffer[gRxWriteIndex] = data;
    gRxWriteIndex = nextWriteIndex;
}

static uint8_t CarCommand_ReadRxByte(uint8_t *data)
{
    uint8_t hasData = 0U;

    __disable_irq();
    if (gRxReadIndex != gRxWriteIndex) {
        *data = gRxBuffer[gRxReadIndex];
        gRxReadIndex++;
        if (gRxReadIndex >= CAR_COMMAND_RX_BUFFER_SIZE) {
            gRxReadIndex = 0U;
        }
        hasData = 1U;
    }
    __enable_irq();

    return hasData;
}

static void CarCommand_RequestMotion(ChassisMotion_t motion)
{
    SpeedMeasure_Reset();

    if (motion == CHASSIS_MOTION_STOP) {
        CarCommand_StopMotion();
        return;
    }

    gActiveDistanceTarget = 0;
    gDistanceModeActive = 0;
    gCarCommandDistanceTarget = 0;
    gCarCommandDistanceRemaining = 0;
    gActiveAngleTarget = 0;
    gAngleModeActive = 0;
    gCarCommandAngleTarget = 0;
    gCarCommandAngleRemaining = 0;

    gCurrentMotion = CHASSIS_MOTION_STOP;
    gPendingMotion = motion;
    gSwitchDelayCycles = CAR_COMMAND_SWITCH_DELAY_CYCLES;
    gCarCommandLeftTarget = 0;
    gCarCommandRightTarget = 0;
    CarCommand_ResetTimeout();
    Chassis_Stop();
    CarDebug_SetState(CAR_DEBUG_STATE_STOP);
}

static void CarCommand_ApplySpeedLevel(ChassisSpeedLevel_t speedLevel)
{
    SpeedMeasure_Reset();
    gCurrentSpeedLevel = speedLevel;
    gCarCommandSpeedLevel = gCurrentSpeedLevel;
    CarCommand_ResetTimeout();

    if ((gSwitchDelayCycles == 0U) &&
        (gCurrentMotion != CHASSIS_MOTION_STOP)) {
        Chassis_SetMotion(gCurrentMotion, gCurrentSpeedLevel);
    }
}

static void CarCommand_ServicePendingMotion(void)
{
    if (gSwitchDelayCycles == 0U) {
        return;
    }

    Chassis_Stop();
    CarDebug_SetState(CAR_DEBUG_STATE_STOP);
    gSwitchDelayCycles--;

    if (gSwitchDelayCycles == 0U) {
        SpeedMeasure_Reset();
        CarCommand_ApplyActiveMotion(gPendingMotion);
    }
}

static void CarCommand_ServiceTimeout(void)
{
    if ((gCurrentMotion == CHASSIS_MOTION_STOP) &&
        (gPendingMotion == CHASSIS_MOTION_STOP) &&
        (gCarCommandLeftTarget == 0) &&
        (gCarCommandRightTarget == 0)) {
        gCommandTimeoutCycles = 0;
        gCarCommandTimeoutCycles = gCommandTimeoutCycles;
        return;
    }

    if (gCommandTimeoutCycles > 0U) {
        gCommandTimeoutCycles--;
        gCarCommandTimeoutCycles = gCommandTimeoutCycles;
        return;
    }

    SpeedMeasure_Reset();
    CarCommand_StopMotion();
}

static void CarCommand_StopMotion(void)
{
    gCurrentMotion = CHASSIS_MOTION_STOP;
    gPendingMotion = CHASSIS_MOTION_STOP;
    gSwitchDelayCycles = 0;
    gCommandTimeoutCycles = 0;
    gCarCommandTimeoutCycles = gCommandTimeoutCycles;
    gCarCommandLeftTarget = 0;
    gCarCommandRightTarget = 0;
    gActiveDistanceTarget = 0;
    gDistanceModeActive = 0;
    gCarCommandDistanceTarget = 0;
    gCarCommandDistanceRemaining = 0;
    gActiveAngleTarget = 0;
    gAngleModeActive = 0;
    gCarCommandAngleTarget = 0;
    gCarCommandAngleRemaining = 0;
    gActiveAngleTarget = 0;
    gAngleModeActive = 0;
    gCarCommandAngleTarget = 0;
    gCarCommandAngleRemaining = 0;
    Chassis_Stop();
    CarDebug_SetState(CAR_DEBUG_STATE_STOP);
}

static void CarCommand_ResetTimeout(void)
{
    gCommandTimeoutCycles = CAR_COMMAND_TIMEOUT_CYCLES;
    gCarCommandTimeoutCycles = gCommandTimeoutCycles;
}

static void CarCommand_ApplyActiveMotion(ChassisMotion_t motion)
{
    gCurrentMotion = motion;

    switch (gCurrentMotion) {
        case CHASSIS_MOTION_FORWARD:
            CarDebug_SetState(CAR_DEBUG_STATE_FORWARD);
            break;
        case CHASSIS_MOTION_BACKWARD:
            CarDebug_SetState(CAR_DEBUG_STATE_BACKWARD);
            break;
        case CHASSIS_MOTION_TURN_LEFT:
            CarDebug_SetState(CAR_DEBUG_STATE_TURN_LEFT);
            break;
        case CHASSIS_MOTION_TURN_RIGHT:
            CarDebug_SetState(CAR_DEBUG_STATE_TURN_RIGHT);
            break;
        case CHASSIS_MOTION_STOP:
        default:
            CarDebug_SetState(CAR_DEBUG_STATE_STOP);
            break;
    }

    Chassis_SetMotion(gCurrentMotion, gCurrentSpeedLevel);
}

static void CarCommand_ApplyWheelTargets(int16_t leftTarget,
    int16_t rightTarget)
{
    leftTarget = CarCommand_LimitWheelTarget(leftTarget);
    rightTarget = CarCommand_LimitWheelTarget(rightTarget);

    if ((leftTarget == 0) && (rightTarget == 0)) {
        SpeedMeasure_Reset();
        CarCommand_StopMotion();
        return;
    }

    gActiveDistanceTarget = 0;
    gDistanceModeActive = 0;
    gCarCommandDistanceTarget = 0;
    gCarCommandDistanceRemaining = 0;
    gActiveAngleTarget = 0;
    gAngleModeActive = 0;
    gCarCommandAngleTarget = 0;
    gCarCommandAngleRemaining = 0;

    SpeedMeasure_Reset();
    gCurrentMotion = CHASSIS_MOTION_STOP;
    gPendingMotion = CHASSIS_MOTION_STOP;
    gSwitchDelayCycles = 0;
    gCarCommandLeftTarget = leftTarget;
    gCarCommandRightTarget = rightTarget;
    CarCommand_ResetTimeout();
    CarCommand_SetDebugStateForTargets(leftTarget, rightTarget);
    Chassis_SetWheelSpeedTargets(leftTarget, rightTarget);
}

static void CarCommand_SetDebugStateForTargets(int16_t leftTarget,
    int16_t rightTarget)
{
    if ((leftTarget > 0) && (rightTarget > 0)) {
        CarDebug_SetState(CAR_DEBUG_STATE_FORWARD);
    } else if ((leftTarget < 0) && (rightTarget < 0)) {
        CarDebug_SetState(CAR_DEBUG_STATE_BACKWARD);
    } else if ((leftTarget < 0) && (rightTarget > 0)) {
        CarDebug_SetState(CAR_DEBUG_STATE_TURN_LEFT);
    } else if ((leftTarget > 0) && (rightTarget < 0)) {
        CarDebug_SetState(CAR_DEBUG_STATE_TURN_RIGHT);
    } else {
        CarDebug_SetState(CAR_DEBUG_STATE_STOP);
    }
}

static void CarCommand_StartLine(uint8_t data)
{
    gCommandLineLength = 0;
    gCommandLineActive = 1;
    CarCommand_AppendLine(data);
}

static void CarCommand_AppendLine(uint8_t data)
{
    if (gCommandLineLength < (CAR_COMMAND_LINE_MAX_LEN - 1U)) {
        gCommandLine[gCommandLineLength] = (char)data;
        gCommandLineLength++;
        gCommandLine[gCommandLineLength] = '\0';
    } else {
        gCommandLineLength = 0;
        gCommandLineActive = 0;
    }
}

static void CarCommand_ProcessLine(void)
{
    int16_t leftTarget;
    int16_t rightTarget;
    int32_t distanceTarget;
    int32_t angleTarget;

    if (CarCommand_ParseWheelTargetLine(gCommandLine, &leftTarget,
        &rightTarget) != 0U) {
        CarCommand_ApplyWheelTargets(leftTarget, rightTarget);
    } else if (CarCommand_ParseDistanceLine(gCommandLine,
        &distanceTarget) != 0U) {
        CarCommand_RequestDistanceMotion(distanceTarget);
    } else if (CarCommand_ParseAngleLine(gCommandLine,
        &angleTarget) != 0U) {
        CarCommand_RequestAngleMotion(angleTarget);
    }

    gCommandLineLength = 0;
    gCommandLineActive = 0;
}

static uint8_t CarCommand_ParseWheelTargetLine(const char *line,
    int16_t *leftTarget, int16_t *rightTarget)
{
    const char *cursor = line;

    if ((*cursor != 'V') && (*cursor != 'v')) {
        return 0U;
    }
    cursor++;

    CarCommand_SkipSeparators(&cursor);
    if (CarCommand_ParseInt16(&cursor, leftTarget) == 0U) {
        return 0U;
    }

    CarCommand_SkipSeparators(&cursor);
    if (CarCommand_ParseInt16(&cursor, rightTarget) == 0U) {
        return 0U;
    }

    return 1U;
}

static uint8_t CarCommand_ParseInt16(const char **cursor, int16_t *value)
{
    int32_t result = 0;
    int32_t sign = 1;
    uint8_t hasDigit = 0;

    if (**cursor == '-') {
        sign = -1;
        (*cursor)++;
    } else if (**cursor == '+') {
        (*cursor)++;
    }

    while ((**cursor >= '0') && (**cursor <= '9')) {
        hasDigit = 1;
        result = (result * 10) + (**cursor - '0');
        (*cursor)++;
    }

    if (hasDigit == 0U) {
        return 0U;
    }

    result *= sign;
    if (result > 32767) {
        result = 32767;
    } else if (result < -32768) {
        result = -32768;
    }

    *value = (int16_t)result;
    return 1U;
}

static void CarCommand_SkipSeparators(const char **cursor)
{
    while ((**cursor == ' ') || (**cursor == '\t') || (**cursor == ',') ||
        (**cursor == ':')) {
        (*cursor)++;
    }
}

static int16_t CarCommand_LimitWheelTarget(int16_t target)
{
    if (target > CAR_COMMAND_WHEEL_TARGET_LIMIT) {
        target = CAR_COMMAND_WHEEL_TARGET_LIMIT;
    } else if (target < -CAR_COMMAND_WHEEL_TARGET_LIMIT) {
        target = -CAR_COMMAND_WHEEL_TARGET_LIMIT;
    }

    return target;
}

static void CarCommand_ProcessChar(uint8_t data)
{
    gCarCommandLastChar = data;

    if (gCommandLineActive != 0U) {
        if ((data == '\r') || (data == '\n')) {
            CarCommand_ProcessLine();
        } else {
            CarCommand_AppendLine(data);
        }
        return;
    }

    switch (data) {
        case 'V':
        case 'v':
            CarCommand_StartLine(data);
            break;
        case 'D':
        case 'd':
            CarCommand_StartLine(data);
            break;
        case 'T':
        case 't':
            CarCommand_StartLine(data);
            break;
        case 'F':
        case 'f':
            CarCommand_RequestMotion(CHASSIS_MOTION_FORWARD);
            break;
        case 'B':
        case 'b':
            CarCommand_RequestMotion(CHASSIS_MOTION_BACKWARD);
            break;
        case 'L':
        case 'l':
            CarCommand_RequestMotion(CHASSIS_MOTION_TURN_LEFT);
            break;
        case 'R':
        case 'r':
            CarCommand_RequestMotion(CHASSIS_MOTION_TURN_RIGHT);
            break;
        case 'S':
        case 's':
            CarCommand_RequestMotion(CHASSIS_MOTION_STOP);
            break;
        case '1':
            CarCommand_ApplySpeedLevel(CHASSIS_SPEED_LOW);
            break;
        case '2':
            CarCommand_ApplySpeedLevel(CHASSIS_SPEED_MEDIUM);
            break;
        case '3':
            CarCommand_ApplySpeedLevel(CHASSIS_SPEED_HIGH);
            break;
        default:
            break;
    }
}

static uint8_t CarCommand_ParseDistanceLine(const char *line,
    int32_t *distance)
{
    const char *cursor = line;

    if ((*cursor != 'D') && (*cursor != 'd')) {
        return 0U;
    }
    cursor++;

    CarCommand_SkipSeparators(&cursor);
    if (CarCommand_ParseInt32(&cursor, distance) == 0U) {
        return 0U;
    }

    return 1U;
}

static uint8_t CarCommand_ParseAngleLine(const char *line,
    int32_t *angle)
{
    const char *cursor = line;

    if ((*cursor != 'T') && (*cursor != 't')) {
        return 0U;
    }
    cursor++;

    CarCommand_SkipSeparators(&cursor);
    if (CarCommand_ParseInt32(&cursor, angle) == 0U) {
        return 0U;
    }

    return 1U;
}

static uint8_t CarCommand_ParseInt32(const char **cursor, int32_t *value)
{
    int32_t result = 0;
    int32_t sign = 1;
    uint8_t hasDigit = 0;

    if (**cursor == '-') {
        sign = -1;
        (*cursor)++;
    } else if (**cursor == '+') {
        (*cursor)++;
    }

    while ((**cursor >= '0') && (**cursor <= '9')) {
        hasDigit = 1;
        result = (result * 10) + (**cursor - '0');
        (*cursor)++;
    }

    if (hasDigit == 0U) {
        return 0U;
    }

    result *= sign;
    *value = result;
    return 1U;
}

static void CarCommand_RequestDistanceMotion(int32_t counts)
{
    if (counts <= 0) {
        return;
    }

    if (counts > CAR_COMMAND_DISTANCE_TARGET_LIMIT) {
        counts = CAR_COMMAND_DISTANCE_TARGET_LIMIT;
    }

    SpeedMeasure_Reset();
    gActiveDistanceTarget = counts;
    gDistanceModeActive = 1;
    gCarCommandDistanceTarget = counts;
    gCarCommandDistanceRemaining = counts;

    gCurrentMotion = CHASSIS_MOTION_STOP;
    gPendingMotion = CHASSIS_MOTION_FORWARD;
    gSwitchDelayCycles = CAR_COMMAND_SWITCH_DELAY_CYCLES;
    gCarCommandLeftTarget = 0;
    gCarCommandRightTarget = 0;
    CarCommand_ResetTimeout();
    Chassis_Stop();
    CarDebug_SetState(CAR_DEBUG_STATE_STOP);
}

static void CarCommand_ServiceDistanceStop(void)
{
    int32_t leftTotal;
    int32_t rightTotal;
    int32_t avgTotal;

    if (gDistanceModeActive == 0U) {
        return;
    }

    leftTotal = gSpeedLeftTotalCount;
    rightTotal = gSpeedRightTotalCount;

    /* Average both wheels to avoid premature stop from one wheel stalling */
    avgTotal = (leftTotal + rightTotal + 1) / 2;

    gCarCommandDistanceRemaining = gActiveDistanceTarget - avgTotal;
    if (gCarCommandDistanceRemaining < 0) {
        gCarCommandDistanceRemaining = 0;
    }

    if (avgTotal >= gActiveDistanceTarget) {
        SpeedMeasure_Reset();
        CarCommand_StopMotion();
    }
}

static void CarCommand_RequestAngleMotion(int32_t counts)
{
    ChassisMotion_t motion;

    if (counts == 0) {
        return;
    }

    if (counts > CAR_COMMAND_ANGLE_TARGET_LIMIT) {
        counts = CAR_COMMAND_ANGLE_TARGET_LIMIT;
    } else if (counts < -CAR_COMMAND_ANGLE_TARGET_LIMIT) {
        counts = -CAR_COMMAND_ANGLE_TARGET_LIMIT;
    }

    /* Positive counts: right turn, negative: left turn */
    if (counts > 0) {
        motion = CHASSIS_MOTION_TURN_RIGHT;
    } else {
        motion = CHASSIS_MOTION_TURN_LEFT;
        counts = -counts;
    }

    SpeedMeasure_Reset();
    gActiveAngleTarget = counts;
    gAngleModeActive = 1;
    gCarCommandAngleTarget = counts;
    gCarCommandAngleRemaining = counts;

    gCurrentMotion = CHASSIS_MOTION_STOP;
    gPendingMotion = motion;
    gSwitchDelayCycles = CAR_COMMAND_SWITCH_DELAY_CYCLES;
    gCarCommandLeftTarget = 0;
    gCarCommandRightTarget = 0;
    CarCommand_ResetTimeout();
    Chassis_Stop();
    CarDebug_SetState(CAR_DEBUG_STATE_STOP);
}

static void CarCommand_ServiceAngleStop(void)
{
    int32_t leftTotal;
    int32_t rightTotal;
    int32_t diff;

    if (gAngleModeActive == 0U) {
        return;
    }

    leftTotal = gSpeedLeftTotalCount;
    rightTotal = gSpeedRightTotalCount;

    /* Differential = |left - right|; turning drives wheels opposite directions */
    diff = leftTotal - rightTotal;
    if (diff < 0) {
        diff = -diff;
    }

    gCarCommandAngleRemaining = gActiveAngleTarget - diff;
    if (gCarCommandAngleRemaining < 0) {
        gCarCommandAngleRemaining = 0;
    }

    if (diff >= gActiveAngleTarget) {
        SpeedMeasure_Reset();
        CarCommand_StopMotion();
    }
}
