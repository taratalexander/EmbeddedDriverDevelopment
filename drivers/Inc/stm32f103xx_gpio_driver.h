/*
 * stm32f103xx_gpio_driver.h
 *
 *  Created on: 11 Mar 2026
 *      Author: Tara Alexander
 */

#ifndef INC_STM32F103XX_GPIO_DRIVER_H_
#define INC_STM32F103XX_GPIO_DRIVER_H_

#include "stm32f103xx.h"


typedef enum
{
	GPIO_NO_PUPD,
	GPIO_PULLUP,
	GPIO_PULLDOWN
}GPIO_PuPd_t;

typedef struct
{

	uint8_t GPIO_PinNumber;		/*!< possible values from @GPIO_PIN_NUMBERS	>*/
	uint8_t GPIO_PinMode;		/*!< possible values from @GPIO_PIN_MODES	>*/
	uint8_t GPIO_PinCnf;		/*!< possible values from @GPIO_CNF	>*/
	GPIO_PuPd_t GPIO_PinPuPdControl;
}GPIO_PinConfig_t;

/**
 * This is a Handle Structure for a GPIO pin
 */
typedef struct
{

	GPIO_RegDef_t *pGPIOx;			/*|<This holds the base address of the GPIO port to which the pin belongs >|*/
	GPIO_PinConfig_t GPIO_PinConfig;

}GPIO_Handle_t;




/**
 * 		@GPIO_PIN_NUMBERS
 * 		GPIO pin numbers
 */
#define GPIO_PIN_NO_0		0
#define GPIO_PIN_NO_1		1
#define GPIO_PIN_NO_2		2
#define GPIO_PIN_NO_3		3
#define GPIO_PIN_NO_4		4
#define GPIO_PIN_NO_5		5
#define GPIO_PIN_NO_6		6
#define GPIO_PIN_NO_7		7
#define GPIO_PIN_NO_8		8
#define GPIO_PIN_NO_9		9
#define GPIO_PIN_NO_10		10
#define GPIO_PIN_NO_11		11
#define GPIO_PIN_NO_12		12
#define GPIO_PIN_NO_13		13
#define GPIO_PIN_NO_14		14
#define GPIO_PIN_NO_15		15

/**
 * 		@GPIO_CNF
 * 		GPIO pin possible CNF input modes
 * 		Port x configuration bits
 */
#define GPIO_CNF_IN_ANALOG 		0
#define GPIO_CNF_IN_FLOATING 	1
#define GPIO_CNF_IN_PUPD 		2

/*
 * 		GPIO pin possible CNF output modes
 */
#define GPIO_CNF_OUT_PP 		0
#define GPIO_CNF_OUT_OD 		1
#define GPIO_CNF_AF_PP 			2
#define GPIO_CNF_AF_OD 			3

/*
 * 		GPIO pin interrupt modes
 */
#define GPIO_MODE_IT_FT 		4
#define GPIO_MODE_IT_RT 		5
#define GPIO_MODE_IT_RFT 		6

/*
 * 		GPIO_PIN_MODES
 * 		GPIO pin possible MODEs
 * 		Port x mode bits
 */
#define GPIO_MODE_IN 			0
#define GPIO_MODE_OUT_10		1
#define GPIO_MODE_OUT_2			2
#define GPIO_MODE_OUT_50		3

/***************************************************************************************************
 * 								APIs Supported by this driver
 * 	For more information about the APIs check the function definitions
 ****************************************************************************************************/

/*
 * Peripheral Clock Control
 */
void GPIO_PeriClockControl(GPIO_RegDef_t *pGPIOx,uint8_t EnorDi);


/*
 * GPIO Init and De-Init
 */
void GPIO_Init(GPIO_Handle_t *pGPIOHandle);
void GPIO_DeInit(GPIO_RegDef_t *pGPIOx);

/*
 * Data read and write
 */

uint8_t GPIO_ReadFromInputPin(GPIO_RegDef_t *pGPIOx,uint8_t PinNumber);
uint16_t GPIO_ReadFromInputPort(GPIO_RegDef_t *pGPIOx);
void GPIO_WriteToOutputPin(GPIO_RegDef_t *pGPIOx,uint8_t PinNumber,uint8_t Value);
void GPIO_WriteToOutputPort(GPIO_RegDef_t *pGPIOx,uint16_t Value);
void GPIO_ToggleOutputPin(GPIO_RegDef_t *pGPIOx,uint8_t PinNumber);

/*
 * IRQ Configuration and ISR handling
 */
void GPIO_IRQConfig(uint8_t IRQNumber,uint8_t EnorDi);
void GPIO_IRQInterruptConfig(uint8_t IRQNumber,uint8_t IRQPriority);
void GPIO_IRQHandling(uint8_t PinNumber);



#endif /* INC_STM32F103XX_GPIO_DRIVER_H_ */
