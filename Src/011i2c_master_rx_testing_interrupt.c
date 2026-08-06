/*
 *   010i2c_master_rx_testing_interrupt.c
 *
 *  Created on		: 2 Aug 2026
 *  Author			: Tara Alexander
 */

#include<stdio.h>
#include <stm32f103xx.h>
#include <stm32f103xx_i2c_driver.h>
#include <string.h>

#define MY_ADDRESS 			0x61
#define SLAVE_ADDR 			0x68
#define COMMAND_FETCH_LEN	0x51
#define COMMAND_FETCH_DATA	0x52

I2C_Handle_t I2C1Handle;

uint8_t rcv_buff[32];
uint8_t rcvComplete = RESET;

void delay() {
	for (uint32_t i = 0; i < 500000 / 2; i++)
		;
}
extern void initialise_monitor_handles();
/*
 * PB6  ----> I2C1_SCL
 * PB7  ----> I2C1_SDA
 */
void I2C1_GPIOInits(void) {
	GPIO_Handle_t I2CPins;

	I2CPins.pGPIOx = GPIOB;

	I2CPins.GPIO_PinConfig.GPIO_PinMode = GPIO_MODE_OUT_50;
	I2CPins.GPIO_PinConfig.GPIO_PinCnf = GPIO_CNF_AF_OD;

	/* =========================
	 * SCL (PB8)
	 * ========================= */
	I2CPins.GPIO_PinConfig.GPIO_PinNumber = GPIO_PIN_NO_6;
	GPIO_Init(&I2CPins);

	/* =========================
	 * SDA (PB9)
	 * ========================= */
	I2CPins.GPIO_PinConfig.GPIO_PinNumber = GPIO_PIN_NO_7;
	GPIO_Init(&I2CPins);

}
void I2C1_Inits() {

	I2C1Handle.pI2Cx = I2C1;
	I2C1Handle.I2CConfig.I2C_ACKControl = I2C_ENABLE_ACK;
	I2C1Handle.I2CConfig.I2C_DeviceAddress = MY_ADDRESS;
	I2C1Handle.I2CConfig.I2C_FMDutycycle = I2C_FM_DUTY_2;
	I2C1Handle.I2CConfig.I2C_SCLSpeed = I2C_SCL_SPEED_SM;
	I2C_Init(&I2C1Handle);

}

void GPIO_ButtonInit(void) {

	GPIO_Handle_t GPIOBtn;
	GPIOBtn.pGPIOx = GPIOC;
	GPIOBtn.GPIO_PinConfig.GPIO_PinNumber = GPIO_PIN_NO_13;
	GPIOBtn.GPIO_PinConfig.GPIO_PinMode = GPIO_MODE_IN;
	GPIOBtn.GPIO_PinConfig.GPIO_PinCnf = GPIO_CNF_IN_PUPD;
	GPIOBtn.GPIO_PinConfig.GPIO_PinPuPdControl = GPIO_PULLUP;
	GPIO_Init(&GPIOBtn);

}

int main(void) {

	uint8_t commandcode;

	uint8_t len;

	GPIO_ButtonInit();


	//I2C pin inits
	I2C1_GPIOInits();

	//I2C peripheral config
	I2C1_Inits();

	initialise_monitor_handles();
		printf("Application is running");

	//I2C IRQ Config
	I2C_IRQInterruptConfig(IRQ_NO_I2C1_EV, ENABLE);
	I2C_IRQInterruptConfig(IRQ_NO_I2C1_ER, ENABLE);

	//Enable the I2C peripheral
	I2C_PeripheralControl(I2C1, ENABLE);

	//ACK bit is set after PE=1
	I2C_ManageAcking(I2C1, I2C_ENABLE_ACK);

	while ( 1 ) {

		//Wait till button is pressed
		while ( (GPIO_ReadFromInputPin(GPIOC, GPIO_PIN_NO_13)) )
			;

		//to avoid button de-bouncing related issues 200 ms of delay
		delay();

		commandcode = 0x51;
		//Send some data
		while ( I2C_MasterSendDataIT(&I2C1Handle, &commandcode, 1, SLAVE_ADDR,
		I2C_ENABLE_SR) != I2C_READY )
			;

		delay();
		delay();
		while ( I2C_MasterReceiveDataIT(&I2C1Handle, &len, 1, SLAVE_ADDR,
		I2C_ENABLE_SR) != I2C_READY )
			;

		commandcode = 0x52;
		while ( I2C_MasterSendDataIT(&I2C1Handle, &commandcode, 1, SLAVE_ADDR,
		I2C_ENABLE_SR) != I2C_READY )
			;

		delay();
		delay();

		while ( I2C_MasterReceiveDataIT(&I2C1Handle, rcv_buff, len, SLAVE_ADDR,
		I2C_DISABLE_SR) != I2C_READY )
			;

		rcvComplete = RESET;
		while ( rcvComplete != SET )
			;
		rcv_buff[len + 1] = '\0';
		printf("%s", rcv_buff);

		rcvComplete = RESET;
	}

}

void I2C1_EV_IRQHandler(void) {
	I2C_EV_IRQHandling(&I2C1Handle);

}
void I2C1_ER_IRQHandler(void) {
	I2C_ER_IRQHandling(&I2C1Handle);
}

void I2C_ApplicationEventCallback(I2C_Handle_t *pI2CHandle, uint8_t AppEv) {
	if (AppEv == I2C_EV_TX_CMPLT) {
		printf("Tx is completed\n");
	} else if (AppEv == I2C_EV_RX_CMPLT) {
		printf("Rx is completed\n");
		rcvComplete = SET;
	} else if (AppEv == I2C_ERROR_AF) {
		printf("Error:ACK failure\n");
		//In Master ACK failure happens when slave fails to send ACK for the byte sent from the master.
		I2C_CloseSendData(pI2CHandle);

		//Generate the STOP condition to release the bus condition
		I2C_GenerateStopCondition(I2C1);

	}
}
