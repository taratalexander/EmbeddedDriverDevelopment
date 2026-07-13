/*
 *   002led_button.c
 *
 *  Created on		: 18 May 2026
 *  Author			: Tara Alexander
 *
 *  Connect external button to the pin number PA0 and external LED to PB0.
 *  Toggle the LED whenever the external button is pressed
 *  Internal PULLUP resistor is used
 *  Use external PULLUP also, then PinCnf = GPIO_CNF_IN_FLOATING and PinPuPdControl = GPIO_NO_PUPD
 */


#include "stm32f103xx.h"

#define LOW 		DISABLE
#define HIGH		ENABLE
#define BTN_PRESSED LOW

void delay() {
	for (uint32_t i = 0; i < 500000/2; i++);
}

int main(void) {
	GPIO_Handle_t GpioLed,GpioBtn;

	GpioLed.pGPIOx = GPIOB;
	GpioLed.GPIO_PinConfig.GPIO_PinNumber = GPIO_PIN_NO_0;
	GpioLed.GPIO_PinConfig.GPIO_PinMode = GPIO_MODE_OUT_50;
	GpioLed.GPIO_PinConfig.GPIO_PinCnf = GPIO_CNF_OUT_PP;


	GPIO_Init(&GpioLed);

	GpioBtn.pGPIOx = GPIOA;
	GpioBtn.GPIO_PinConfig.GPIO_PinNumber = GPIO_PIN_NO_0;
	GpioBtn.GPIO_PinConfig.GPIO_PinMode = GPIO_MODE_IN;
	GpioBtn.GPIO_PinConfig.GPIO_PinCnf = GPIO_CNF_IN_FLOATING;
	GpioBtn.GPIO_PinConfig.GPIO_PinPuPdControl= GPIO_NO_PUPD;


	GPIO_Init(&GpioBtn);

	while (1) {
		if(GPIO_ReadFromInputPin(GPIOA, GPIO_PIN_NO_0)== BTN_PRESSED)
		{	 delay();
			GPIO_ToggleOutputPin(GPIOB, GPIO_PIN_NO_0);
		}

	}
	return 0;
}

