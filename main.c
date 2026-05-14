#include <stdint.h>
#include <stdbool.h>
#include "tm4c123gh6pm.h"
#include "FreeRTOS.h"
#include "semphr.h"
#include "task.h"
#include "basic_io.h"
#include "queue.h"
#include "ProjectShared.h"
#include "HMI.h"

QueueHandle_t xInputQueue; // from input task to safty task and gate control
SemaphoreHandle_t xGateStateMutex;
SemaphoreHandle_t xObstacleSemaphore;

// steady state of the gate
volatile GateState_t g_gateState = IDLE_CLOSED;
volatile GateMode_t g_gateMode = MODE_AUTO;

void changeGateState_and_Mode(GateState_t newGateState, GateMode_t newModeState)
{
    xSemaphoreTake(xGateStateMutex, portMAX_DELAY);

    g_gateState = newGateState;
    g_gateMode = newModeState;

    xSemaphoreGive(xGateStateMutex);
};

bool CheckOpenButtons(InputData_t Input)
{
    return ((Input.securityOpen && (!Input.securityClose)) ||
            (Input.driverOpen && (!Input.driverClose) && (!Input.securityClose) && (!Input.securityOpen)));
}

bool CheckCloseButtons(InputData_t Input)
{
    return ((Input.securityClose && (!Input.securityOpen)) ||
            (Input.driverClose && (!Input.driverOpen) && (!Input.securityClose) && (!Input.securityOpen)));
}

bool CheckClashingButtons(InputData_t Input)
{
    return ((Input.driverClose && Input.driverOpen) || (Input.securityOpen && Input.securityClose));
}

void vInputTask(void *pvParameters)
{
    InputData_t Input;

    while (1)
    {
        Input = Buttons_Read();
        // Process input data if needed (currently just forwarding to safety task)
        if (Input.obstacle == 1)
        {
            xSemaphoreGive(xObstacleSemaphore);
        }
        else
        {
            xQueueSendToBack(xInputQueue, &Input, portMAX_DELAY);
        }
        vTaskDelay(pdMS_TO_TICKS(200)); // Adjust delay as needed to balance responsiveness and CPU usage
    }
}

void vGateTask(void *pvParameters)
{

    InputData_t Input;

    GateState_t currentState;
    GateMode_t currentMode;

    while (1)
    {
        xSemaphoreTake(xGateStateMutex, portMAX_DELAY);

        currentState = g_gateState;
        currentMode = g_gateMode;

        xSemaphoreGive(xGateStateMutex);

        xQueueReceive(xInputQueue, &Input, portMAX_DELAY);

        switch (currentState)
        {
        case IDLE_OPEN:
            if (CheckCloseButtons(Input))
            {
                changeGateState_and_Mode(CLOSING, MODE_AUTO);
                vTaskDelay(pdMS_TO_TICKS(200));
            }
            break;

        case IDLE_CLOSED:
            if (CheckOpenButtons(Input))

            {
                changeGateState_and_Mode(OPENING, MODE_AUTO);
                vTaskDelay(pdMS_TO_TICKS(200));
            }
            break;

        case OPENING:

            if (Input.openLimit)
            {
                changeGateState_and_Mode(IDLE_OPEN, MODE_AUTO);
            }
            else if (CheckClashingButtons(Input))
            {
                changeGateState_and_Mode(STOPPED_MIDWAY, MODE_AUTO);
            }
            else if (Input.securityClose && (!Input.securityOpen))
            {
                changeGateState_and_Mode(CLOSING, MODE_AUTO);
                vTaskDelay(pdMS_TO_TICKS(200));
            }
            else if (currentMode == MODE_AUTO && CheckOpenButtons(Input))
            {
                changeGateState_and_Mode(OPENING, MODE_MANUAL);
            }
            else if (currentMode == MODE_MANUAL && !CheckOpenButtons(Input))
            {
                changeGateState_and_Mode(STOPPED_MIDWAY, MODE_AUTO);
            }

            break;

        case CLOSING:

            if (Input.closedLimit)
            {
                changeGateState_and_Mode(IDLE_CLOSED, MODE_AUTO);
            }
            else if (CheckClashingButtons(Input))
            {
                changeGateState_and_Mode(STOPPED_MIDWAY, MODE_AUTO);
            }
            else if (Input.securityOpen && (!Input.securityClose))
            {
                changeGateState_and_Mode(OPENING, MODE_AUTO);
                vTaskDelay(pdMS_TO_TICKS(200));
            }
            else if (currentMode == MODE_AUTO && CheckCloseButtons(Input))
            {
                changeGateState_and_Mode(CLOSING, MODE_MANUAL);
            }
            else if (currentMode == MODE_MANUAL && !CheckCloseButtons(Input))
            {
                changeGateState_and_Mode(STOPPED_MIDWAY, MODE_AUTO);
            }
            break;

        case STOPPED_MIDWAY:
            if (CheckOpenButtons(Input))
            {
                changeGateState_and_Mode(OPENING, MODE_AUTO);
                vTaskDelay(pdMS_TO_TICKS(500));
            }

            if (CheckCloseButtons(Input))
            {
                changeGateState_and_Mode(CLOSING, MODE_AUTO);
                vTaskDelay(pdMS_TO_TICKS(500));
            }
            break;

        case REVERSING:
            vTaskDelay(pdMS_TO_TICKS(500));
            changeGateState_and_Mode(STOPPED_MIDWAY, MODE_AUTO);
            break;
        }
    }
}

void vSafetyTask(void *pvParameters)
{

    GateState_t currentState;
    GateMode_t currentMode;
    while (1)
    {
        xSemaphoreTake(xObstacleSemaphore, portMAX_DELAY);

        xSemaphoreTake(xGateStateMutex, portMAX_DELAY);

        currentState = g_gateState;
        currentMode = g_gateMode;

        xSemaphoreGive(xGateStateMutex);

        if (currentState == CLOSING)
        {
            // sent to the Gate_ControlTask
            changeGateState_and_Mode(REVERSING, MODE_AUTO);
        }
    }
}

void vLEDTask(void *pvParameters)
{
    GateState_t currentState;

    while (1)
    {
        xSemaphoreTake(xGateStateMutex, portMAX_DELAY);
        currentState = g_gateState;
        xSemaphoreGive(xGateStateMutex);

        switch (currentState)
        {
        case IDLE_OPEN:
            LED_AllOff();
            break;

        case IDLE_CLOSED:
            LED_AllOff();
            break;

        case OPENING:
            LED_AllOff();
            LED_Set(GREEN, ON);
            break;

        case CLOSING:
            LED_AllOff();
            LED_Set(RED, ON);
            break;

        case STOPPED_MIDWAY:
            LED_AllOff();
            break;

        case REVERSING:
            LED_AllOff();
            LED_Set(GREEN, ON);
            break;
        }
        vTaskDelay(pdMS_TO_TICKS(250));
    }
}

void vStatusTask(void *pvParameters)
{
    GateState_t currentState;
    GateState_t lastState = IDLE_CLOSED;

    while (1)
    {
        xSemaphoreTake(xGateStateMutex, portMAX_DELAY);
        currentState = g_gateState;
        xSemaphoreGive(xGateStateMutex);

        if (currentState != lastState)
        {
            lastState = currentState;

            switch (currentState)
            {
            case IDLE_OPEN:
                vPrintString("Status: IDLE_OPEN\r\n");
                break;

            case IDLE_CLOSED:
                vPrintString("Status: IDLE_CLOSED\r\n");
                break;

            case OPENING:
                vPrintString("Status: OPENING\r\n");
                break;

            case CLOSING:
                vPrintString("Status: CLOSING\r\n");
                break;

            case STOPPED_MIDWAY:
                vPrintString("Status: STOPPED_MIDWAY\r\n");
                break;

            case REVERSING:
                vPrintString("Status: REVERSING\r\n");
                break;
            }
        }

        vTaskDelay(pdMS_TO_TICKS(250));
    }
}
int main(void)
{
    Buttons_Init();
    LED_Init();

    xInputQueue = xQueueCreate(10, sizeof(InputData_t));
    xGateStateMutex = xSemaphoreCreateMutex();
    xObstacleSemaphore = xSemaphoreCreateBinary();

    xTaskCreate(vSafetyTask, "Safety Task", 128, NULL, 4, NULL);
    xTaskCreate(vInputTask, "Input Task", 128, NULL, 3, NULL);
    xTaskCreate(vGateTask, "Gate Task", 512, NULL, 2, NULL);
    xTaskCreate(vLEDTask, "LED Task", 128, NULL, 2, NULL);
    xTaskCreate(vStatusTask, "Status Task", 128, NULL, 1, NULL);

    vTaskStartScheduler();
}