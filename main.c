#include <stdint.h>
#include "tm4c123gh6pm.h"
#include "FreeRTOS.h"
#include "task.h"
#include "basic_io.h"
#include "queue.h"
#include "ProjectShared.h"

QueueHandle_t xInputQueue;  // from input task to safty task and gate control 
QueueHandle_t xGateQueue;   // from safty task to Gate control task
SemaphoreHandle_t xGateStateMutex;

// steady state of the gate
volatile GateState_t g_gateState = IDLE_CLOSED;
volatile GateMode_t g_gateMode = MODE_AUTO;




static void vSafetyTask(void *pvParameters)
{
	InputData_t input;
  GateEvent_t safetyEvent;
	while(1){
		if (xQueueReceive(xInputQueue, &input, portMAX_DELAY) == pdPASS)
			{
				if(input.obstacle==1){
					
								 xSemaphoreTake(xGateStateMutex, portMAX_DELAY);

									GateState_t currentState = g_gateState;
									GateMode_t currentMode = g_gateMode;
					
									xSemaphoreGive(xGateStateMutex);  
			  
						if(currentState == CLOSING && currentMode == MODE_AUTO)
							{
								    safetyEvent.command = CMD_SAFETY_REVERSE;  //sent to the Gate_ControlTask
                                    safetyEvent.mode = MODE_AUTO;
                                    safetyEvent.source = SOURCE_SAFETY;
								 xQueueSendToFront(xGateQueue, &safetyEvent, portMAX_DELAY);
							
						} 
				
				}
				
			}
			
		}
}

static void vStatusTask(void *pvParameters)
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
											// Green_On();
											// Red_Off();
                    break;

                case IDLE_CLOSED:
                    vPrintString("Status: IDLE_CLOSED\r\n");
								    //Red_On();
								    //Greem_Off();
                    break;

                case OPENING:
                    vPrintString("Status: OPENING\r\n");
								    //Green_Toggle();
                    break;

                case CLOSING:
                    vPrintString("Status: CLOSING\r\n");
								    //Red_Toggle();
                    break;

                case STOPPED_MIDWAY:
                    vPrintString("Status: STOPPED_MIDWAY\r\n");
								      //Green_Off();
                      //Red_Off();
                    break;

                case REVERSING:
                    vPrintString("Status: REVERSING\r\n");
								    //Green_Toggle();
                    break;
            }
        }

        vTaskDelay(pdMS_TO_TICKS(250));
    }
}
int main(void)
{
  xInputQueue = xQueueCreate(10, sizeof(InputData_t));
	xGateQueue = xQueueCreate(10, sizeof(GateEvent_t));
	xGateStateMutex = xSemaphoreCreateMutex();	
	
	xTaskCreate(vSafetyTask,"Safety Task",128,NULL,4, NULL);
	xTaskCreate(vStatusTask,"Status Task",128,NULL,1,NULL);
	
	 vTaskStartScheduler();
}