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
static void I2C_ClearAddrFlag(I2C_RegDef_t *pI2Cx);
static void I2C_GenerateStopCondition(I2C_RegDef_t *pI2Cx);

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
 * @fn      		  - I2C_ClearAddrFlag
 *
 * @brief             - This function clears the ADDR flag in SR1 by
 *                      performing the required dummy read sequence of
 *                      SR1 followed by SR2
 *
 * @param[in]         - pI2Cx : Base address of I2C peripheral
 *                      (can be I2C1 or I2C2)
 *
 * @return            -  None
 *
 * @Note              -  As per the I2C peripheral hardware, the ADDR
 *                      flag is cleared only by a software sequence:
 *                      a read of SR1 followed by a read of SR2.
 *
 *                      The (void) cast is used to explicitly discard
 *                      the read values since we only care about the
 *                      side effect of reading the registers, not their
 *                      contents; this also avoids unused-value/
 *                      compiler warnings.
 *
 *                      This function must be called after the ADDR
 *                      flag is set (i.e., after the slave address has
 *                      been acknowledged) to allow the communication
 *                      to proceed further.
 */
static void I2C_ClearAddrFlag(I2C_RegDef_t *pI2Cx) {

	(void) pI2Cx->SR1;
	(void) pI2Cx->SR2;
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
static void I2C_GenerateStopCondition(I2C_RegDef_t *pI2Cx) {
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

uint8_t I2C_GetFlagStatus(I2C_RegDef_t *pI2Cx, uint32_t flag) {
	return (pI2Cx->SR1 & flag) ? I2C_FLAG_SET : I2C_FLAG_RESET;
}

void I2C_MasterSendData(I2C_Handle_t *pI2CHandle, uint8_t *pTxBuffer,
		uint32_t len, uint8_t slaveAddr,uint8_t Sr) {
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
	I2C_ClearAddrFlag(pI2CHandle->pI2Cx);

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
	if(Sr == I2C_DISABLE_SR)
	I2C_GenerateStopCondition(pI2CHandle->pI2Cx);
}

void I2C_MasterReceiveData(I2C_Handle_t *pI2CHandle, uint8_t *pRxBuffer,
		uint8_t len, uint8_t slaveAddr,uint8_t Sr) {

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
		I2C_ManageAcking(pI2CHandle->pI2Cx, I2C_ACK_DISABLE);

		//clear the ADDR flag
		I2C_ClearAddrFlag(pI2CHandle->pI2Cx);

		//wait until  RXNE becomes 1
		while ( !(I2C_GetFlagStatus(pI2CHandle->pI2Cx, I2C_FLAG_RXNE)) );

		//generate STOP condition
		if(Sr == I2C_DISABLE_SR)
		I2C_GenerateStopCondition(pI2CHandle->pI2Cx);

		//read data in to buffer
		*pRxBuffer = pI2CHandle->pI2Cx->DR;

	}

	//procedure to read data from slave when Len > 1
	if (len > 1) {
		//clear the ADDR flag
		I2C_ClearAddrFlag(pI2CHandle->pI2Cx);
		//read the data until Len becomes zero
		for (uint32_t i = len; i > 0; i--) {
			//wait until RXNE becomes 1
			while ( !(I2C_GetFlagStatus(pI2CHandle->pI2Cx, I2C_FLAG_RXNE)) )
				;
			if (i == 2) //if last 2 bytes are remaining
					{
				//Disable Acking
				I2C_ManageAcking(pI2CHandle->pI2Cx, I2C_ACK_DISABLE);

				//generate STOP condition
				if(Sr == I2C_DISABLE_SR)
				I2C_GenerateStopCondition(pI2CHandle->pI2Cx);
			}

			//read the data from data register in to buffer
			*pRxBuffer = pI2CHandle->pI2Cx->DR;
			//increment the buffer address
			pRxBuffer++;
		}

	}
	//re-enable ACKing
	if (pI2CHandle->I2CConfig.I2C_ACKControl == I2C_ACK_ENABLE) {
		I2C_ManageAcking(pI2CHandle->pI2Cx, I2C_ACK_ENABLE);
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
	if (EnorDi == I2C_ACK_ENABLE) {
		pI2Cx->CR1 |= (1 << I2C_CR1_ACK);
	} else {
		pI2Cx->CR1 &= ~(1 << I2C_CR1_ACK);
	}
}
__weak void I2C_ApplicationEventCallback(I2C_Handle_t *pI2CHandle,
		uint8_t AppEv) {
	//This is a weak implementation . The user application may override this function
}

