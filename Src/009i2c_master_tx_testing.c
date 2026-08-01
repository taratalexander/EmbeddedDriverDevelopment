/*
 *   009i2c_master_tx_testing.c
 *
 *  Created on		: 31 Jul 2026
 *  Author			: Tara Alexander
 */

#include<stdio.h>
#include <stm32f103xx.h>
#include <stm32f103xx_i2c_driver.h>
#include <string.h>

#define MY_ADDRESS 0x61
#define SLAVE_ADDR 0x68
I2C_Handle_t I2C1Handle;

uint8_t test_data[] = "Hello Arduino, from STM32\n";
void delay() {
	for (uint32_t i = 0; i < 500000 / 2; i++)
		;
}

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
	I2C1Handle.I2CConfig.I2C_ACKControl = I2C_ACK_ENABLE;
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

	GPIO_ButtonInit();
	//i2c pin inits
	I2C1_GPIOInits();

	//i2c peripheral config
	I2C1_Inits();

	//Enable the I2C peripheral
	I2C_PeripheralControl(I2C1, ENABLE);
	I2C_ManageAcking(I2C1, I2C_ACK_ENABLE);
	while (1) {

		//Wait till button is pressed
		while ((GPIO_ReadFromInputPin(GPIOC, GPIO_PIN_NO_13)));

		//to avoid button de-bouncing related issues 200 ms of delay
		delay();

		//Send some data
		I2C_MasterSendData(&I2C1Handle, test_data, strlen((char*) test_data),
				SLAVE_ADDR);
	}

}
