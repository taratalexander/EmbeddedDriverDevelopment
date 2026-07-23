/*
 * stm32f103xx_spi_driver.c
 *
 *  Created on: 17 Jun 2026
 *      Author: Tara Alexander
 */
#include "stm32f103xx_spi_driver.h"

static void spi_txe_interuppt_handle(SPI_Handle_t *pSPIHandle);
static void spi_rxne_interuppt_handle(SPI_Handle_t *pSPIHandle);
static void spi_ovr_err_interuppt_handle(SPI_Handle_t *pSPIHandle);

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
		while (SPI_GetFlagStatus(pSPIx, SPI_RXNE_FLAG) == SPI_FLAG_RESET)
			;

		//2.Check the DFF bit in CR1
		if (pSPIx->CR1 & 1 << SPI_CR1_DFF) {
			//16 bit data
			//1.Load data from the DR reg to RxBuffer.
			*((uint16_t*) pRxBuffer) = pSPIx->DR;
			len--;
			len--;
			pRxBuffer += 2;
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
 * @Note              - This function identifies the source of the SPI
 *                      interrupt by checking the corresponding status
 *                      flags and interrupt enable bits. Based on the
 *                      interrupt source, it invokes the appropriate
 *                      interrupt service routine to handle:
 *                        - TXE (Transmit Buffer Empty)
 *                        - RXNE (Receive Buffer Not Empty)
 *                        - OVR (Overrun Error)
 *
 *                      It should be called from the SPIx interrupt
 *                      service routine (ISR) corresponding to the SPI
 *                      peripheral (e.g., SPI1_IRQHandler(),
 *                      SPI2_IRQHandler(), etc.).
 *

 */
void SPI_IRQHandling(SPI_Handle_t *pSPIHandle) {

	uint8_t temp1, temp2;
	//First lets check for TXE
	temp1 = pSPIHandle->pSPIx->SR & (1 << SPI_SR_TXE);
	temp2 = pSPIHandle->pSPIx->CR2 & (1 << SPI_CR2_TXEIE);

	if (temp1 && temp2) {
		//Handle TXE
		spi_txe_interuppt_handle(pSPIHandle);
	}

	// check for RXNE
	temp1 = pSPIHandle->pSPIx->SR & (1 << SPI_SR_RXNE);
	temp2 = pSPIHandle->pSPIx->CR2 & (1 << SPI_CR2_RXNEIE);

	if (temp1 && temp2) {
		//Handle RXNE
		spi_rxne_interuppt_handle(pSPIHandle);
	}

	// check for OVR flag
	temp1 = pSPIHandle->pSPIx->SR & (1 << SPI_SR_OVR);
	temp2 = pSPIHandle->pSPIx->CR2 & (1 << SPI_CR2_ERRIE);

	if (temp1 && temp2) {
		//Handle RXNE
		spi_ovr_err_interuppt_handle(pSPIHandle);
	}

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

/*********************************************************************
 * @fn                - SPI_SendDataIT
 *
 * @brief             - Initiates interrupt-based (non-blocking) SPI data transmission.
 *
 * @param[in]         - pSPIHandle : Pointer to the SPI handle structure
 *
 * @param[in]         - pTxBuffer : Pointer to the transmit data buffer
 *
 * @param[in]         - len : Number of bytes to transmit
 *
 * @return            - Current SPI transmission state
 *                      SPI_READY      : Transmission successfully initiated
 *                      SPI_BUSY_IN_TX : SPI peripheral is already busy transmitting
 *
 * @Note              - This function only initializes the interrupt-driven
 *                      transmission process. It stores the transmit buffer
 *                      pointer and length in the SPI handle, marks the SPI
 *                      peripheral as busy, and enables the TXE interrupt.
 *                      The actual byte transmission is performed by the
 *                      SPI interrupt handler whenever the TXE flag is set.
 *
 *********************************************************************/
uint8_t SPI_SendDataIT(SPI_Handle_t *pSPIHandle, uint8_t *pTxBuffer,
		uint32_t len) {
	uint8_t state = pSPIHandle->TxState;
	if (state != SPI_BUSY_IN_TX) {
		//1.Save the Tx buffer address and Len in some global variable
		pSPIHandle->pTxBuffer = pTxBuffer;
		pSPIHandle->TxLen = len;

		//2. Mark the SPI state as busy in transmission to avoid using the same SPI peripheral for other usage
		pSPIHandle->TxState = SPI_BUSY_IN_TX;

		//3. Enable the TXEIE Control Bit to get interrupt whenever TXE flag is set in SR
		while (SPI_GetFlagStatus(pSPIHandle->pSPIx, SPI_TXE_FLAG)
				== SPI_FLAG_RESET)
			;

		pSPIHandle->pSPIx->CR2 |= (1 << SPI_CR2_TXEIE);
	}
	return state;

}
/*********************************************************************
 * @fn                - SPI_ReceiveDataIT
 *
 * @brief             - Initiates interrupt-based (non-blocking) SPI data reception.
 *
 * @param[in]         - pSPIHandle : Pointer to the SPI handle structure
 *
 * @param[in]         - pRxBuffer : Pointer to the receive data buffer
 *
 * @param[in]         - len : Number of bytes to receive
 *
 * @return            - Current SPI reception state
 *                      SPI_READY      : Reception successfully initiated
 *                      SPI_BUSY_IN_RX : SPI peripheral is already busy receiving
 *
 * @Note              - This function only initializes the interrupt-driven
 *                      reception process. It stores the receive buffer
 *                      pointer and length in the SPI handle, marks the SPI
 *                      peripheral as busy, and enables the RXNE interrupt.
 *                      The actual data reception is performed by the
 *                      SPI interrupt handler whenever the RXNE flag is set.
 *
 *********************************************************************/

uint8_t SPI_ReceiveDataIT(SPI_Handle_t *pSPIHandle, uint8_t *pRxBuffer,
		uint32_t len) {
	uint8_t state = pSPIHandle->RxState;
	if (state != SPI_BUSY_IN_RX) {
		//1.Save the Rx buffer address and Len in some global variable
		pSPIHandle->pRxBuffer = pRxBuffer;
		pSPIHandle->RxLen = len;

		//2. Mark the SPI state as busy in transmission to avoid using the same SPI peripheral for other usage
		pSPIHandle->RxState = SPI_BUSY_IN_RX;

		//3. Enable the RXNEIE Control Bit to get interrupt whenever TXE flag is set in SR
		while (SPI_GetFlagStatus(pSPIHandle->pSPIx, SPI_RXNE_FLAG)
				== SPI_FLAG_RESET)
			;

		pSPIHandle->pSPIx->CR2 |= (1 << SPI_CR2_RXNEIE);
	}

	return state;

}

static void spi_txe_interuppt_handle(SPI_Handle_t *pSPIHandle) {
	//Check the DFF bit in CR1
	if (pSPIHandle->pSPIx->CR1 & 1 << SPI_CR1_DFF) {
		//16 bit data
		//1.Load data to the DR reg.
		pSPIHandle->pSPIx->DR = *((uint16_t*) pSPIHandle->pTxBuffer);
		pSPIHandle->TxLen--;
		pSPIHandle->TxLen--;
		(uint16_t*) pSPIHandle->pTxBuffer++;
	} else {
		//8 bit data
		pSPIHandle->pSPIx->DR = *(pSPIHandle->pTxBuffer);
		pSPIHandle->TxLen--;
		pSPIHandle->pTxBuffer++;
	}
	if (!pSPIHandle->TxLen) {
		//TxLen is zero, so close the SPI transmission and inform the application that Tx is over
		//This prevents interrupts from setting of TXE flag
		SPI_CloseTransmission(pSPIHandle);
		SPI_ApplicationEventCallback(pSPIHandle, SPI_EVENT_TX_CMPLT);
	}

}
static void spi_rxne_interuppt_handle(SPI_Handle_t *pSPIHandle) {

	//Check the DFF bit in CR1
	if (pSPIHandle->pSPIx->CR1 & 1 << SPI_CR1_DFF) {
		//16 bit data
		//1.Load data from the DR reg to RxBuffer.
		*((uint16_t*) pSPIHandle->pRxBuffer) = (uint16_t) pSPIHandle->pSPIx->DR;
		pSPIHandle->RxLen--;
		pSPIHandle->RxLen--;
		pSPIHandle->pRxBuffer += 2;
	} else {
		//8 bit data
		*(pSPIHandle->pRxBuffer) = pSPIHandle->pSPIx->DR;
		pSPIHandle->RxLen--;
		pSPIHandle->pRxBuffer++;
	}
	if (!pSPIHandle->RxLen) {
		//TxLen is zero, so close the SPI transmission and inform the application that Tx is over
		//This prevents interrupts from setting of TXE flag
		SPI_CloseReception(pSPIHandle);
		SPI_ApplicationEventCallback(pSPIHandle, SPI_EVENT_RX_CMPLT);
	}
}
static void spi_ovr_err_interuppt_handle(SPI_Handle_t *pSPIHandle) {
	//1.Clear the OVR flag :
	/*
	 * Clearing the OVR bit is done by a read from the SPI_DR register followed by a read access
	 to the SPI_SR register.
	 */
	uint8_t temp;
	if (pSPIHandle->TxState != SPI_BUSY_IN_TX) {
		temp = pSPIHandle->pSPIx->DR;
		temp = pSPIHandle->pSPIx->SR;
	}
		(void)temp;
	//2. Inform the application
		SPI_ApplicationEventCallback(pSPIHandle, SPI_EVENT_OVR_ERR);

}
void SPI_ClearOVRFlag(SPI_RegDef_t *pSPIx)
{
	uint8_t temp;
	temp =pSPIx->DR;
	temp = pSPIx->SR;
	(void)temp;
	}
void SPI_CloseTransmission(SPI_Handle_t *pSPIHandle) {
	pSPIHandle->pSPIx->CR2 &= ~(1 << SPI_CR2_TXEIE);
	pSPIHandle->pTxBuffer = NULL;
	pSPIHandle->TxLen = 0;
	pSPIHandle->TxState = SPI_READY;
	SPI_ApplicationEventCallback(pSPIHandle, SPI_EVENT_TX_CMPLT);
}
void SPI_CloseReception(SPI_Handle_t *pSPIHandle) {
	pSPIHandle->pSPIx->CR2 &= ~(1 << SPI_CR2_RXNEIE);
	pSPIHandle->pRxBuffer = NULL;
	pSPIHandle->RxLen = 0;
	pSPIHandle->RxState = SPI_READY;
	SPI_ApplicationEventCallback(pSPIHandle, SPI_EVENT_RX_CMPLT);
}
__weak void SPI_ApplicationEventCallback(SPI_Handle_t *pSPIHandle,uint8_t AppEv)
{
	//This is a weak implementation . The user application may override this function
}
