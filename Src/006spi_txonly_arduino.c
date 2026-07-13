/*
 *   006spi_txonly_arduino.c
 *
 *  Created on		: 30 Jun 2026
 *  Author			: Tara Alexander
 */

#include <stm32f103xx.h>
#include <stm32f103xx_spi_driver.h>
#include <string.h>
/*
 * PB14  ----> SPI2_MISO
 * PB15  ----> SPI2_MOSI
 * PB13  ----> SPI2_SCK
 * PB12  ----> SPI2_NSS
 *
 */

void delay() {
	for (uint32_t i = 0; i < 500000 / 2; i++)
		;
}
void delay2() {
	for (uint32_t i = 0; i < 500000 / 4; i++)
		;
}

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
	GPIOBtn.GPIO_PinConfig.GPIO_PinPuPdControl= GPIO_PULLUP;

	GPIO_Init(&GPIOBtn);

}

int main(void) {

	char user_data[] = "Hello World";

	GPIO_ButtonInit();
	//This function is used to initialise the GPIO pins to behave as SPI2 pins
	SPI_GPIOInits();

	//This function is used to initialise the SPI2 peripheral parameters
	SPI2_Inits();

	SPI_SSOEConfig(SPI2, ENABLE);

	while (1) {
		while (!(GPIO_ReadFromInputPin(GPIOC, GPIO_PIN_NO_13)));

		delay();

		//This function is used to enable the SPI2 peripheral in CR1
		SPI_PeripheralControl(SPI2, ENABLE);

		//First send length information
		uint8_t dataLen = strlen(user_data);
		SPI_SendData(SPI2, &dataLen, 1);
		delay2();

		//Send the data
		SPI_SendData(SPI2, (uint8_t*) user_data, dataLen);

		//Confirm SPI flag is not busy
		while (SPI_GetFlagStatus(SPI2, SPI_BUSY_FLAG))
			;

		//This function is used to disable the SPI2 peripheral in CR1
		SPI_PeripheralControl(SPI2, DISABLE);
	}

	return 0;
}
