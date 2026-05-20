/*
 *   002led_button.c
 *
 *  Created on		: 18 May 2026
 *  Author			: Tara Alexander
 */


#include "stm32f103xx.h"

#define LOW 		DISABLE
#define BTN_PRESSED LOW

void delay() {
	for (uint32_t i = 0; i < 500000/2; i++);
}

int main(void) {
	GPIO_Handle_t GpioLed,GPIOBtn;

	GpioLed.pGPIOx = GPIOA;
	GpioLed.GPIO_PinConfig.GPIO_PinNumber = GPIO_PIN_NO_5;
	GpioLed.GPIO_PinConfig.GPIO_PinMode = GPIO_MODE_OUT_50;
	GpioLed.GPIO_PinConfig.GPIO_PinCnf = GPIO_CNF_OUT_PP;

	GPIO_PeriClockControl(GPIOA, ENABLE);
	GPIO_Init(&GpioLed);

	GPIOBtn.pGPIOx = GPIOC;
	GPIOBtn.GPIO_PinConfig.GPIO_PinNumber = GPIO_PIN_NO_13;
	GPIOBtn.GPIO_PinConfig.GPIO_PinMode = GPIO_MODE_IN;
	GPIOBtn.GPIO_PinConfig.GPIO_PinCnf = GPIO_CNF_IN_PUPD;
	GPIOBtn.GPIO_PinConfig.GPIO_PinPuPdControl= GPIO_PULLUP;

	GPIO_PeriClockControl(GPIOC, ENABLE);
	GPIO_Init(&GPIOBtn);

	while (1) {
		if(GPIO_ReadFromInputPin(GPIOC, GPIO_PIN_NO_13)== BTN_PRESSED)
		{	delay();
			GPIO_ToggleOutputPin(GPIOA, GPIO_PIN_NO_5);
		}

	}
	return 0;
}

