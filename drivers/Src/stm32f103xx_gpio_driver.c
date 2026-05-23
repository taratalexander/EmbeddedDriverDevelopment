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
	uint8_t pin = pGPIOHandle->GPIO_PinConfig.GPIO_PinNumber;

	if (pGPIOHandle->GPIO_PinConfig.GPIO_PinMode >= GPIO_MODE_IT_FT) {
		//the interrupt mode

		if (pGPIOHandle->GPIO_PinConfig.GPIO_PinMode == GPIO_MODE_IT_FT) {
			//1.Configure the FTSR
			EXTI->FTSR |= (1 << pin);
			//1.Clear corresponding RTSR
			EXTI->RTSR &= ~(1 << pin);

		} else if (pGPIOHandle->GPIO_PinConfig.GPIO_PinMode == GPIO_MODE_IT_RT) {
			//1.Configure the RTSR
			EXTI->RTSR |= (1 << pin);
			//1.Clear corresponding FTSR
			EXTI->FTSR &= ~(1 << pin);
		} else if (pGPIOHandle->GPIO_PinConfig.GPIO_PinMode == GPIO_MODE_IT_RFT) {
			//1.Configure both the RTSR and FTSR

			EXTI->FTSR |= (1 << pin);
			EXTI->RTSR |= (1 << pin);
		}
		//2. Configure the GPIO port selection in AFIO_EXTICR
		/*
		 * Configure GPIO port source for a particular EXTI line
		 *
		 * temp1 -> Selects EXTICR register index
		 *          EXTICR[0] : EXTI0  - EXTI3
		 *          EXTICR[1] : EXTI4  - EXTI7
		 *          EXTICR[2] : EXTI8  - EXTI11
		 *          EXTICR[3] : EXTI12 - EXTI15
		 *
		 * temp2 -> Calculates bit position inside selected EXTICR register
		 *          Each EXTI line occupies 4 bits
		 *
		 * Clear existing port selection and configure new GPIO port
		 * for the required EXTI line.
		 */
		uint8_t temp1 = pin / 4; 		 			// Select EXTICR register
		uint8_t temp2 = (pin % 4) * 4;				// Calculate bit position

		/*
		 * portCode -> Encoded GPIO port value used to map
		 *             a GPIO port to the corresponding EXTI line
		 *
		 * GPIOA = 0x0
		 * GPIOB = 0x1
		 * GPIOC = 0x2
		 * GPIOD = 0x3
		 */
		uint8_t portcode = GPIO_BASEADDR_TO_CODE(pGPIOHandle->pGPIOx);

		AFIO->EXTICR[temp1] &= ~(0xF << temp2); 	// Clear existing mapping
		AFIO->EXTICR[temp1] |= (portcode << temp2); // Set new GPIO port mapping

		//3. Enable the EXTI interrupt delivery using IMR
		EXTI->IMR |= (1 << pGPIOHandle->GPIO_PinConfig.GPIO_PinNumber);

	} else {
		//non interrupt mode
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

	value = (uint8_t) (pGPIOx->IDR >> PinNumber) & 00000001;

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

	value = (uint16_t) (pGPIOx->IDR);

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
		uint8_t Value) {
	if (Value == GPIO_PIN_SET) {
		//Write 1 to the output data register at the bit field corresponding to the pin number
		pGPIOx->ODR |= (1 << PinNumber);
	} else {
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
void GPIO_WriteToOutputPort(GPIO_RegDef_t *pGPIOx, uint16_t Value) {
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
void GPIO_ToggleOutputPin(GPIO_RegDef_t *pGPIOx, uint8_t PinNumber) {
	pGPIOx->ODR ^= (1 << PinNumber);
}
/*********************************************************************
 * @fn      		  - SPI_IRQPriorityConfig
 *
 * @brief             - Configuring the interrupt from processor side (Cortex M3 Generic User Guide )
 *
 * @param[in]         - IRQ number: unique number assigned to each interrupt source inside the NVIC
 * @param[in]         - EnorDi : Enable or Disable the interrupt
 *
 * @return            - void
 *
 * @Note              - This function enables/disables the interrupt in NVIC and configures its priority.

 */
void GPIO_IRQConfig(uint8_t IRQNumber,uint8_t EnorDi) {
	if (EnorDi == ENABLE) {
		if (IRQNumber <= 31) {

			*NVIC_ISER0 |= (1 << IRQNumber);
			// program ISER0
		} else if (IRQNumber >= 32 && IRQNumber < 63) {
			// program ISER1
			*NVIC_ISER1 |= (1 << (IRQNumber % 32));
		} else if (IRQNumber >= 64 && IRQNumber < 96) {
			// program ISER2
			*NVIC_ISER2 |= (1 << (IRQNumber % 64));
		}
	} else {
		if (IRQNumber <= 31) {
			// program ICER0
			*NVIC_ICER0 |= (1 << IRQNumber);
		} else if (IRQNumber >= 32 && IRQNumber < 63) {
			// program ICER1
			*NVIC_ICER1 |= (1 << (IRQNumber % 32));
		} else if (IRQNumber >= 64 && IRQNumber < 96) {
			// program ICER2
			*NVIC_ICER2 |= (1 << (IRQNumber % 64));
		}
	}

}
/*********************************************************************
 * @fn      		  - GPIO_IRQInterruptConfig
 *
 * @brief             - Configures the priority of a given IRQ (Interrupt Request)
 * 						using the NVIC Interrupt Priority Registers (IPR).
 *
 * @param[in]         - IRQNumber    : IRQ number whose priority must be configured
 * @param[in]         - IRQPriority  : Priority value to be assigned
 *
 * @return            - void
 * Working:
 *  1. Each NVIC IPR register is 32 bits wide.
 *  2. One IPR register contains priority fields for 4 IRQs.
 *  3. IRQNumber/4 determines which IPR register to use.
 *  4. IRQNumber%4 determines the section inside that register.
 *  5. The priority value is shifted to the correct byte position
 *     and written into the corresponding priority field.
 *
 * Formula:
 *  IPR register index  = IRQNumber / 4
 *  Section within IPR  = IRQNumber % 4
 *
 * Example:
 *  IRQNumber = 6
 *      -> Stored in IPR1
 *      -> Section = 2
 *
 * @Note              -  NVIC priority registers are byte-addressable.
 *  					Lower priority value means higher interrupt priority.

 */
void GPIO_IRQInterruptConfig(uint8_t IRQNumber,uint8_t IRQPriority) {

	//1.Find out the IPR register
	uint8_t iprx = (IRQNumber/4)*4;
	uint8_t iprx_section = IRQNumber%4;
	uint8_t shift_amount = (8 * iprx_section) + (8- NO_PR_BITS_IMPLEMENTED);
	*(NVIC_PR_BASEADDR + iprx) |=(IRQPriority << shift_amount);

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

	//clear the exti pr register corresponding to the pin number
	if( EXTI->PR & (1<< PinNumber) ){
		//clear
		EXTI->PR = (1<<PinNumber);
	}
}

