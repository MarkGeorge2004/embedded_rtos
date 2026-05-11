#ifndef PROJECT_SHARED_H
#define PROJECT_SHARED_H

#include <stdint.h>
#include "FreeRTOS.h"
#include "queue.h"
#include "semphr.h"

typedef enum {
    IDLE_OPEN,
    IDLE_CLOSED,
    OPENING,
    CLOSING,
    STOPPED_MIDWAY,
    REVERSING
} GateState_t;

typedef enum {
    CMD_NONE,
    CMD_OPEN,
    CMD_CLOSE,
    CMD_STOP,
    CMD_SAFETY_REVERSE
} GateCommand_t;

typedef enum {
    MODE_MANUAL,
    MODE_AUTO
} GateMode_t;

typedef enum {     // commandssource  
    SOURCE_DRIVER = 0,
    SOURCE_SECURITY = 1,
    SOURCE_SAFETY = 2
} CommandSource_t;

typedef struct {      // push buttons inputs
    uint8_t driverOpen;
    uint8_t driverClose;
    uint8_t securityOpen;
    uint8_t securityClose;
    uint8_t openLimit;
    uint8_t closedLimit;
    uint8_t obstacle;
} InputData_t;

typedef struct {
    GateCommand_t command;
    GateMode_t mode;
    uint8_t source;
} GateEvent_t;

extern QueueHandle_t xInputQueue;
extern QueueHandle_t xGateQueue;
extern SemaphoreHandle_t xGateStateMutex;
extern SemaphoreHandle_t xInputStateMutex;
extern SemaphoreHandle_t xOpenLimitSemaphore;
extern SemaphoreHandle_t xClosedLimitSemaphore;


extern volatile GateState_t g_gateState;
extern volatile GateMode_t g_gateMode;
extern volatile InputData_t g_latestInput;

#endif
