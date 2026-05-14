#ifndef HMI_H
#define HMI_H

#include "ProjectShared.h"

#include <stdint.h>
#include <stdbool.h>

// portf
#define GREEN (1U << 3)
#define RED (1U << 1)
#define BLUE (1U << 2)

#define INPUT_PORT_CLOCK 0x02U       // Port B clock
#define DRIVER_OPEN_BTN (1U << 0)    // PB0
#define DRIVER_CLOSE_BTN (1U << 1)   // PB1
#define SECURITY_OPEN_BTN (1U << 2)  // PB2
#define SECURITY_CLOSE_BTN (1U << 3) // PB3
#define OPEN_LIMIT_BTN (1U << 4)     // PB4
#define CLOSED_LIMIT_BTN (1U << 5)   // PB5
#define OBSTACLE_BTN (1U << 6)       // PB6

#define ALL_INPUT_PINS 0x7FU // PB0 to PB6

typedef enum
{
    OFF,
    ON,
    TOGGLE
} LED_Action_t;

extern void LED_Init(void);
extern void LED_Set(uint8_t led, LED_Action_t action);
extern void LED_AllOff(void);

extern void Buttons_Init(void);
extern InputData_t Buttons_Read(void);

#endif
