/*
 *   stm32f103xx_i2c_driver.h
 *
 *  Created on		: 24 July 2026
 *  Author			: Tara Alexander
 */

#ifndef INC_STM32F103XX_I2C_DRIVER_H_
#define INC_STM32F103XX_I2C_DRIVER_H_
#include "stm32f103xx.h"


/*
 * Configuration structure for I2Cx peripheral
 */
typedef struct
{
	uint32_t I2C_SCLSpeed;
	uint8_t I2C_DeviceAddress;
	uint8_t I2C_ACKControl;
	uint8_t I2C_FMDutycycle;
}I2C_Config_T;

/**
 * Handle structure for I2Cx peripheral
 */
typedef struct
{
	I2C_RegDef_t *pI2Cx;  	/* !< Holds the base address of I2Cx peripherals > */
	I2C_Config_T I2CConfig;
	uint8_t *pTxBuffer;		/* !< To store the app. Tx buffer address >*/
	uint8_t *pRxBuffer;		/* !< To store the app. Rx buffer address >*/
	uint32_t TxLen;			/* !< To store the Tx len >*/
	uint32_t RxLen;			/* !< To store the Rx len >*/
	uint8_t TxRxState;		/* !< To store the Communuication State >*/
	uint8_t DevAddr;		/* !< To store the slave/device Address >*/
	uint32_t RxSize;		/* !< To store the Rx Size >*/
	uint8_t Sr;				/* !< To store the repeated start value >*/

}I2C_Handle_t;


/**
 * @I2C_SCLSpeed
 */
#define I2C_SCL_SPEED_SM		100000U
#define I2C_SCL_SPEED_FM4k		400000U
#define I2C_SCL_SPEED_FM2k		200000U


/**
 * @I2C_ACKControl
 */
#define I2C_ENABLE_ACK			1
#define I2C_DISABLE_ACK			0

/**
 * @I2C_RepeatedStart
 */
#define I2C_ENABLE_SR			SET
#define I2C_DISABLE_SR			RESET


/**
 * @I2C_FMDutycycle
 */
#define I2C_FM_DUTY_2			0
#define I2C_FM_DUTY_16_9		1

/**
 * I2C Flag Status Masks
 */
#define I2C_FLAG_SB					(1 << I2C_SR1_SB)
#define I2C_FLAG_ADDR 				(1 << I2C_SR1_ADDR)
#define I2C_FLAG_BTF 				(1 << I2C_SR1_BTF)
#define I2C_FLAG_STOPF 				(1 << I2C_SR1_STOPF)
#define I2C_FLAG_RXNE 				(1 << I2C_SR1_RxNE)
#define I2C_FLAG_TXE 				(1 << I2C_SR1_TxE)
#define I2C_FLAG_BERR 				(1 << I2C_SR1_BERR)
#define I2C_FLAG_ARLO 				(1 << I2C_SR1_ARLO)
#define I2C_FLAG_AF 				(1 << I2C_SR1_AF)
#define I2C_FLAG_OVR 				(1 << I2C_SR1_OVR)
#define I2C_FLAG_TIMEOUT 			(1 << I2C_SR1_TIMEOUT)

/**
 * I2C Application States
 */
#define I2C_READY						0
#define I2C_BUSY_IN_RX					1
#define I2C_BUSY_IN_TX					2

/**
 * Possible I2C Application Events
 */
#define I2C_EV_TX_CMPLT			0
#define I2C_EV_RX_CMPLT			1
#define I2C_EV_STOP				2
#define I2C_ERROR_BERR			3
#define I2C_ERROR_ARLO			4
#define I2C_ERROR_AF			5
#define I2C_ERROR_OVR			6
#define I2C_ERROR_TIMEOUT		7




/***************************************************************************************
 * 								APIs supported by this driver
 * 				For more information about the APIs check the function definition
 ***************************************************************************************/
/*
 * Peripheral Clock Control
 */
void I2C_PeriClockControl(I2C_RegDef_t *pI2Cx,uint8_t EnorDi);


/*
 * I2C Init and De-Init
 */
void I2C_Init(I2C_Handle_t *pI2CHandle);
void I2C_DeInit(I2C_RegDef_t *pI2Cx);

/**
 * Data Send and Receive
 */

void I2C_MasterSendData(I2C_Handle_t *pI2CHandle,uint8_t *pTxBuffer,uint32_t len,uint8_t slaveAddr,uint8_t Sr);
void I2C_MasterReceiveData(I2C_Handle_t *pI2CHandle, uint8_t *pRxBuffer, uint8_t len, uint8_t slaveAddr,uint8_t Sr);

/**
 * Data Send and Receive using Interrupts
 */

uint8_t I2C_MasterSendDataIT(I2C_Handle_t *pI2CHandle,uint8_t *pTxBuffer,uint32_t len,uint8_t slaveAddr,uint8_t Sr);
uint8_t I2C_MasterReceiveDataIT(I2C_Handle_t *pI2CHandle, uint8_t *pRxBuffer, uint8_t len, uint8_t slaveAddr,uint8_t Sr);

void I2C_CloseReceiveData(I2C_Handle_t *pI2CHandle);
void I2C_CloseSendData(I2C_Handle_t *pI2CHandle);

/*
 * IRQ Configuration and ISR handling
 */
void I2C_IRQInterruptConfig(uint8_t IRQNumber,uint8_t EnorDi);
void I2C_IRQPriorityConfig(uint8_t IRQNumber,uint8_t IRQPriority);
void I2C_EV_IRQHandling(I2C_Handle_t *pI2CHandle);
void I2C_ER_IRQHandling(I2C_Handle_t *pI2CHandle);
/*
 * Other peripheral control APIs
 */
void I2C_PeripheralControl(I2C_RegDef_t *pI2Cx,uint8_t EnorDi);
void I2C_ManageAcking(I2C_RegDef_t *pI2Cx,uint8_t EnorDi);
uint8_t I2C_GetFlagStatus(I2C_RegDef_t *pI2Cx,uint32_t flag);
void I2C_GenerateStopCondition(I2C_RegDef_t *pI2Cx);
/*
 * Application Callback
 */
void I2C_ApplicationEventCallback(I2C_Handle_t *pI2CHandle,uint8_t AppEv);
#endif /* INC_STM32F103XX_I2C_DRIVER_H_ */
