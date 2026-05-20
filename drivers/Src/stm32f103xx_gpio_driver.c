/*
 * stm32f103xx_gpio_driver.c
 *
 *  Created on: 11 Mar 2026
 *      Author: taraa
 */
#include "stm32f103xx_gpio_driver.h"

/*********************************************************************
 * @fn      		  - GPIO_PeriClockControl
 *
 * @brief             - This function enables or disables peripheral clock for the given GPIO port
 *
 * @param[in]         - base address of the gpio peripheral
 * @param[in]         - ENABLE or DISABLE macros
 * @param[in]         -
 *
 * @return            -  none
 *
 * @Note              -  none

 */
void GPIO_PeriClockControl(GPIO_RegDef_t *pGPIOx, uint8_t EnorDi) {
	if (EnorDi == ENABLE) {
		if (pGPIOx == GPIOA) {
			GPIOA_PCLK_EN();
		} else if (pGPIOx == GPIOB) {
			GPIOB_PCLK_EN();
		} else if (pGPIOx == GPIOC) {
			GPIOC_PCLK_EN();
		} else if (pGPIOx == GPIOD) {
			GPIOD_PCLK_EN();
		} else if (pGPIOx == GPIOE) {
			GPIOE_PCLK_EN();
		} else if (pGPIOx == GPIOF) {
			GPIOF_PCLK_EN();
		} else if (pGPIOx == GPIOG) {
			GPIOG_PCLK_EN();
		}
	} else if (EnorDi == DISABLE) {
		if (pGPIOx == GPIOA) {
			GPIOA_PCLK_DI();
		} else if (pGPIOx == GPIOB) {
			GPIOB_PCLK_DI();
		} else if (pGPIOx == GPIOC) {
			GPIOC_PCLK_DI();
		} else if (pGPIOx == GPIOD) {
			GPIOD_PCLK_DI();
		} else if (pGPIOx == GPIOE) {
			GPIOE_PCLK_DI();
		} else if (pGPIOx == GPIOF) {
			GPIOF_PCLK_DI();
		} else if (pGPIOx == GPIOG) {
			GPIOG_PCLK_DI();
		}
	}

}

/*********************************************************************
 * @fn      		  - GPIO_Init
 *
 * @brief             - Mode and Cnf decides about all the input and output characteristics.
 *
 * @param[in]         - GPIO_Handle_t (Pin Configuration and Port details)
 *
 * @return            - void
 *
 * @Note              - Refer Reference Manual GPIOs

 */
void GPIO_Init(GPIO_Handle_t *pGPIOHandle) {

	//1. Configure the mode & cnf of gpio pin &
	//2. Configure the speed using Mode
	//3. Configure the pupd settings
	//4. Configure the output type

	if (pGPIOHandle->GPIO_PinConfig.GPIO_PinMode >= GPIO_MODE_IT_FT) {
		//the interrupt mode
	} else {
		//non interrupt mode
		uint8_t pin = pGPIOHandle->GPIO_PinConfig.GPIO_PinNumber;
		uint32_t position;
		volatile uint32_t *configReg;
		if (pin < 8) {
			position = pin * 4;
			configReg = &pGPIOHandle->pGPIOx->CRL;
		} else {
			position = (pin - 8) * 4;
			configReg = &pGPIOHandle->pGPIOx->CRH;
		}

		uint32_t value = ((pGPIOHandle->GPIO_PinConfig.GPIO_PinCnf << 2)
				| pGPIOHandle->GPIO_PinConfig.GPIO_PinMode);
		uint32_t mask = 0xF << position;
		*configReg = (*configReg & ~mask) | (value << position);

		if (pGPIOHandle->GPIO_PinConfig.GPIO_PinMode == GPIO_MODE_IN
				&& pGPIOHandle->GPIO_PinConfig.GPIO_PinCnf == GPIO_CNF_IN_PUPD) {
			if (pGPIOHandle->GPIO_PinConfig.GPIO_PinPuPdControl
					== GPIO_PULLUP) {
				pGPIOHandle->pGPIOx->ODR |= (1
						<< pGPIOHandle->GPIO_PinConfig.GPIO_PinNumber);
			} else if (pGPIOHandle->GPIO_PinConfig.GPIO_PinPuPdControl
					== GPIO_PULLDOWN) {
				pGPIOHandle->pGPIOx->ODR &= ~(1
						<< pGPIOHandle->GPIO_PinConfig.GPIO_PinNumber);
			}
		}

	}

	//5. Configure the alt functionality (if required)

}

/*********************************************************************
 * @fn      		  - GPIO_DeInit
 *
 * @brief             - Refer APB2 peripheral reset register in Reference manual
 *
 * @param[in]         - GPIO_RegDef_t: Base Address of the GPIO Peripheral (GPIO Port A,B etc)

 *
 * @return            - void
 *
 * @Note              -

 */
void GPIO_DeInit(GPIO_RegDef_t *pGPIOx) {

	if (pGPIOx == GPIOA) {
		GPIOA_REG_RESET();
	} else if (pGPIOx == GPIOB) {
		GPIOB_REG_RESET();
	} else if (pGPIOx == GPIOC) {
		GPIOC_REG_RESET();
	} else if (pGPIOx == GPIOD) {
		GPIOD_REG_RESET();
	} else if (pGPIOx == GPIOE) {
		GPIOE_REG_RESET();
	} else if (pGPIOx == GPIOF) {
		GPIOF_REG_RESET();
	} else if (pGPIOx == GPIOG) {
		GPIOG_REG_RESET();
	}
}

/*********************************************************************
 * @fn      		  - GPIO_ReadFromInputPin
 *
 * @brief             - Reads the value from the specific input pin of the GPIO Port
 *
 * @param[in]         - Base Address of the GPIO Peripheral
 * @param[in]         - Pin Number: From 0 to 15

 *
 * @return            - 0 or 1
 *
 * @Note              - Value of the IDR shifted (corresponding pin number) times to the left
 * 						and read the leftmost value by masking

 */

uint8_t GPIO_ReadFromInputPin(GPIO_RegDef_t *pGPIOx, uint8_t PinNumber) {

	uint8_t value;

	value = (uint8_t)(pGPIOx->IDR >> PinNumber) & 00000001 ;

	return value;

}

/*********************************************************************
 * @fn      		  - GPIO_ReadFromInputPort
 *
 * @brief             - Reads the value from the specific GPIO Port
 *
 * @param[in]         - Base Address of the GPIO Peripheral
 *
 * @return            - 16 bit GPIO Port Value
 *
 * @Note              - Simply read the value of IDR

 */
uint16_t GPIO_ReadFromInputPort(GPIO_RegDef_t *pGPIOx) {

	uint16_t value;

	value = (uint16_t)(pGPIOx->IDR);

	return value;
}

/*********************************************************************
 * @fn      		  - GPIO_WriteToOutputPin
 *
 * @brief             - Set the value to the corresponding pin of GPIO Port
 *
 * @param[in]         - Base Address of the GPIO Peripheral
 * @param[in]         - Pin Number: From 0 to 15
 * @param[in]         - Value either 0 or 1
 *
 * @return            - void
 *
 * @Note              - Set the value corresponding to the pin number in ODR register

 */
void GPIO_WriteToOutputPin(GPIO_RegDef_t *pGPIOx, uint8_t PinNumber,
		uint8_t Value)
{
		if(Value == GPIO_PIN_SET)
		{
			//Write 1 to the output data register at the bit field corresponding to the pin number
			pGPIOx->ODR |= (1 << PinNumber);
		}
		else
		{
			//Write 0 to the output data register at the bit field corresponding to the pin number
			pGPIOx->ODR &= ~(1 << PinNumber);
		}

}

/*********************************************************************
 * @fn      		  - GPIO_WriteToOutputPort
 *
 * @brief             -  Set the value to the GPIO Port
 *
 * @param[in]         - Base Address of the GPIO Peripheral
 * @param[in]         - 16 bit value

 *
 * @return            - void
 *
 * @Note              - Set the 16 bit value to ODR register

 */
void GPIO_WriteToOutputPort(GPIO_RegDef_t *pGPIOx, uint16_t Value)
{
	pGPIOx->ODR = Value;
}

/*********************************************************************
 * @fn      		  - GPIO_ToggleOutputPin
 *
 * @brief             - Toggle the corresponding pin in the GPIO
 *
 * @param[in]         - Base Address of the GPIO Peripheral
 * @param[in]         - Pin Number
 *
 * @return            - void
 *
 * @Note              - Use XOR operation for toggling.
 * 						XOR with 1 → flips the bit
						XOR with 0 → leaves it unchanged

 */
void GPIO_ToggleOutputPin(GPIO_RegDef_t *pGPIOx, uint8_t PinNumber)
{
	pGPIOx->ODR ^= (1 << PinNumber);
}
/*********************************************************************
 * @fn      		  - SPI_IRQPriorityConfig
 *
 * @brief             -
 *
 * @param[in]         -
 * @param[in]         -
 * @param[in]         -
 *
 * @return            -
 *
 * @Note              -

 */
void GPIO_IRQConfig(uint8_t IRQNumber, uint8_t IRQPriority, uint8_t EnorDi) {
}
/*********************************************************************
 * @fn      		  - GPIO_IRQHandling
 *
 * @brief             -
 *
 * @param[in]         -
 * @param[in]         -
 * @param[in]         -
 *
 * @return            -
 *
 * @Note              -

 */
void GPIO_IRQHandling(uint8_t PinNumber) {
}

