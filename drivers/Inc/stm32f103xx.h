/*
 * stm32f103xx.h
 *
 *  Created on: 9 Mar 2026
 *      Author: Tara
 */

#ifndef INC_STM32F103XX_H_
#define INC_STM32F103XX_H_
#include<stdint.h>
/*
 * Base addresses of Flash and SRAM Memories
 */
#define __vo		volatile


#define FLASH_BASEADDR			0x08000000U 	//Refer Reference Manual for base addresses
#define SRAM1_BASEADDR			0x20000000U
#define SRAM 					SRAM1_BASEADDR
#define ROM_BASEADDR 			0x1FFFF000U

/*
 * AHB and APBx Bus Peripheral base addresses
 */
#define PERIPH_BASEADDR				0x40000000U
#define APB1PERIPH_BASEADDR			(PERIPH_BASEADDR)
#define APB2PERIPH_BASEADDR			(PERIPH_BASEADDR + 0x00010000U)
#define AHBPERIPH_BASEADDR			(PERIPH_BASEADDR + 0x00018000U)

/*
 * GPIOs Peripherals hanging on APB2 bus
 */
#define GPIOA_BASEADDR				(APB2PERIPH_BASEADDR + 0x00000800U)
#define GPIOB_BASEADDR				(APB2PERIPH_BASEADDR + 0x00000C00U)
#define GPIOC_BASEADDR				(APB2PERIPH_BASEADDR + 0x00001000U)
#define GPIOD_BASEADDR				(APB2PERIPH_BASEADDR + 0x00001400U)
#define GPIOE_BASEADDR				(APB2PERIPH_BASEADDR + 0x00001800U)
#define GPIOF_BASEADDR				(APB2PERIPH_BASEADDR + 0x00001C00U)
#define GPIOG_BASEADDR				(APB2PERIPH_BASEADDR + 0x00002000U)

/*
 *  Peripherals hanging on APB1 bus
 */

#define I2C1_BASEADDR				(APB1PERIPH_BASEADDR + 0x00005400U)
#define I2C2_BASEADDR				(APB1PERIPH_BASEADDR + 0x00005800U)

#define SPI2_BASEADDR				(APB1PERIPH_BASEADDR + 0x00003800U)
#define SPI3_BASEADDR				(APB1PERIPH_BASEADDR + 0x00003C00U)

#define USART2_BASEADDR				(APB1PERIPH_BASEADDR + 0x00004400U)
#define USART3_BASEADDR				(APB1PERIPH_BASEADDR + 0x00004800U)

#define UART4_BASEADDR				(APB1PERIPH_BASEADDR + 0x00004C00U)
#define UART5_BASEADDR				(APB1PERIPH_BASEADDR + 0x00005000U)


/*
 *  Peripherals hanging on APB2 bus
 */

#define EXT1_BASEADDR 				(APB2PERIPH_BASEADDR + 0x00000400U)
#define SPI1_BASEADDR 				(APB2PERIPH_BASEADDR + 0x00003000U)
#define USART1_BASEADDR 			(APB2PERIPH_BASEADDR + 0x00003800U)

/*
 *  Peripherals hanging on AHB bus
 */

#define RCC_BASEADDR 				(AHBPERIPH_BASEADDR + 0x00009000U)

/**************************************peripheral register definition structures**************************************/

typedef struct
{
	__vo uint32_t CRL;				/*Port configuration register low , 			Address Offset:0x00*/
	__vo uint32_t CRH;				/*Port configuration register high , 			Address Offset:0x04*/
	__vo uint32_t IDR;				/*Port input data register , 					Address Offset:0x08*/
	__vo uint32_t ODR;				/*Port output data register , 					Address Offset:0x0C*/
	__vo uint32_t BSRR;				/*Port bit set/reset register, 					Address Offset:0x10*/
	__vo uint32_t BRR;				/*Port bit reset register, 						Address Offset:0x14*/
	__vo uint32_t LCKR;				/*Port configuration lock register, 			Address Offset:0x18*/
}GPIO_RegDef_t;


typedef struct
{
	__vo uint32_t CR;					/*Clock control register, 						Address Offset:0x00*/
	__vo uint32_t CFGR;					/*Clock configuration register , 				Address Offset:0x04*/
	__vo uint32_t CIR;					/*Clock interrupt register,						Address Offset:0x08*/
	__vo uint32_t APB2RSTR;				/*APB2 peripheral reset register, 				Address Offset:0x0C*/
	__vo uint32_t APB1RSTR;				/*APB1 peripheral reset register, 				Address Offset:0x10*/
	__vo uint32_t AHBENR;				/*AHB peripheral clock enable register, 		Address Offset:0x14*/
	__vo uint32_t APB2ENR;				/*APB2 peripheral clock enable register, 		Address Offset:0x18*/
	__vo uint32_t APB1ENR;				/*APB1 peripheral clock enable register, 		Address Offset:0x1C*/
	__vo uint32_t BDCR;					/*Backup domain control register,	 			Address Offset:0x20*/
	__vo uint32_t CSR;					/*Control/status register,			 			Address Offset:0x2C*/
}RCC_RegDef_t;

/*********************************peripheral definitions (Peripheral Base Addresses typecasted to xxx_RegDef_t)*************************/
#define GPIOA  			((GPIO_RegDef_t*)GPIOA_BASEADDR)
#define GPIOB  			((GPIO_RegDef_t*)GPIOB_BASEADDR)
#define GPIOC  			((GPIO_RegDef_t*)GPIOC_BASEADDR)
#define GPIOD  			((GPIO_RegDef_t*)GPIOD_BASEADDR)
#define GPIOE  			((GPIO_RegDef_t*)GPIOE_BASEADDR)
#define GPIOF  			((GPIO_RegDef_t*)GPIOF_BASEADDR)
#define GPIOG  			((GPIO_RegDef_t*)GPIOG_BASEADDR)

#define RCC 			((RCC_RegDef_t*)RCC_BASEADDR)

/*
 *  Clock Enable Macros for GPIOx peripherals
 */
#define GPIOA_PCLK_EN()			( RCC->APB2ENR |= (1<<2) )
#define GPIOB_PCLK_EN()			( RCC->APB2ENR |= (1<<3) )
#define GPIOC_PCLK_EN()			( RCC->APB2ENR |= (1<<4) )
#define GPIOD_PCLK_EN()			( RCC->APB2ENR |= (1<<5) )
#define GPIOE_PCLK_EN()			( RCC->APB2ENR |= (1<<6) )
#define GPIOF_PCLK_EN()			( RCC->APB2ENR |= (1<<7) )
#define GPIOG_PCLK_EN()			( RCC->APB2ENR |= (1<<8) )


/*
 *  Clock Enable Macros for I2Cx peripherals
 */
#define I2C1_PCLK_EN()			( RCC->APB1ENR |= (1<<21) )
#define I2C2_PCLK_EN()			( RCC->APB1ENR |= (1<<22) )


/*
 *  Clock Enable Macros for SPIx peripherals
 */
#define SPI1_PCLK_EN()			( RCC->APB2ENR |= (1<<12) )
#define SPI2_PCLK_EN()			( RCC->APB1ENR |= (1<<14) )
#define SPI3_PCLK_EN()			( RCC->APB1ENR |= (1<<15) )


/*
 *  Clock Enable Macros for USARTx peripherals
 */
#define USART1_PCLK_EN()			( RCC->APB2ENR |= (1<<14) )
#define USART2_PCLK_EN()			( RCC->APB1ENR |= (1<<17) )
#define USART3_PCLK_EN()			( RCC->APB1ENR |= (1<<18) )
#define UART4_PCLK_EN()				( RCC->APB1ENR |= (1<<19) )
#define UART5_PCLK_EN()				( RCC->APB1ENR |= (1<<20) )


/*
 *  Clock Disable Macros for GPIOx peripherals
 */
#define GPIOA_PCLK_DI()			( RCC->APB2ENR &= ~(1<<2) )
#define GPIOB_PCLK_DI()			( RCC->APB2ENR &= ~(1<<3) )
#define GPIOC_PCLK_DI()			( RCC->APB2ENR &= ~(1<<4) )
#define GPIOD_PCLK_DI()			( RCC->APB2ENR &= ~(1<<5) )
#define GPIOE_PCLK_DI()			( RCC->APB2ENR &= ~(1<<6) )
#define GPIOF_PCLK_DI()			( RCC->APB2ENR &= ~(1<<7) )
#define GPIOG_PCLK_DI()			( RCC->APB2ENR &= ~(1<<8) )

/*
 *  Clock Disable Macros for I2Cx peripherals
 */
#define I2C1_PCLK_DI()			( RCC->APB1ENR &= ~(1<<21) )
#define I2C2_PCLK_DI()			( RCC->APB1ENR &= ~(1<<22) )


/*
 *  Clock Enable Macros for SPIx peripherals
 */
#define SPI1_PCLK_DI()			( RCC->APB2ENR &= ~(1<<12) )
#define SPI2_PCLK_DI()			( RCC->APB1ENR &= ~(1<<14) )
#define SPI3_PCLK_DI()			( RCC->APB1ENR &= ~(1<<15) )


/*
 *  Clock Enable Macros for USARTx peripherals
 */
#define USART1_PCLK_DI()			( RCC->APB2ENR &= ~(1<<14) )
#define USART2_PCLK_DI()			( RCC->APB1ENR &= ~(1<<17) )
#define USART3_PCLK_DI()			( RCC->APB1ENR &= ~(1<<18) )
#define UART4_PCLK_DI()				( RCC->APB1ENR &= ~(1<<19) )
#define UART5_PCLK_DI()				( RCC->APB1ENR &= ~(1<<20) )




/*
 *  Macros for reset GPIOx peripherals
 */
#define GPIOA_REG_RESET()		do{ (RCC->APB2ENR |= (1<<2));  (RCC->APB2ENR &= ~(1<<2)); }while(0)
#define GPIOB_REG_RESET()		do{ (RCC->APB2ENR |= (1<<3));  (RCC->APB2ENR &= ~(1<<3)); }while(0)
#define GPIOC_REG_RESET()		do{ (RCC->APB2ENR |= (1<<4));  (RCC->APB2ENR &= ~(1<<4)); }while(0)
#define GPIOD_REG_RESET()		do{ (RCC->APB2ENR |= (1<<5));  (RCC->APB2ENR &= ~(1<<5)); }while(0)
#define GPIOE_REG_RESET()		do{ (RCC->APB2ENR |= (1<<6));  (RCC->APB2ENR &= ~(1<<6)); }while(0)
#define GPIOF_REG_RESET()		do{ (RCC->APB2ENR |= (1<<7));  (RCC->APB2ENR &= ~(1<<7)); }while(0)
#define GPIOG_REG_RESET()		do{ (RCC->APB2ENR |= (1<<8));  (RCC->APB2ENR &= ~(1<<8)); }while(0)

/*
 * Generic Macros
 */
#define ENABLE 				1
#define DISABLE 			0
#define SET 				ENABLE
#define RESET				DISABLE
#define GPIO_PIN_SET 		SET
#define GPIO_PIN_RESET 		RESET

#include "../drivers/Inc/stm32f103xx_gpio_driver.h"

#endif /* INC_STM32F103XX_H_ */
