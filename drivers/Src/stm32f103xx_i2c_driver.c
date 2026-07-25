/*
 *   stm32f103xx_i2c_driver.c
 *
 *  Created on		: 24 Jul 2026
 *  Author			: Tara Alexander
 */

#include "stm32f103xx_i2c_driver.h"


uint16_t AHB_PreScaler[8] = {2,4,8,16,64,128,256,512};
uint16_t APB1_PreScaler[4] = {2,4,8,16};


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



uint32_t RCC_GetPLLOutputClock(void)
{
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
    if (pllMulBits == 15)
    {
        pllMul = 16;
    }
    else
    {
        pllMul = pllMulBits + 2;
    }

    // Determine PLL input clock
    if (pllSrc == 0)
    {
        // HSI/2 is always used when PLLSRC = 0
        inputClk = 8000000 / 2;   // = 4,000,000
    }
    else
    {
        // PLLSRC = 1 → HSE selected, optionally divided by 2
        if (pllXtPre == 0)
        {
            inputClk = 8000000;       // HSE not divided
        }
        else
        {
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
uint32_t RCC_GetPClk1Value(void)
{
	uint32_t pclk1,SysClk;

	uint8_t clksrc,temp,ahbpre,apb1pre;

	clksrc = (RCC->CFGR >> 2) & 0x3;

	if(clksrc == 0)
	{
		SysClk = 8000000;
	}
	else if (clksrc == 1)
	{
		SysClk = 8000000;
	}
	else if (clksrc == 2)
	{
		SysClk = RCC_GetPLLOutputClock();
	}

	temp = (RCC->CFGR >> 4) & 0xF;

	if(temp < 8)
	{
		ahbpre = 1;
	}
	else
	{
		ahbpre = AHB_PreScaler[temp-8];
	}

	temp = (RCC->CFGR >> 8) & 0x7;

	if(temp < 4)
	{
		apb1pre = 1;
	}
	else
	{
		apb1pre = APB1_PreScaler[temp-4];
	}

	pclk1 = (SysClk/ahbpre)/apb1pre;

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

		//Enable ACKing
		temp_reg = 0;
		temp_reg |= pI2CHandle->I2CConfig.I2C_ACKControl << 10;
		pI2CHandle->pI2Cx->CR1 = temp_reg;

		//Configure the FREQ field of CR2

		temp_reg = 0;
		temp_reg |= (RCC_GetPClk1Value() / 1000000U); //Remove the MHz;need only the value

		pI2CHandle->pI2Cx->CR2 = ( temp_reg & 0x3F ); //Masking the last 6 bits to write the FREQ value

		//device own address

		temp_reg = 0;
		temp_reg |= pI2CHandle -> I2CConfig.I2C_DeviceAddress << 1;
		// bit 14 of OAR1 "should always be kept at 1 by software."
		temp_reg |= (1<<14);
		pI2CHandle->pI2Cx->OAR1 = temp_reg;

		//CCR calculations
		uint16_t ccr_value = 0;
		temp_reg = 0;
		if(pI2CHandle->I2CConfig.I2C_SCLSpeed <= I2C_SCL_SPEED_SM)
		{
			//Standard Mode
			ccr_value = (RCC_GetPClk1Value() /(2 * pI2CHandle->I2CConfig.I2C_SCLSpeed ));
			temp_reg |= ccr_value & 0xFFF;
		}
		else
		{
			//Fast Mode
			temp_reg |= (1 << 15);
			temp_reg |= (pI2CHandle->I2CConfig.I2C_FMDutycycle << 14);
			if(pI2CHandle->I2CConfig.I2C_FMDutycycle == I2C_FM_DUTY_2)
			{
				ccr_value = RCC_GetPClk1Value() /(3 * pI2CHandle->I2CConfig.I2C_SCLSpeed );
			}
			else
			{
				ccr_value = RCC_GetPClk1Value() /(25 * pI2CHandle->I2CConfig.I2C_SCLSpeed );
			}
			temp_reg |= ccr_value & 0xFFF;

		}
		pI2CHandle->pI2Cx->CCR = temp_reg;
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

__weak void I2C_ApplicationEventCallback(I2C_Handle_t *pI2CHandle,uint8_t AppEv)
{
	//This is a weak implementation . The user application may override this function
}

