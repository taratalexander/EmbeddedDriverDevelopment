/*
 *   001led_toggle.c
 *
 *  Created on		: 8 May 2026
 *  Author			: Tara Alexander
 */

#include "stm32f103xx.h"

void delay() {
	for (uint32_t i = 0; i < 500000/2; i++);
}

int main(void) {
	GPIO_Handle_t GpioLed;

	GpioLed.pGPIOx = GPIOA;
	GpioLed.GPIO_PinConfig.GPIO_PinNumber = GPIO_PIN_NO_5;
	GpioLed.GPIO_PinConfig.GPIO_PinMode = GPIO_MODE_OUT_50;
	GpioLed.GPIO_PinConfig.GPIO_PinCnf = GPIO_CNF_OUT_OD;

	GPIO_PeriClockControl(GPIOA, ENABLE);
	GPIO_Init(&GpioLed);

	while (1) {
		GPIO_ToggleOutputPin(GPIOA, GPIO_PIN_NO_5);
		delay();
	}
	return 0;
}

