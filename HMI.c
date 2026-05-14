#include "tm4c123gh6pm.h"
#include "HMI.h"

void LED_Init(void)
{
    SYSCTL_RCGCGPIO_R |= 0x20;
    while ((SYSCTL_PRGPIO_R & 0x20) == 0)
    {
    }

    GPIO_PORTF_DIR_R |= (GREEN | RED | BLUE);
    GPIO_PORTF_DEN_R |= (GREEN | RED | BLUE);
}

void Buttons_Init(void)
{
    SYSCTL_RCGCGPIO_R |= INPUT_PORT_CLOCK;

    while ((SYSCTL_PRGPIO_R & INPUT_PORT_CLOCK) == 0)
    {
    }

    GPIO_PORTB_DIR_R &= ~ALL_INPUT_PINS; // Set PB0-PB6 as input
    GPIO_PORTB_DEN_R |= ALL_INPUT_PINS;  // Enable digital function
    GPIO_PORTB_PUR_R |= ALL_INPUT_PINS;  // Enable pull-up resistors

    // config interrupt for limit switches and obstacle detection (Falling Edge)

    GPIO_PORTB_IS_R &= ~(OPEN_LIMIT_BTN | CLOSED_LIMIT_BTN | OBSTACLE_BTN);  // Edge-sensitive
    GPIO_PORTB_IBE_R &= ~(OPEN_LIMIT_BTN | CLOSED_LIMIT_BTN | OBSTACLE_BTN); // Not Both Edges
    GPIO_PORTB_IEV_R &= ~(OPEN_LIMIT_BTN | CLOSED_LIMIT_BTN | OBSTACLE_BTN); // Falling edge (button press)
    GPIO_PORTB_ICR_R = (OPEN_LIMIT_BTN | CLOSED_LIMIT_BTN | OBSTACLE_BTN);   // Clear any prior interrupts
    GPIO_PORTB_IM_R |= (OPEN_LIMIT_BTN | CLOSED_LIMIT_BTN | OBSTACLE_BTN);   // Enable interrupts for the switches

    NVIC_EN0_R |= (1 << 1);

    NVIC_PRI0_R = (NVIC_PRI0_R & 0xFFFF00FF) | (5 << 13);
}

void LED_Set(uint8_t led, LED_Action_t action)
{
    switch (action)
    {
    case ON:
        GPIO_PORTF_DATA_R |= led;
        break;

    case OFF:
        GPIO_PORTF_DATA_R &= ~led;
        break;

    case TOGGLE:
        GPIO_PORTF_DATA_R ^= led;
        break;
    }
}

void LED_AllOff(void)
{
    GPIO_PORTF_DATA_R &= ~(GREEN | RED | BLUE);
}

InputData_t Buttons_Read(void)
{
    InputData_t input;

    input.driverOpen = (GPIO_PORTB_DATA_R & DRIVER_OPEN_BTN) ? 0 : 1;
    input.driverClose = (GPIO_PORTB_DATA_R & DRIVER_CLOSE_BTN) ? 0 : 1;
    input.securityOpen = (GPIO_PORTB_DATA_R & SECURITY_OPEN_BTN) ? 0 : 1;
    input.securityClose = (GPIO_PORTB_DATA_R & SECURITY_CLOSE_BTN) ? 0 : 1;

    // those commented out buttons are moved to run with an interrupt for faster response
    // 3a4an zarayer safety we keda

    // input.openLimit = (GPIO_PORTB_DATA_R & OPEN_LIMIT_BTN) ? 0 : 1;
    // input.closedLimit = (GPIO_PORTB_DATA_R & CLOSED_LIMIT_BTN) ? 0 : 1;
    // input.obstacle = (GPIO_PORTB_DATA_R & OBSTACLE_BTN) ? 0 : 1;

    return input;
}
