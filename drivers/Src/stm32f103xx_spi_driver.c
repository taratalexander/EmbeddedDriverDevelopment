/*
 * stm32f103xx_spi_driver.c
 *
 *  Created on: 17 Jun 2026
 *      Author: Tara Alexander
 */
#include "stm32f103xx_spi_driver.h"

/*********************************************************************
 * @fn      		  - SPI_PeriClockControl
 *
 * @brief             - This function enables or disables peripheral clock for the given SPI
 *
 * @param[in]         - pSPIx : Base address of SPI peripheral
 *                      (can be SPI1, SPI2, or SPI3)
 *
 * @param[in]         -  EnorDi : ENABLE or DISABLE macro to control
 *                      the peripheral clock state
 *
 * @return            -  None
 *
 * @Note              -  Clock must be enabled before accessing SPI
 *                      registers; otherwise the peripheral will not
 *                      respond.
 *
 *                      This function internally controls RCC APB1/APB2
 *                      clock enable/disable bits depending on SPI
 *                      instance.
 */

void SPI_PeriClockControl(SPI_RegDef_t *pSPIx, uint8_t EnorDi) {
	if (EnorDi == ENABLE) {
		if (pSPIx == SPI1) {
			SPI1_PCLK_EN();
		} else if (pSPIx == SPI2) {
			SPI2_PCLK_EN();
		} else if (pSPIx == SPI3) {
			SPI3_PCLK_EN();
		}
	} else if (EnorDi == DISABLE) {
		if (pSPIx == SPI1) {
			SPI1_PCLK_DI();
		} else if (pSPIx == SPI2) {
			SPI2_PCLK_DI();
		} else if (pSPIx == SPI3) {
			SPI3_PCLK_DI();
		}
	}

}

/*********************************************************************
 * @fn      		  - SPI_Init
 *
 * @brief             - Initialises the SPI peripheral with the
 *                      configuration parameters specified in the
 *                      SPI handle structure.
 *
 * @param[in]         - pSPIHandle : Pointer to SPI handle structure
 *                      containing SPI instance and configuration
 *                      settings.
 *
 * @return            - None
 *
 * @Note              - Configures the SPI_CR1 register fields:
 *                        - Device Mode (Master/Slave)
 *                        - Bus Configuration
 *                        - Serial Clock Speed
 *                        - Data Frame Format
 *                        - Clock Polarity (CPOL)
 *                        - Clock Phase (CPHA)
 *                        - Software Slave Management (SSM)
 *
 *                      Refer to the STM32F103 Reference Manual
 *                      for detailed SPI register descriptions.

 */
void SPI_Init(SPI_Handle_t *pSPIHandle) {

	//Enable the SPI Clock

	SPI_PeriClockControl(pSPIHandle->pSPIx, ENABLE);
	// first lets configure the SPI_CR1 register
	uint32_t tempreg = 0;

	//1.Configure the device mode
	tempreg |= (pSPIHandle->SPIConfig.SPI_DeviceMode << SPI_CR1_MSTR);

	//2.Configure the bus config(Bidi type)

	if (pSPIHandle->SPIConfig.SPI_BusConfig == SPI_BUS_CONFIG_FD) {
		//BIDIMODE should be cleared
		tempreg &= ~(1 << SPI_CR1_BIDIMOOE);
	} else if (pSPIHandle->SPIConfig.SPI_BusConfig == SPI_BUS_CONFIG_HD) {
		//BIDIMODE should be set
		tempreg |= (1 << SPI_CR1_BIDIMOOE);
	} else if (pSPIHandle->SPIConfig.SPI_BusConfig
			== SPI_BUS_CONFIG_SIMPLEX_RXONLY) {
		//BIDIMODE should be cleared
		tempreg &= ~(1 << SPI_CR1_BIDIMOOE);
		//RXONLY should be set
		tempreg |= (1 << SPI_CR1_RXONLY);
	}

	//3.Configure the serial clock speed
	tempreg |= (pSPIHandle->SPIConfig.SPI_SclkSpeed << SPI_CR1_BR);

	//4. Configure the DFF
	tempreg |= (pSPIHandle->SPIConfig.SPI_DFF << SPI_CR1_DFF);

	//5. Configure the CPOL
	tempreg |= (pSPIHandle->SPIConfig.SPI_CPOL << SPI_CR1_CPOL);

	//6. Configure the CPOL
	tempreg |= (pSPIHandle->SPIConfig.SPI_CPHA << SPI_CR1_CPHA);

	//7. Configure the SSM
	tempreg |= (pSPIHandle->SPIConfig.SPI_SSM << SPI_CR1_SSM);

	pSPIHandle->pSPIx->CR1 = tempreg;
}

/*********************************************************************
 * @fn      		  - SPI_DeInit
 *
 * @brief             - Resets the selected SPI peripheral to its
 *                      default reset state by asserting and then
 *                      releasing the peripheral reset via the
 *                      RCC APB1/APB2 reset registers.
 *
 * @param[in]         -  pSPIx : Base address of the SPI peripheral
 *                      (can be SPI1, SPI2, or SPI3)

 *
 * @return            - None
 *
 * @Note              - This function performs a software reset of
 *                      the SPI peripheral by toggling the reset
 *                      bit in RCC registers.
 *
 *                      Refer to STM32F103 Reference Manual:
 *                      - APB2 peripheral reset register (SPI1)
 *                      - APB1 peripheral reset register (SPI2, SPI3)
 *
 *                      After reset, all SPI registers are restored
 *                      to their default hardware reset values.

 */
void SPI_DeInit(SPI_RegDef_t *pSPIx) {

	if (pSPIx == SPI1) {
		SPI1_REG_RESET();
	} else if (pSPIx == SPI2) {
		SPI2_REG_RESET();
	} else if (pSPIx == SPI3) {
		SPI3_REG_RESET();
	}
}

uint8_t SPI_GetFlagStatus(SPI_RegDef_t *pSPIx, uint32_t flag) {
	return (pSPIx->SR & flag) ? SPI_FLAG_SET : SPI_FLAG_RESET;
}
/*********************************************************************
 * @fn      		  - SPI_SendData
 *
 * @brief             -
 *
 * @param[in]         - pSPIx: Base Address of the SPI Peripheral
 * @param[in]         - pTxBuffer: Pointer to the transmit data buffer.
 * @param[in]         - len: Number of bytes to transmit
 *
 * @return            - None
 *
 * @Note              - This is a blocking (polling) API. The function waits for
 *          			the TXE (Transmit Buffer Empty) flag before loading each
 *          			data frame into the SPI Data Register (DR). Supports both
 *          			8-bit and 16-bit data frame formats.

 */

void SPI_SendData(SPI_RegDef_t *pSPIx, uint8_t *pTxBuffer, uint32_t len) {

	while (len > 0) {
		//1. Wait until TXE flag is set
		while (SPI_GetFlagStatus(pSPIx, SPI_TXE_FLAG) == SPI_FLAG_RESET)
			;

		//2.Check the DFF bit in CR1
		if (pSPIx->CR1 & 1 << SPI_CR1_DFF) {
			//16 bit data
			//1.Load data to the DR reg.
			pSPIx->DR = *((uint16_t*) pTxBuffer);
			len--;
			len--;
			(uint16_t*) pTxBuffer++;
		} else {
			//8 bit data
			pSPIx->DR = *(pTxBuffer);
			len--;
			pTxBuffer++;
		}

	}
}

/*********************************************************************
 * @fn      		  - SPI_ReceiveData
 *
 * @brief             - Reads the value from the specific SPI
 *
 * @param[in]         - pSPIx: Base Address of the SPI Peripheral
 * @param[in]         - pRxBuffer: Pointer to the receive data buffer.
 * @param[in]         - len: Number of bytes to transmit
 *
 * @return            - None
 *
 * @Note              - This is a blocking (polling) API. The function waits for
 *          			the RXNE (Receive Buffer Empty) flag before loading each
 *          			data frame into the SPI Data Register (DR). Supports both
 *          			8-bit and 16-bit data frame formats.

 */
void SPI_ReceiveData(SPI_RegDef_t *pSPIx, uint8_t *pRxBuffer, uint32_t len) {

	while (len > 0) {
		//1. Wait until RXNE flag is set
		while (SPI_GetFlagStatus(pSPIx, SPI_RXNE_FLAG) == SPI_FLAG_RESET);

		//2.Check the DFF bit in CR1
		if (pSPIx->CR1 & 1 << SPI_CR1_DFF) {
			//16 bit data
			//1.Load data from the DR reg to RxBuffer.
			*((uint16_t*) pRxBuffer) = pSPIx->DR;
			len--;
			len--;
			(uint16_t*) pRxBuffer++;
		} else {
			//8 bit data
			*(pRxBuffer) = pSPIx->DR;
			len--;
			pRxBuffer++;
		}

	}
}

/*********************************************************************
 * @fn      		  - SPI_IRQInterruptConfig
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
void SPI_IRQInterruptConfig(uint8_t IRQNumber, uint8_t EnorDi) {
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
 * @fn      		  - SPI_IRQPriorityConfig
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
void SPI_IRQPriorityConfig(uint8_t IRQNumber, uint8_t IRQPriority) {

	//1.Find out the IPR register
	uint8_t iprx = (IRQNumber / 4);
	uint8_t iprx_section = IRQNumber % 4;
	uint8_t shift_amount = (8 * iprx_section) + (8 - NO_PR_BITS_IMPLEMENTED);
	*(NVIC_PR_BASEADDR + iprx) &= ~(0xF << shift_amount);
	*(NVIC_PR_BASEADDR + (iprx)) |= (IRQPriority << shift_amount);

}
/*********************************************************************
 * @fn      		  - SPI_IRQHandling
 *
 * @brief             -  Checks whether the interrupt is generated from the given SPI.
 *
 * @param[in]         -  SPI Handle
 *
 * @return            - void
 *
 * @Note              - If the pending bit is set in the EXTI Pending Register (PR),
 * 						the function clears the pending bit by writing 1 to it.
 *
 * 						STM32 EXTI pending bits are cleared by writing 1
 * 						to the corresponding bit position.

 */
void SPI_IRQHandling(SPI_Handle_t *pSPIHandle) {

}

/*********************************************************************
 * @fn                - SPI_PeripheralControl
 *
 * @brief             - Enables or disables the SPI peripheral by
 *                      setting or clearing the SPE (SPI Enable) bit
 *                      in the CR1 register.
 *
 * @param[in]         - pSPIx : Base address of the SPI peripheral
 *
 * @param[in]         - EnorDi : ENABLE or DISABLE
 *                               ENABLE  -> Sets the SPE bit
 *                               DISABLE -> Clears the SPE bit
 *
 * @return            - None
 *
 * @Note              - The SPI peripheral must be enabled before any
 *                      data transmission or reception can occur.
 *                      It is recommended to disable the peripheral
 *                      before changing SPI configuration settings.
 *
 *********************************************************************/
void SPI_PeripheralControl(SPI_RegDef_t *pSPIx, uint8_t EnorDi) {
	if (EnorDi == ENABLE) {
		pSPIx->CR1 |= (1 << SPI_CR1_SPE);
	} else {
		pSPIx->CR1 &= ~(1 << SPI_CR1_SPE);
	}
}

/*********************************************************************
 * @fn                - SPI_SSIConfig
 *
 * @brief             - Configures the SSI (Internal Slave Select) bit
 *                      in the SPI CR1 register.
 *
 * @param[in]         - pSPIx : Base address of the SPI peripheral
 *
 * @param[in]         - EnorDi : ENABLE or DISABLE
 *                               ENABLE  -> Sets the SSI bit
 *                               DISABLE -> Clears the SSI bit
 *
 * @return            - None
 *
 * @Note              - The SSI bit is used when Software Slave
 *                      Management (SSM) is enabled. Setting SSI high
 *                      prevents a MODF (Mode Fault) error in Master
 *                      mode when NSS is managed by software.
 *
 *********************************************************************/
void SPI_SSIConfig(SPI_RegDef_t *pSPIx, uint8_t EnorDi) {
	if (EnorDi == ENABLE) {
		pSPIx->CR1 |= (1 << SPI_CR1_SSI);
	} else {
		pSPIx->CR1 &= ~(1 << SPI_CR1_SSI);
	}
}

/*********************************************************************
 * @fn                - SPI_SSIConfig
 *
 * @brief             - Configures the SSOE (Slave Select Output Enable) bit
 *                      in the SPI CR2 register.
 *
 * @param[in]         - pSPIx : Base address of the SPI peripheral
 *
 * @param[in]         - EnorDi : ENABLE or DISABLE
 *                               ENABLE  -> Sets the SSOE bit
 *                               DISABLE -> Clears the SSOE bit
 *
 * @return            - None
 *
 * @Note              - The SSOE bit is used when the device operates in master mode and also
 * 						hardware slave select management is used (SSM =0)
 *                      NSS output enabled (SSM = 0, SSOE = 1):
 *                      The NSS signal is driven low when the master starts the communication
 *                      and is kept low until the SPI is disabled
 *                      NSS output disabled (SSM = 0, SSOE = 0):
 *                      This configuration allows multi-master capability for devices operating in master mode
 *
 *********************************************************************/
void SPI_SSOEConfig(SPI_RegDef_t *pSPIx, uint8_t EnorDi) {
	if (EnorDi == ENABLE) {
		pSPIx->CR2 |= (1 << SPI_CR2_SSOE);
	} else {
		pSPIx->CR2 &= ~(1 << SPI_CR2_SSOE);
	}
}
