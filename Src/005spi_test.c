/*
 *   005spi_test.c
 *
 *  Created on		: 24 Jun 2026
 *  Author			: Tara Alexander
 */

#include <stm32f103xx.h>
#include <string.h>
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
	SPI2Handle.SPIConfig.SPI_SclkSpeed = SPI_SCLK_SPEED_DIV32;
	SPI2Handle.SPIConfig.SPI_DFF = SPI_DFF_8BITS;
	SPI2Handle.SPIConfig.SPI_CPOL = SPI_CPOL_HIGH;
	SPI2Handle.SPIConfig.SPI_CPHA = SPI_CPHA_LOW;
	SPI2Handle.SPIConfig.SPI_SSM = SPI_SSM_EN;

	SPI_Init(&SPI2Handle);

}

int main(void) {

	char user_data[] = "Hello World";

	//This function is used to initialise the GPIO pins to behave as SPI2 pins
	SPI_GPIOInits();

	//This function is used to initialise the SPI2 peripheral parameters
	SPI2_Inits();


	//This function makes NSS internally high and avoid MODF error
	SPI_SSIConfig(SPI2, ENABLE);

	//This function is used to enable the SPI2 peripheral in CR1
	SPI_PeripheralControl(SPI2, ENABLE);

	SPI_SendData(SPI2, (uint8_t*)user_data, strlen(user_data));

	//This function is used to disable the SPI2 peripheral in CR1
	SPI_PeripheralControl(SPI2, DISABLE);

	while(1);
	return 0;
}
