/*
 *   007spi_cmd_handling.c
 *
 *  Created on		: 30 Jun 2026
 *  Author			: Tara Alexander
 */

#include<stdio.h>
#include <stm32f103xx.h>
#include <stm32f103xx_spi_driver.h>
#include <string.h>

extern void initialise_monitor_handles();

//command codes
#define COMMAND_LED_CTRL			0x50
#define COMMAND_SENSOR_READ			0x51
#define COMMAND_LED_READ			0x52
#define COMMAND_PRINT				0x53
#define COMMAND_ID_READ				0x54

#define LED_ON 		1
#define LED_OFF 	0

//Arduino Analog Pins
#define ANALOG_PIN0				0
#define ANALOG_PIN1				1
#define ANALOG_PIN2				2
#define ANALOG_PIN3				3
#define ANALOG_PIN4				4

//Arduino LED
#define LED_PIN 9

//ACK - NACK bytes
#define ACK			0xF5
#define NACK 		0xA5

void delay() {
	for (uint32_t i = 0; i < 500000 / 2; i++)
		;
}

/*
 * PB14  ----> SPI2_MISO
 * PB15  ----> SPI2_MOSI
 * PB13  ----> SPI2_SCK
 * PB12  ----> SPI2_NSS
 *
 */
void SPI_GPIOInits(void) {
	GPIO_Handle_t SPIPins;
	SPIPins.pGPIOx = GPIOB;
	/* =========================
	 * SCK (PB13)
	 * ========================= */
	SPIPins.GPIO_PinConfig.GPIO_PinNumber = GPIO_PIN_NO_13;
	SPIPins.GPIO_PinConfig.GPIO_PinMode = GPIO_MODE_OUT_50;
	SPIPins.GPIO_PinConfig.GPIO_PinCnf = GPIO_CNF_AF_PP;
	GPIO_Init(&SPIPins);

	/* =========================
	 * MOSI (PB15)
	 * ========================= */
	SPIPins.GPIO_PinConfig.GPIO_PinNumber = GPIO_PIN_NO_15;
	SPIPins.GPIO_PinConfig.GPIO_PinMode = GPIO_MODE_OUT_50;
	SPIPins.GPIO_PinConfig.GPIO_PinCnf = GPIO_CNF_AF_PP;
	GPIO_Init(&SPIPins);

	/* =========================
	 * NSS (PB12)
	 * ========================= */
	SPIPins.GPIO_PinConfig.GPIO_PinNumber = GPIO_PIN_NO_12;
	SPIPins.GPIO_PinConfig.GPIO_PinMode = GPIO_MODE_OUT_50;
	SPIPins.GPIO_PinConfig.GPIO_PinCnf = GPIO_CNF_AF_PP;
	GPIO_Init(&SPIPins);

	/* =========================
	 * MISO (PB14)
	 * ========================= */
	SPIPins.GPIO_PinConfig.GPIO_PinNumber = GPIO_PIN_NO_14;
	SPIPins.GPIO_PinConfig.GPIO_PinMode = GPIO_MODE_IN;
	SPIPins.GPIO_PinConfig.GPIO_PinCnf = GPIO_CNF_IN_FLOATING;
	GPIO_Init(&SPIPins);

}
void SPI2_Inits() {

	SPI_Handle_t SPI2Handle;
	SPI2Handle.pSPIx = SPI2;
	SPI2Handle.SPIConfig.SPI_BusConfig = SPI_BUS_CONFIG_FD;
	SPI2Handle.SPIConfig.SPI_DeviceMode = SPI_DEVICE_MODE_MASTER;
	SPI2Handle.SPIConfig.SPI_SclkSpeed = SPI_SCLK_SPEED_DIV128;
	SPI2Handle.SPIConfig.SPI_DFF = SPI_DFF_8BITS;
	SPI2Handle.SPIConfig.SPI_CPOL = SPI_CPOL_LOW;
	SPI2Handle.SPIConfig.SPI_CPHA = SPI_CPHA_LOW;
	SPI2Handle.SPIConfig.SPI_SSM = SPI_SSM_DI; //Hardware slave management enabled for NSS pin

	SPI_Init(&SPI2Handle);

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
uint8_t SPI_VerifyResponse(uint8_t ackbyte) {
	if (ackbyte == ACK) {
		return 1;
	}
	return 0;
}

int main(void) {

	uint8_t dummy_write = 0xff;
	uint8_t dummy_read;

	initialise_monitor_handles();

	printf("Application is running\n");

	GPIO_ButtonInit();
	//This function is used to initialise the GPIO pins to behave as SPI2 pins
	SPI_GPIOInits();

	//This function is used to initialise the SPI2 peripheral parameters
	SPI2_Inits();

	printf("SPI initialized\n");

	SPI_SSOEConfig(SPI2, ENABLE);

	while (1) {

		//Wait till button is pressed
		while (!(GPIO_ReadFromInputPin(GPIOC, GPIO_PIN_NO_13)))
			;

		//to avoid button de-bouncing related issues 200 ms of delay
		delay();

		//This function is used to enable the SPI2 peripheral in CR1
		SPI_PeripheralControl(SPI2, ENABLE);

		//1. CMD_LED_CTRL <pin_no(1)> 		<value(1)>

		uint8_t commandcode = COMMAND_LED_CTRL;
		uint8_t ackbyte;
		uint8_t args[2];

		SPI_SendData(SPI2, &commandcode, 1);

		//do dummy read to clear off the RXNE
		SPI_ReceiveData(SPI2, &dummy_read, 1);

		//send some dummmy bits(1byte) to fetch the response from the slave
		SPI_SendData(SPI2, &dummy_write, 1);

		//read the ACK byte received
		SPI_ReceiveData(SPI2, &ackbyte, 1);

		if (SPI_VerifyResponse(ackbyte)) {
			args[0] = LED_PIN;
			args[1] = LED_ON;
			SPI_SendData(SPI2, args, 2);

		}

		//2. CMD_SENSOR_READ 		< analog pin number(1) >

		//Wait till button is pressed
		while (!(GPIO_ReadFromInputPin(GPIOC, GPIO_PIN_NO_13)))
			;

		//to avoid button de-bouncing related issues 200 ms of delay
		delay();

		commandcode = COMMAND_SENSOR_READ;
		//send command
		SPI_SendData(SPI2, &commandcode, 1);

		//do dummy read to clear off the RXNE
		SPI_ReceiveData(SPI2, &dummy_read, 1);

		//send some dummmy bits(1byte) to fetch the response from the slave
		SPI_SendData(SPI2, &dummy_write, 1);

		//read the ACK byte received
		SPI_ReceiveData(SPI2, &ackbyte, 1);

		if (SPI_VerifyResponse(ackbyte)) {
			args[0] = ANALOG_PIN0;
			SPI_SendData(SPI2, args, 1);
			//do dummy read to clear off the RXNE
			SPI_ReceiveData(SPI2, &dummy_read, 1);

			//Insert some delay so that slave can ready with the data
			delay();

			//send some dummmy bits(1byte) to fetch the response from the slave
			SPI_SendData(SPI2, &dummy_write, 1);

			uint8_t analog_read;
			SPI_ReceiveData(SPI2, &analog_read, 1);
			printf("COMMAND_SENSOR_READ %d\n", analog_read);
		}

		//3.  CMD_LED_READ 	 <pin no(1) >

		//wait till button is pressed
		while (!GPIO_ReadFromInputPin(GPIOA, GPIO_PIN_NO_0))
			;

		//to avoid button de-bouncing related issues 200ms of delay
		delay();

		commandcode = COMMAND_LED_READ;

		//send command
		SPI_SendData(SPI2, &commandcode, 1);

		//do dummy read to clear off the RXNE
		SPI_ReceiveData(SPI2, &dummy_read, 1);

		//Send some dummy byte to fetch the response from the slave
		SPI_SendData(SPI2, &dummy_write, 1);

		//read the ack byte received
		SPI_ReceiveData(SPI2, &ackbyte, 1);

		if (SPI_VerifyResponse(ackbyte)) {
			args[0] = LED_PIN;

			//send arguments
			SPI_SendData(SPI2, args, 1); //sending one byte of

			//do dummy read to clear off the RXNE
			SPI_ReceiveData(SPI2, &dummy_read, 1);

			//insert some delay so that slave can ready with the data
			delay();

			//Send some dummy bits (1 byte) fetch the response from the slave
			SPI_SendData(SPI2, &dummy_write, 1);

			uint8_t led_status;
			SPI_ReceiveData(SPI2, &led_status, 1);
			printf("COMMAND_READ_LED %d\n", led_status);

		}

		//4. CMD_PRINT 		<len(2)>  <message(len) >

		//wait till button is pressed
		while (!GPIO_ReadFromInputPin(GPIOA, GPIO_PIN_NO_0))
			;

		//to avoid button de-bouncing related issues 200ms of delay
		delay();

		commandcode = COMMAND_PRINT;

		//send command
		SPI_SendData(SPI2, &commandcode, 1);

		//do dummy read to clear off the RXNE
		SPI_ReceiveData(SPI2, &dummy_read, 1);

		//Send some dummy byte to fetch the response from the slave
		SPI_SendData(SPI2, &dummy_write, 1);

		//read the ack byte received
		SPI_ReceiveData(SPI2, &ackbyte, 1);

		uint8_t message[] = "Hello ! How are you ??";
		if (SPI_VerifyResponse(ackbyte)) {
			args[0] = strlen((char*) message);

			//send arguments
			SPI_SendData(SPI2, args, 1); //sending length

			//do dummy read to clear off the RXNE
			SPI_ReceiveData(SPI2, &dummy_read, 1);

			delay();

			//send message
			for (int i = 0; i < args[0]; i++) {
				SPI_SendData(SPI2, &message[i], 1);
				SPI_ReceiveData(SPI2, &dummy_read, 1);
			}

			printf("COMMAND_PRINT Executed \n");

		}

		//5. CMD_ID_READ
		//wait till button is pressed
		while (!GPIO_ReadFromInputPin(GPIOA, GPIO_PIN_NO_0))
			;

		//to avoid button de-bouncing related issues 200ms of delay
		delay();

		commandcode = COMMAND_ID_READ;

		//send command
		SPI_SendData(SPI2, &commandcode, 1);

		//do dummy read to clear off the RXNE
		SPI_ReceiveData(SPI2, &dummy_read, 1);

		//Send some dummy byte to fetch the response from the slave
		SPI_SendData(SPI2, &dummy_write, 1);

		//read the ack byte received
		SPI_ReceiveData(SPI2, &ackbyte, 1);

		uint8_t id[11];
		uint32_t i = 0;
		if (SPI_VerifyResponse(ackbyte)) {
			//read 10 bytes id from the slave
			for (i = 0; i < 10; i++) {
				//send dummy byte to fetch data from slave
				SPI_SendData(SPI2, &dummy_write, 1);
				SPI_ReceiveData(SPI2, &id[i], 1);
			}

			id[10] = '\0';

			printf("COMMAND_ID : %s \n", id);

		}

		//Confirm SPI flag is not busy
		while (SPI_GetFlagStatus(SPI2, SPI_BUSY_FLAG))
			;

		//This function is used to disable the SPI2 peripheral in CR1
		SPI_PeripheralControl(SPI2, DISABLE);
		printf("SPI Communication Closed \n");

	}

	return 0;
}
