/*
 *   stm32f103xx_i2c_driver.c
 *
 *  Created on		: 24 Jul 2026
 *  Author			: Tara Alexander
 */

#include "stm32f103xx_i2c_driver.h"

uint16_t AHB_PreScaler[8] = { 2, 4, 8, 16, 64, 128, 256, 512 };
uint16_t APB1_PreScaler[4] = { 2, 4, 8, 16 };

#define WRITE		0
#define READ 		1


static void I2C_GenerateStartCondition(I2C_RegDef_t *pI2Cx);
static void I2C_ExecuteAddressPhase(uint8_t slaveAddr, I2C_RegDef_t *pI2Cx,
		uint8_t ReadOrWrite);
static void I2C_ClearAddrFlag(I2C_Handle_t *pI2CHandle);

static void I2C_MasterHandleRXNEInterrupt(I2C_Handle_t *pI2CHandle);
static void I2CMasterHandleTXEInterrupt(I2C_Handle_t *pI2CHandle);
/*********************************************************************
 * @fn      		  - I2C_GenerateStartCondition
 *
 * @brief             - This function generates a Start condition on
 *                      the I2C bus by setting the START bit in CR1
 *
 * @param[in]         - pI2Cx : Base address of I2C peripheral
 *                      (can be I2C1 or I2C2)
 *
 * @return            -  None
 *
 * @Note              -  Setting the START bit puts the I2C peripheral
 *                      into Master mode and generates a Start condition
 *                      once the bus is free.
 *
 *                      In Master mode, this bit must be cleared by
 *                      hardware after the Start condition is sent and
 *                      SB (Start Bit) flag is set in SR1; software
 *                      should not clear it manually.
 *
 *                      Application must wait for the SB flag to be
 *                      set (in SR1) before proceeding to send the
 *                      slave address.
 */
static void I2C_GenerateStartCondition(I2C_RegDef_t *pI2Cx) {
	pI2Cx->CR1 |= (1 << I2C_CR1_START);
}
/*********************************************************************
 * @fn      		  - I2C_ExecuteAddressPhase
 *
 * @brief             - This function sends the slave address along
 *                      with the read/write bit onto the I2C bus during
 *                      the address phase of communication
 *
 * @param[in]         - slaveAddr : 7-bit address of the slave device
 *
 * @param[in]         - pI2Cx : Base address of I2C peripheral
 *                      (can be I2C1 or I2C2)
 *
 * @return            -  None
 *
 * @Note              -  The slave address is left-shifted by 1 bit to
 *                      make room for the R/W bit in the LSB position.
 *
 *                      The LSB is cleared (set to 0) to indicate a
 *                      Write operation; for a Read operation this bit
 *                      should be set to 1 instead.
 *
 *                      This function should be called only after the
 *                      Start condition has been generated and the SB
 *                      flag is set.
 */

static void I2C_ExecuteAddressPhase(uint8_t slaveAddr, I2C_RegDef_t *pI2Cx,
		uint8_t ReadOrWrite) {

	slaveAddr = slaveAddr << 1;
	if (ReadOrWrite == WRITE) {
		slaveAddr &= ~(1);
	} else if (ReadOrWrite == READ) {
		slaveAddr |= 1;
	}
	pI2Cx->DR = slaveAddr;
}
/*********************************************************************
 * @fn				- I2C_ClearAddrFlag
 *
 * @brief			- This function clears the ADDR flag in SR1, which is
 *					  set when address matching completes in slave mode,
 *					  or when address is successfully sent in master mode.
 *					  ADDR is cleared by the hardware-mandated sequence of
 *					  reading SR1 followed by SR2.
 *
 * @param[in]		- pI2CHandle : Pointer to the I2C handle structure
 *					  containing the base address of the I2Cx peripheral
 *					  and application state information.
 *
 * @return			- none
 *
 * @Note			- In Master Receive mode, if only 1 byte is to be
 *					  received (RxSize == 1), the ACK bit must be disabled
 *					  BEFORE clearing the ADDR flag. This is required per
 *					  the reference manual's single-byte reception procedure,
 *					  to prevent the hardware from generating an ACK for an
 *					  unwanted second byte.
 *
 *					  This function does NOT handle the 2-byte reception
 *					  case (NACK + POS bit setup), which must be configured
 *					  by the caller before invoking this function.
 *
 *					  This function must be called every time the ADDR
 *					  interrupt/event fires, regardless of device mode,
 *					  otherwise the SCL line will remain stretched (master)
 *					  or communication will stall (slave).
 *
 *********************************************************************/
static void I2C_ClearAddrFlag(I2C_Handle_t *pI2CHandle) {

	//Check for device mode
	if (pI2CHandle->pI2Cx->SR2 & (1 << I2C_SR2_MSL)) {
		//Master mode
		if (pI2CHandle->TxRxState == I2C_BUSY_IN_RX) {
			if (pI2CHandle->RxSize == 1) {
				// First disable the ack
				I2C_ManageAcking(pI2CHandle->pI2Cx, I2C_DISABLE_ACK);
			}

			// Clear the ADDR flag
			(void) pI2CHandle->pI2Cx->SR1;
			(void) pI2CHandle->pI2Cx->SR2;

		} else {
			(void) pI2CHandle->pI2Cx->SR1;
			(void) pI2CHandle->pI2Cx->SR2;
		}
	} else {
		//Slave mode
		(void) pI2CHandle->pI2Cx->SR1;
		(void) pI2CHandle->pI2Cx->SR2;
	}

}
/*********************************************************************
 * @fn      		  - I2C_GenerateStopCondition
 *
 * @brief             - This function generates a Stop condition on
 *                      the I2C bus by setting the STOP bit in CR1
 *
 * @param[in]         - pI2Cx : Base address of I2C peripheral
 *                      (can be I2C1 or I2C2)
 *
 * @return            -  None
 *
 * @Note              -  Setting the STOP bit releases the bus and
 *                      ends the current I2C transaction; it is
 *                      automatically cleared by hardware once the
 *                      Stop condition is generated.
 *
 *                      This should be called only after the required
 *                      data transfer is complete, typically after
 *                      confirming TXE and BTF (or the last byte in
 *                      RXNE) flags are set as applicable.
 */
void I2C_GenerateStopCondition(I2C_RegDef_t *pI2Cx) {
	pI2Cx->CR1 |= (1 << I2C_CR1_STOP);
}

/*********************************************************************
 * @fn      		  - I2C_PeriClockControl
 *
 * @brief             - This function enables or disables peripheral clock for the given I2C
 *
 * @param[in]         - pI2Cx : Base address of I2C peripheral
 *                      (can be I2C1 or I2C2
 *
 * @param[in]         -  EnorDi : ENABLE or DISABLE macro to control
 *                      the peripheral clock state
 *
 * @return            -  None
 *
 * @Note              -  Clock must be enabled before accessing I2C
 *                      registers; otherwise the peripheral will not
 *                      respond.
 *
 *                      This function internally controls RCC APB1/APB2
 *                      clock enable/disable bits depending on I2C
 *                      instance.
 */

void I2C_PeriClockControl(I2C_RegDef_t *pI2Cx, uint8_t EnorDi) {
	if (EnorDi == ENABLE) {
		if (pI2Cx == I2C1) {
			I2C1_PCLK_EN();
		} else if (pI2Cx == I2C2) {
			I2C2_PCLK_EN();
		}
	} else if (EnorDi == DISABLE) {
		if (pI2Cx == I2C1) {
			I2C1_PCLK_DI();
		} else if (pI2Cx == I2C2) {
			I2C2_PCLK_DI();
		}
	}

}

uint32_t RCC_GetPLLOutputClock(void) {
	uint32_t pllClk;
	uint8_t pllSrc, pllXtPre, pllMulBits, pllMul;
	uint32_t inputClk;

	// Bit 16: PLL entry clock source
	pllSrc = (RCC->CFGR >> 16) & 0x1;

	// Bit 17: HSE divider for PLL entry (only matters if PLLSRC = HSE)
	pllXtPre = (RCC->CFGR >> 17) & 0x1;

	// Bits 21:18: PLL multiplication factor
	pllMulBits = (RCC->CFGR >> 18) & 0xF;

	// PLLMUL: 0000 = x2, 0001 = x3, ... 1110 = x16, 1111 = x16 (per errata, not x15)
	if (pllMulBits == 15) {
		pllMul = 16;
	} else {
		pllMul = pllMulBits + 2;
	}

	// Determine PLL input clock
	if (pllSrc == 0) {
		// HSI/2 is always used when PLLSRC = 0
		inputClk = 8000000 / 2;   // = 4,000,000
	} else {
		// PLLSRC = 1 → HSE selected, optionally divided by 2
		if (pllXtPre == 0) {
			inputClk = 8000000;       // HSE not divided
		} else {
			inputClk = 8000000 / 2;   // HSE divided by 2
		}
	}

	pllClk = inputClk * pllMul;

	return pllClk;
}

/*********************************************************************
 * @fn      		  - RCC_GetPClk1Value
 *
 * @brief             - This function calculates and returns the current
 *                       APB1 peripheral clock (PCLK1) frequency by reading
 *                       the system clock source, AHB prescaler, and APB1
 *                       prescaler from the RCC_CFGR register
 *
 * @param[in]         - None
 *
 * @param[in]         - None
 *
 * @return            - uint32_t : PCLK1 frequency value in Hz
 *
 * @Note              - Assumes HSE = 8MHz where applicable; if HSE differs,
 *                       update the hardcoded value accordingly
 *
 */
uint32_t RCC_GetPClk1Value(void) {
	uint32_t pclk1, SysClk;

	uint8_t clksrc, temp, ahbpre, apb1pre;

	clksrc = (RCC->CFGR >> 2) & 0x3;

	if (clksrc == 0) {
		SysClk = 8000000;
	} else if (clksrc == 1) {
		SysClk = 8000000;
	} else if (clksrc == 2) {
		SysClk = RCC_GetPLLOutputClock();
	}

	temp = (RCC->CFGR >> 4) & 0xF;

	if (temp < 8) {
		ahbpre = 1;
	} else {
		ahbpre = AHB_PreScaler[temp - 8];
	}

	temp = (RCC->CFGR >> 8) & 0x7;

	if (temp < 4) {
		apb1pre = 1;
	} else {
		apb1pre = APB1_PreScaler[temp - 4];
	}

	pclk1 = (SysClk / ahbpre) / apb1pre;

	return pclk1;
}

/*********************************************************************
 * @fn      		  - I2C_Init
 *
 * @brief             - Initialises the I2C peripheral with the
 *                      configuration parameters specified in the
 *                      I2C handle structure.
 *
 * @param[in]         - pI2CHandle : Pointer to I2C handle structure
 *                      containing I2C instance and configuration
 *                      settings.
 *
 * @return            - None
 *
 * @Note              - Configures the I2C register fields:
 *                        - ACK control bit
 *                        - FREQ field of CR2
 *                        - Slave Address
 *                        - CCR calculations
 *
 *                      Refer to the STM32F103 Reference Manual
 *                      for detailed I2C register descriptions.

 */
void I2C_Init(I2C_Handle_t *pI2CHandle) {

	uint32_t temp_reg;

	//Enable the clock for I2Cx peripheral

	I2C_PeriClockControl(pI2CHandle->pI2Cx, ENABLE);

	//Enable ACKing
	temp_reg = 0;
	temp_reg |= pI2CHandle->I2CConfig.I2C_ACKControl << 10;
	pI2CHandle->pI2Cx->CR1 = temp_reg;

	//Configure the FREQ field of CR2

	temp_reg = 0;
	temp_reg |= (RCC_GetPClk1Value() / 1000000U); //Remove the MHz;need only the value

	pI2CHandle->pI2Cx->CR2 = (temp_reg & 0x3F); //Masking the last 6 bits to write the FREQ value

	//device own address

	temp_reg = 0;
	temp_reg |= pI2CHandle->I2CConfig.I2C_DeviceAddress << 1;
	// bit 14 of OAR1 "should always be kept at 1 by software."
	temp_reg |= (1 << 14);
	pI2CHandle->pI2Cx->OAR1 = temp_reg;

	//CCR calculations
	uint16_t ccr_value = 0;
	temp_reg = 0;
	if (pI2CHandle->I2CConfig.I2C_SCLSpeed <= I2C_SCL_SPEED_SM) {
		//Standard Mode
		ccr_value = (RCC_GetPClk1Value()
				/ (2 * pI2CHandle->I2CConfig.I2C_SCLSpeed));
		temp_reg |= ccr_value & 0xFFF;
	} else {
		//Fast Mode
		temp_reg |= (1 << 15);
		temp_reg |= (pI2CHandle->I2CConfig.I2C_FMDutycycle << 14);
		if (pI2CHandle->I2CConfig.I2C_FMDutycycle == I2C_FM_DUTY_2) {
			ccr_value = RCC_GetPClk1Value()
					/ (3 * pI2CHandle->I2CConfig.I2C_SCLSpeed);
		} else {
			ccr_value = RCC_GetPClk1Value()
					/ (25 * pI2CHandle->I2CConfig.I2C_SCLSpeed);
		}
		temp_reg |= ccr_value & 0xFFF;

	}
	pI2CHandle->pI2Cx->CCR = temp_reg;

	temp_reg = 0;
	uint32_t freqMhz = (RCC_GetPClk1Value() / 1000000U);
	//TRISE Calculation
	if (pI2CHandle->I2CConfig.I2C_SCLSpeed <= I2C_SCL_SPEED_SM) {

		temp_reg = freqMhz + 1U;
	} else {
		temp_reg = ((freqMhz * 300) / 1000U) + 1U;
	}

	pI2CHandle->pI2Cx->TRISE = temp_reg & (0x3F);
}

/*********************************************************************
 * @fn      		  - I2C_DeInit
 *
 * @brief             - Resets the selected I2C peripheral to its
 *                      default reset state by asserting and then
 *                      releasing the peripheral reset via the
 *                      RCC APB1/APB2 reset registers.
 *
 * @param[in]         -  pI2Cx : Base address of the I2C peripheral
 *                      (can be I2C1, I2C2, or I2C3)

 *
 * @return            - None
 *
 * @Note              - This function performs a software reset of
 *                      the I2C peripheral by toggling the reset
 *                      bit in RCC registers.
 *
 *                      Refer to STM32F103 Reference Manual:
 *                      - APB2 peripheral reset register (I2C1)
 *                      - APB1 peripheral reset register (I2C2, I2C3)
 *
 *                      After reset, all I2C registers are restored
 *                      to their default hardware reset values.

 */
void I2C_DeInit(I2C_RegDef_t *pI2Cx) {

	if (pI2Cx == I2C1) {
		I2C1_REG_RESET();
	} else if (pI2Cx == I2C2) {
		I2C2_REG_RESET();
	}
}
/*********************************************************************
 * @fn      		  - I2C_GetFlagStatus
 *
 * @brief             - This function checks whether a given flag is set
 *                       in the I2C Status Register 1 (SR1)
 *
 * @param[in]         - Base address of the I2C peripheral
 * @param[in]         - Flag mask/name to check (e.g. I2C_FLAG_SB, I2C_FLAG_ADDR)
 *
 * @return            - I2C_FLAG_SET if flag is set, I2C_FLAG_RESET otherwise
 *
 * @Note              - none

 */

uint8_t I2C_GetFlagStatus(I2C_RegDef_t *pI2Cx, uint32_t flag) {
	return (pI2Cx->SR1 & flag) ? I2C_FLAG_SET : I2C_FLAG_RESET;
}

/*********************************************************************
 * @fn      		  - I2C_MasterSendData
 *
 * @brief             - This function sends data as an I2C master to a
 *                       given slave address, blocking (polling) until
 *                       each stage completes
 *
 * @param[in]         - Pointer to I2C handle structure
 * @param[in]         - Pointer to Tx buffer containing data to send
 * @param[in]         - Length of data to send (in bytes)
 * @param[in]         - 7-bit address of the slave device
 * @param[in]         - Repeated Start control (I2C_ENABLE_SR / I2C_DISABLE_SR)
 *
 * @return            - none
 *
 * @Note              - This is a blocking call (polling based, not interrupt based)

 */

void I2C_MasterSendData(I2C_Handle_t *pI2CHandle, uint8_t *pTxBuffer,
		uint32_t len, uint8_t slaveAddr, uint8_t Sr) {
	//1.Generate the Start Condition

	I2C_GenerateStartCondition(pI2CHandle->pI2Cx);

	//2.Confirm the start generation is completed by checking SB flag in SR1.
	//	Note: Until SB is cleared SCL will be stretched (pulled to LOW)
	while ( !I2C_GetFlagStatus(pI2CHandle->pI2Cx, I2C_FLAG_SB) )
		;

	//3. Send the address of the slave with r/w bit set to w(0)(total 8 bits)

	I2C_ExecuteAddressPhase(slaveAddr, pI2CHandle->pI2Cx, WRITE);

	//4. Confirm the address phase is completed by checking the ADDR flag in the SR1
	while ( !I2C_GetFlagStatus(pI2CHandle->pI2Cx, I2C_FLAG_ADDR) )
		;

	//5. Clear the ADDR flag according to its software sequence
	//	 Note: Until ADDR is cleared SCL will be stretched (pulled to LOW)
	I2C_ClearAddrFlag(pI2CHandle);

	//6. Send the data until len = 0
	while ( len > 0 ) {
		while ( !I2C_GetFlagStatus(pI2CHandle->pI2Cx, I2C_FLAG_TXE) )
			;
		pI2CHandle->pI2Cx->DR = *pTxBuffer;
		pTxBuffer++;
		len--;
	}

	//7. When len becomes 0 wait for TXE=1 and BTF=1 before generating the STOP condition
	//	 Note: TXE=1,BTF=1, means both SR and DR are empty and next transmission should begin
	//	 when BTF=1 SCL will be stretched (pulled to LOW)

	while ( !I2C_GetFlagStatus(pI2CHandle->pI2Cx, I2C_FLAG_TXE) )
		;

	while ( !I2C_GetFlagStatus(pI2CHandle->pI2Cx, I2C_FLAG_BTF) )
		;

	//8. Generate the STOP condition and master need not to wait for the completion of stop condition.
	//	 Note: generating STOP, automatically clears the BTF
	if (Sr == I2C_DISABLE_SR)
		I2C_GenerateStopCondition(pI2CHandle->pI2Cx);
}

/*********************************************************************
 * @fn      		  - I2C_MasterReceiveData
 *
 * @brief             - This function receives data as an I2C master from a
 *                       given slave address, blocking (polling) until
 *                       each stage completes
 *
 * @param[in]         - Pointer to I2C handle structure
 * @param[in]         - Pointer to Rx buffer where received data will be stored
 * @param[in]         - Length of data to receive (in bytes)
 * @param[in]         - 7-bit address of the slave device
 * @param[in]         - Repeated Start control (I2C_ENABLE_SR / I2C_DISABLE_SR)
 *
 * @return            - none
 *
 * @Note              - This is a blocking call (polling based, not interrupt based)
 *                       ACK bit handling differs depending on whether 1 byte
 *                       or multiple bytes are being received

 */

void I2C_MasterReceiveData(I2C_Handle_t *pI2CHandle, uint8_t *pRxBuffer,
		uint8_t len, uint8_t slaveAddr, uint8_t Sr) {

	//1. Generate the START condition
	I2C_GenerateStartCondition(pI2CHandle->pI2Cx);

	//2. confirm that start generation is completed by checking the SB flag in the SR1
	//   Note: Until SB is cleared SCL will be stretched (pulled to LOW)
	while ( !(I2C_GetFlagStatus(pI2CHandle->pI2Cx, I2C_FLAG_SB)) )
		;

	//3. Send the address of the slave with r/nw bit set to R(1) (total 8 bits )
	I2C_ExecuteAddressPhase(slaveAddr, pI2CHandle->pI2Cx, READ);

	//4. wait until address phase is completed by checking the ADDR flag in the SR1
	while ( !(I2C_GetFlagStatus(pI2CHandle->pI2Cx, I2C_FLAG_ADDR)) )
		;

	//procedure to read only 1 byte from slave
	if (len == 1) {
		//Disable Acking
		I2C_ManageAcking(pI2CHandle->pI2Cx, I2C_DISABLE_ACK);

		//clear the ADDR flag
		I2C_ClearAddrFlag(pI2CHandle);

		//wait until  RXNE becomes 1
		while ( !(I2C_GetFlagStatus(pI2CHandle->pI2Cx, I2C_FLAG_RXNE)) )
			;

		//generate STOP condition
		if (Sr == I2C_DISABLE_SR)
			I2C_GenerateStopCondition(pI2CHandle->pI2Cx);

		//read data in to buffer
		*pRxBuffer = pI2CHandle->pI2Cx->DR;

	}

	//procedure to read data from slave when Len > 1
	if (len > 1) {
		//clear the ADDR flag
		I2C_ClearAddrFlag(pI2CHandle);
		//read the data until Len becomes zero
		for (uint32_t i = len; i > 0; i--) {
			//wait until RXNE becomes 1
			while ( !(I2C_GetFlagStatus(pI2CHandle->pI2Cx, I2C_FLAG_RXNE)) )
				;
			if (i == 2) //if last 2 bytes are remaining
					{
				//Disable Acking
				I2C_ManageAcking(pI2CHandle->pI2Cx, I2C_DISABLE_ACK);

				//generate STOP condition
				if (Sr == I2C_DISABLE_SR)
					I2C_GenerateStopCondition(pI2CHandle->pI2Cx);
			}

			//read the data from data register in to buffer
			*pRxBuffer = pI2CHandle->pI2Cx->DR;
			//increment the buffer address
			pRxBuffer++;
		}

	}
	//re-enable ACKing
	if (pI2CHandle->I2CConfig.I2C_ACKControl == I2C_ENABLE_ACK) {
		I2C_ManageAcking(pI2CHandle->pI2Cx, I2C_ENABLE_ACK);
	}
}

/*********************************************************************
 * @fn      		  - I2C_IRQInterruptConfig
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
void I2C_IRQInterruptConfig(uint8_t IRQNumber, uint8_t EnorDi) {
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
 * @fn      		  - I2C_IRQPriorityConfig
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
void I2C_IRQPriorityConfig(uint8_t IRQNumber, uint8_t IRQPriority) {

	//1.Find out the IPR register
	uint8_t iprx = (IRQNumber / 4);
	uint8_t iprx_section = IRQNumber % 4;
	uint8_t shift_amount = (8 * iprx_section) + (8 - NO_PR_BITS_IMPLEMENTED);
	*(NVIC_PR_BASEADDR + iprx) &= ~(0xF << shift_amount);
	*(NVIC_PR_BASEADDR + (iprx)) |= (IRQPriority << shift_amount);

}

/*********************************************************************
 * @fn                - I2C_PeripheralControl
 *
 * @brief             - Enables or disables the I2C peripheral by
 *                      setting or clearing the PE (Peripheral Enable) bit
 *                      in the CR1 register.
 *
 * @param[in]         - pI2Cx : Base address of the I2C peripheral
 *
 * @param[in]         - EnorDi : ENABLE or DISABLE
 *                               ENABLE  -> Sets the SPE bit
 *                               DISABLE -> Clears the SPE bit
 *
 * @return            - None
 *
 * @Note              - The I2C peripheral must be enabled before any
 *                      data transmission or reception can occur.
 *                      It is recommended to disable the peripheral
 *                      before changing I2C configuration settings.
 *
 *********************************************************************/
void I2C_PeripheralControl(I2C_RegDef_t *pI2Cx, uint8_t EnorDi) {
	if (EnorDi == ENABLE) {
		pI2Cx->CR1 |= (1 << I2C_CR1_PE);
	} else {
		pI2Cx->CR1 &= ~(1 << I2C_CR1_PE);
	}
}

/*********************************************************************
 * @fn      		  - I2C_ManageAcking
 *
 * @brief             - Enables or disables Acking for the given I2C peripheral
 *
 * @param[in]         - Base address of the I2C peripheral (I2C1, I2C2, etc.)
 * @param[in]         - ENABLE or DISABLE macros
 *
 * @return            - none
 *
 * @Note              - ACK bit is automatically cleared by hardware when
 *                       PE = 0, so this function must be called AFTER
 *                       enabling the I2C peripheral (I2C_PeripheralControl),
 *                       not before.
 */
void I2C_ManageAcking(I2C_RegDef_t *pI2Cx, uint8_t EnorDi) {
	if (EnorDi == I2C_ENABLE_ACK) {
		pI2Cx->CR1 |= (1 << I2C_CR1_ACK);
	} else {
		pI2Cx->CR1 &= ~(1 << I2C_CR1_ACK);
	}
}
/*********************************************************************
 * @fn      		  - I2C_CloseReceiveData
 *
 * @brief             - This function closes out an ongoing interrupt-based
 *                       I2C reception. It disables the buffer and event
 *                       interrupts, resets the handle's Rx-related state
 *                       members, and re-enables ACK if the application's
 *                       configured ACK control was originally enabled
 *
 * @param[in]         - Pointer to I2C handle structure
 *
 * @return            - none
 *
 * @Note              - Should be called once RxLen reaches 0 in the RXNE
 *                       interrupt handler, or when the application wants
 *                       to abort an ongoing reception

 */
void I2C_CloseReceiveData(I2C_Handle_t *pI2CHandle){
	//Implement the code to disable ITBUFEN Control Bit
		pI2CHandle->pI2Cx->CR2 &= ~(1 << I2C_CR2_ITBUFEN);

		//Implement the code to disable ITEVTEN Control Bit
		pI2CHandle->pI2Cx->CR2 &= ~(1 << I2C_CR2_ITEVTEN);

		pI2CHandle->TxRxState = I2C_READY;
		pI2CHandle->RxLen = 0;
		pI2CHandle->RxSize = 0;
		pI2CHandle->pRxBuffer = NULL;
		if(pI2CHandle->I2CConfig.I2C_ACKControl == I2C_ENABLE_ACK)
		{
			I2C_ManageAcking(pI2CHandle->pI2Cx, I2C_ENABLE_ACK);
		}

}
/*********************************************************************
 * @fn      		  - I2C_CloseSendData
 *
 * @brief             - This function closes out an ongoing interrupt-based
 *                       I2C transmission. It disables the buffer and event
 *                       interrupts and resets the handle's Tx-related state
 *                       members
 *
 * @param[in]         - Pointer to I2C handle structure
 *
 * @return            - none
 *
 * @Note              - Should be called once TxLen reaches 0 in the BTF
 *                       interrupt handler, or when the application wants
 *                       to abort an ongoing transmission

 */
void I2C_CloseSendData(I2C_Handle_t *pI2CHandle){
	//Implement the code to disable ITBUFEN Control Bit
		pI2CHandle->pI2Cx->CR2 &= ~(1 << I2C_CR2_ITBUFEN);

		//Implement the code to disable ITEVTEN Control Bit
		pI2CHandle->pI2Cx->CR2 &= ~(1 << I2C_CR2_ITEVTEN);

		pI2CHandle->TxRxState = I2C_READY;
		pI2CHandle->TxLen = 0;
		pI2CHandle->pTxBuffer = NULL;

}
/*********************************************************************
 * @fn      		  - I2C_MasterSendDataIT
 *
 * @brief             - This function sends data as an I2C master using
 *                       interrupt mode (non-blocking). It only initiates
 *                       the transfer; the actual byte-by-byte transmission
 *                       is handled in I2C_EV_IRQHandling via ISR callbacks
 *
 * @param[in]         - Pointer to I2C handle structure
 * @param[in]         - Pointer to Tx buffer containing data to send
 * @param[in]         - Length of data to send (in bytes)
 * @param[in]         - 7-bit address of the slave device
 * @param[in]         - Repeated Start control (I2C_ENABLE_SR / I2C_DISABLE_SR)
 *
 * @return            - Current busy state of the I2C handle before this
 *                       call (I2C_READY, I2C_BUSY_IN_TX, or I2C_BUSY_IN_RX)
 *
 * @Note              - Non-blocking call. If the peripheral is already busy
 *                       in TX or RX, the new request is rejected (transfer
 *                       is not started) and the caller should check the
 *                       returned busy state

 */
uint8_t I2C_MasterSendDataIT(I2C_Handle_t *pI2CHandle, uint8_t *pTxBuffer,
		uint32_t len, uint8_t slaveAddr, uint8_t Sr) {
	uint8_t busystate = pI2CHandle->TxRxState;

	if ((busystate != I2C_BUSY_IN_TX) && (busystate != I2C_BUSY_IN_RX)) {
		pI2CHandle->pTxBuffer = pTxBuffer;
		pI2CHandle->TxLen = len;
		pI2CHandle->TxRxState = I2C_BUSY_IN_TX;
		pI2CHandle->DevAddr = slaveAddr;
		pI2CHandle->Sr = Sr;

		//Implement code to Generate START Condition

		I2C_GenerateStartCondition(pI2CHandle->pI2Cx);

		//Implement the code to enable ITBUFEN Control Bit
		pI2CHandle->pI2Cx->CR2 |= (1 << I2C_CR2_ITBUFEN);

		//Implement the code to enable ITEVTEN Control Bit
		pI2CHandle->pI2Cx->CR2 |= (1 << I2C_CR2_ITEVTEN);

		//Implement the code to enable ITERREN Control Bit
		pI2CHandle->pI2Cx->CR2 |= (1 << I2C_CR2_ITERREN);

	}

	return busystate;
}

/*********************************************************************
 * @fn      		  - I2C_MasterReceiveDataIT
 *
 * @brief             - This function receives data as an I2C master using
 *                       interrupt mode (non-blocking). It only initiates
 *                       the transfer; the actual byte-by-byte reception
 *                       is handled in I2C_EV_IRQHandling via ISR callbacks
 *
 * @param[in]         - Pointer to I2C handle structure
 * @param[in]         - Pointer to Rx buffer where received data will be stored
 * @param[in]         - Length of data to receive (in bytes)
 * @param[in]         - 7-bit address of the slave device
 * @param[in]         - Repeated Start control (I2C_ENABLE_SR / I2C_DISABLE_SR)
 *
 * @return            - Current busy state of the I2C handle before this
 *                       call (I2C_READY, I2C_BUSY_IN_TX, or I2C_BUSY_IN_RX)
 *
 * @Note              - Non-blocking call. If the peripheral is already busy
 *                       in TX or RX, the new request is rejected (transfer
 *                       is not started) and the caller should check the
 *                       returned busy state

 */
uint8_t I2C_MasterReceiveDataIT(I2C_Handle_t *pI2CHandle, uint8_t *pRxBuffer,
		uint8_t len, uint8_t slaveAddr, uint8_t Sr) {
	uint8_t busystate = pI2CHandle->TxRxState;

	if ((busystate != I2C_BUSY_IN_TX) && (busystate != I2C_BUSY_IN_RX)) {
		pI2CHandle->pRxBuffer = pRxBuffer;
		pI2CHandle->RxLen = len;
		pI2CHandle->TxRxState = I2C_BUSY_IN_RX;
		pI2CHandle->RxSize = len; //Rxsize is used in the ISR code to manage the data reception
		pI2CHandle->DevAddr = slaveAddr;
		pI2CHandle->Sr = Sr;

		//Implement code to Generate START Condition
		I2C_GenerateStartCondition(pI2CHandle->pI2Cx);

		//Implement the code to enable ITBUFEN Control Bit
		pI2CHandle->pI2Cx->CR2 |= (1 << I2C_CR2_ITBUFEN);

		//Implement the code to enable ITEVFEN Control Bit
		pI2CHandle->pI2Cx->CR2 |= (1 << I2C_CR2_ITEVTEN);

		//Implement the code to enable ITERREN Control Bit
		pI2CHandle->pI2Cx->CR2 |= (1 << I2C_CR2_ITERREN);
	}

	return busystate;
}

/*********************************************************************
 * @fn      		  - I2C_MasterHandleRXNEInterrupt
 *
 * @brief             - Helper function called from I2C_EV_IRQHandling when
 *                       an RXNE (Receive buffer Not Empty) event occurs in
 *                       master mode. Handles reading the received byte(s)
 *                       from DR, manages ACK disabling for the last 2 bytes,
 *                       and closes out reception when RxLen reaches 0
 *
 * @param[in]         - Pointer to I2C handle structure
 *
 * @return            - none
 *
 * @Note              - This is a static/private helper, only used internally
 *                       by the interrupt event handler

 */

static void I2C_MasterHandleRXNEInterrupt(I2C_Handle_t *pI2CHandle) {
	//We have to the data reception
	if (pI2CHandle->RxSize == 1) {
		*(pI2CHandle->pRxBuffer) = pI2CHandle->pI2Cx->DR;
		pI2CHandle->RxLen--;
	}
	if (pI2CHandle->RxSize > 1) {
		if (pI2CHandle->RxSize == 2) {
			//Disable ACK
			I2C_ManageAcking(pI2CHandle->pI2Cx, I2C_DISABLE_ACK);
		}
		//Read DR
		*(pI2CHandle->pRxBuffer) = pI2CHandle->pI2Cx->DR;
		pI2CHandle->pRxBuffer++;
		pI2CHandle->RxLen--;
	}
	if (pI2CHandle->RxLen == 0) {
		//Close the I2C data reception and notify the application
		// 1. Generate the STOP condition
		if (pI2CHandle->Sr == I2C_DISABLE_SR) {
			I2C_GenerateStopCondition(pI2CHandle->pI2Cx);
		}
		//2.Close the I2C Rx
		I2C_CloseReceiveData(pI2CHandle);
		//3. Notify the application
		I2C_ApplicationEventCallback(pI2CHandle, I2C_EV_RX_CMPLT);
	}
}

/*********************************************************************
 * @fn      		  - I2CMasterHandleTXEInterrupt
 *
 * @brief             - Helper function called from I2C_EV_IRQHandling when
 *                       a TXE (Transmit buffer Empty) event occurs in
 *                       master mode. Loads the next byte from the Tx buffer
 *                       into DR if data remains to be sent
 *
 * @param[in]         - Pointer to I2C handle structure
 *
 * @return            - none
 *
 * @Note              - This is a static/private helper, only used internally
 *                       by the interrupt event handler

 */
static void I2CMasterHandleTXEInterrupt(I2C_Handle_t *pI2CHandle) {
	//Check the length of the data
	if (pI2CHandle->TxLen > 0) {
		// 1) Load the data to the DR
		pI2CHandle->pI2Cx->DR = *(pI2CHandle->pTxBuffer);
		// 2) Decrease the transmission data length
		pI2CHandle->TxLen--;
		// 3) Increment the buffer address
		pI2CHandle->pTxBuffer++;
	}
}
/*********************************************************************
 * @fn      		  - I2C_EV_IRQHandling
 *
 * @brief             - This function handles all I2C event interrupts
 *                       (SB, ADDR, BTF, STOPF, TXE, RXNE) for both master
 *                       and slave modes of operation. It should be called
 *                       from the corresponding I2C event IRQ handler
 *
 * @param[in]         - Pointer to I2C handle structure
 *
 * @return            - none
 *
 * @Note              - SB and STOPF flags are mutually exclusive to mode:
 *                       SB is only relevant in master mode, STOPF only in
 *                       slave mode

 */
void I2C_EV_IRQHandling(I2C_Handle_t *pI2CHandle) {

	//Interrupt handling for both master and slave mode of a device

	uint32_t temp1, temp2, temp3;

	temp1 = pI2CHandle->pI2Cx->CR2 & (1 << I2C_CR2_ITEVTEN);
	temp2 = pI2CHandle->pI2Cx->CR2 & (1 << I2C_CR2_ITBUFEN);

	temp3 = pI2CHandle->pI2Cx->SR1 & (1 << I2C_SR1_SB);
	//1. Handle For interrupt generated by SB event
	//	Note : SB flag is only applicable in Master mode
	if (temp1 && temp3) {
		//Interrupt is generated because of SB event
		//This block will not be executed in slave mode because for slave SB is always zero
		//In this block lets executed the address phase
		if (pI2CHandle->TxRxState == I2C_BUSY_IN_TX) {
			I2C_ExecuteAddressPhase(pI2CHandle->DevAddr, pI2CHandle->pI2Cx,
			WRITE);
		}
		if (pI2CHandle->TxRxState == I2C_BUSY_IN_RX) {
			I2C_ExecuteAddressPhase(pI2CHandle->DevAddr, pI2CHandle->pI2Cx,
			READ);
		}
	}

	temp3 = pI2CHandle->pI2Cx->SR1 & (1 << I2C_SR1_ADDR);
	//2. Handle For interrupt generated by ADDR event
	//Note : When master mode : Address is sent
	//		 When Slave mode   : Address matched with own address
	if (temp1 && temp3) {
		//ADDR flag is set
		I2C_ClearAddrFlag(pI2CHandle);

	}

	temp3 = pI2CHandle->pI2Cx->SR1 & (1 << I2C_SR1_BTF);

	//3. Handle For interrupt generated by BTF(Byte Transfer Finished) event
	if (temp1 && temp3) {
		//BTF flag is set
		if (pI2CHandle->TxRxState == I2C_BUSY_IN_TX) {
			// make sure that TxE bit is also set
			if (pI2CHandle->pI2Cx->SR1 & (1 << I2C_SR1_TxE)) {
				//BTF, TxE = 1
				if (pI2CHandle->TxLen == 0) {
					// 1. Generate the STOP condition
					if (pI2CHandle->Sr == I2C_DISABLE_SR)
						I2C_GenerateStopCondition(pI2CHandle->pI2Cx);
					// 2. Reset all the member elements of the handle structure
					I2C_CloseSendData(pI2CHandle);
					// 3. Notify the application about the transmission complete
					I2C_ApplicationEventCallback(pI2CHandle, I2C_EV_TX_CMPLT);
				}
			}
		} else if (pI2CHandle->TxRxState == I2C_BUSY_IN_RX) {
		}
	}

	temp3 = pI2CHandle->pI2Cx->SR1 & (1 << I2C_SR1_STOPF);
	//4. Handle For interrupt generated by STOPF event
	// Note : Stop detection flag is applicable only slave mode . For master this flag will never be set
	if (temp1 && temp3) {
		//STOPF flag is set
		//Clear the STOPF (i.e 1) read SR1 2)Write to CR1)
		pI2CHandle->pI2Cx->CR1 |= 0x0000; // dummy write to CR1 to clear STOPF (requires CR1 read-modify-write)

		//Notify the application that STOP is detected
		I2C_ApplicationEventCallback(pI2CHandle, I2C_EV_STOP);
	}

	temp3 = pI2CHandle->pI2Cx->SR1 & (1 << I2C_SR1_TxE);
	//5. Handle For interrupt generated by TXE event
	if (temp1 && temp2 && temp3) {
		//TXE flag is set
		//Check for device mode : Master mode first
		if ((pI2CHandle->pI2Cx->SR2) & (1 << I2C_SR2_MSL)) {
			//Data Transmission only if application state is BUSY_TXN
			if (pI2CHandle->TxRxState == I2C_BUSY_IN_TX) {
				//Check the length of the data
				I2CMasterHandleTXEInterrupt(pI2CHandle);
			}
		}
	}
	temp3 = pI2CHandle->pI2Cx->SR1 & (1 << I2C_SR1_RxNE);
	//6. Handle For interrupt generated by RXNE event
	if (temp1 && temp2 && temp3) {
		//RXNE flag is set
		if ((pI2CHandle->pI2Cx->SR2) & (1 << I2C_SR2_MSL)) {
			if (pI2CHandle->TxRxState == I2C_BUSY_IN_RX) {
				//We have to the data reception
				I2C_MasterHandleRXNEInterrupt(pI2CHandle);
			}

		}
	}
}
/*********************************************************************
 * @fn      		  - I2C_ER_IRQHandling
 *
 * @brief             - This function handles all I2C error interrupts:
 *                       Bus Error (BERR), Arbitration Lost (ARLO), ACK
 *                       Failure (AF), Overrun/Underrun (OVR), and Timeout.
 *                       For each error detected (and enabled via ITERREN),
 *                       it clears the corresponding flag and notifies the
 *                       application via the callback
 *
 * @param[in]         - Pointer to I2C handle structure
 *
 * @return            - none
 *
 * @Note              - Should be called from the corresponding I2C error
 *                       IRQ handler

 */
void I2C_ER_IRQHandling(I2C_Handle_t *pI2CHandle) {

	uint32_t temp1,temp2;

	    //Know the status of  ITERREN control bit in the CR2
		temp2 = (pI2CHandle->pI2Cx->CR2) & ( 1 << I2C_CR2_ITERREN);


	/***********************Check for Bus error************************************/
		temp1 = (pI2CHandle->pI2Cx->SR1) & ( 1<< I2C_SR1_BERR);
		if(temp1  && temp2 )
		{
			//This is Bus error

			//Implement the code to clear the buss error flag
			pI2CHandle->pI2Cx->SR1 &= ~( 1 << I2C_SR1_BERR);

			//Implement the code to notify the application about the error
		   I2C_ApplicationEventCallback(pI2CHandle,I2C_ERROR_BERR);
		}

	/***********************Check for arbitration lost error************************************/
		temp1 = (pI2CHandle->pI2Cx->SR1) & ( 1 << I2C_SR1_ARLO );
		if(temp1  && temp2)
		{
			//This is arbitration lost error

			//Implement the code to clear the arbitration lost error flag
			(pI2CHandle->pI2Cx->SR1) &= ~( 1 << I2C_SR1_ARLO );

			//Implement the code to notify the application about the error
			I2C_ApplicationEventCallback(pI2CHandle,I2C_ERROR_ARLO);
		}

	/***********************Check for ACK failure  error************************************/

		temp1 = (pI2CHandle->pI2Cx->SR1) & ( 1 << I2C_SR1_AF);
		if(temp1  && temp2)
		{
			//This is ACK failure error

		    //Implement the code to clear the ACK failure error flag
			(pI2CHandle->pI2Cx->SR1) &= ~( 1 << I2C_SR1_AF);

			//Implement the code to notify the application about the error
			I2C_ApplicationEventCallback(pI2CHandle,I2C_ERROR_AF);
		}

	/***********************Check for Overrun/underrun error************************************/
		temp1 = (pI2CHandle->pI2Cx->SR1) & ( 1 << I2C_SR1_OVR);
		if(temp1  && temp2)
		{
			//This is Overrun/underrun

		    //Implement the code to clear the Overrun/underrun error flag
			(pI2CHandle->pI2Cx->SR1) &= ~( 1 << I2C_SR1_OVR);

			//Implement the code to notify the application about the error
			I2C_ApplicationEventCallback(pI2CHandle,I2C_ERROR_OVR);
		}

	/***********************Check for Time out error************************************/
		temp1 = (pI2CHandle->pI2Cx->SR1) & ( 1 << I2C_SR1_TIMEOUT);
		if(temp1  && temp2)
		{
			//This is Time out error

		    //Implement the code to clear the Time out error flag
			(pI2CHandle->pI2Cx->SR1) &= ~( 1 << I2C_SR1_TIMEOUT);
			//Implement the code to notify the application about the error
			I2C_ApplicationEventCallback(pI2CHandle,I2C_ERROR_TIMEOUT);
		}

}
__weak void I2C_ApplicationEventCallback(I2C_Handle_t *pI2CHandle,
		uint8_t AppEv) {
	//This is a weak implementation . The user application may override this function
}

