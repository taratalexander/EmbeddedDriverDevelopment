/*
 *   004button_interrupt
 *
 *  Created on		: 18 May 2026
 *  Author			: Tara Alexander
 *
 * This program demonstrates GPIO interrupt handling using EXTI
 * peripheral in STM32F103RB.
 *
 * An external push button is connected to PA0 and an external
 * LED is connected to PB0.
 *
 * The button pin PA0 is configured in Falling Edge Trigger
 * interrupt mode with internal pull-up resistor enabled.
 */


#include "stm32f103xx.h"
#include<string.h>

void delay() {
	for (uint32_t i = 0; i < 500000/2; i++);
}

int main(void) {
	GPIO_Handle_t GpioLed,GpioBtn;
	memset(&GpioLed,0,sizeof(GpioLed));
	memset(&GpioBtn,0,sizeof(GpioBtn));

	GpioLed.pGPIOx = GPIOB;
	GpioLed.GPIO_PinConfig.GPIO_PinNumber = GPIO_PIN_NO_0;
	GpioLed.GPIO_PinConfig.GPIO_PinMode = GPIO_MODE_OUT_50;
	GpioLed.GPIO_PinConfig.GPIO_PinCnf = GPIO_CNF_OUT_PP;

	GPIO_PeriClockControl(GPIOB, ENABLE);
	GPIO_Init(&GpioLed);

	GpioBtn.pGPIOx = GPIOA;
	GpioBtn.GPIO_PinConfig.GPIO_PinNumber = GPIO_PIN_NO_0;
	GpioBtn.GPIO_PinConfig.GPIO_PinMode = GPIO_MODE_IT_FT;
	GpioBtn.GPIO_PinConfig.GPIO_PinCnf = GPIO_CNF_IN_PUPD;
	GpioBtn.GPIO_PinConfig.GPIO_PinPuPdControl= GPIO_PULLUP;

	GPIO_PeriClockControl(GPIOA, ENABLE);
	GPIO_AFIO_ClockCntrl(ENABLE);
	GPIO_Init(&GpioBtn);

	//IRQ Configuration
	GPIO_IRQPriorityConfig(IRQ_NO_EXTI0, NVIC_IRQ_PR15);
	GPIO_IRQInterruptConfig(IRQ_NO_EXTI0, ENABLE);

	while(1);

	return 0;
}
void EXTI0_IRQHandler(void){
	delay();
	GPIO_IRQHandling(GPIO_PIN_NO_0);
	GPIO_ToggleOutputPin(GPIOB, GPIO_PIN_NO_0);
}

