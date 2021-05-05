/**
 * @file acc.cpp
 * @author Bernd Giesecke (bernd.giesecke@rakwireless.com)
 * @brief 3-axis accelerometer functions
 * @version 0.1
 * @date 2020-07-24
 * 
 * @copyright Copyright (c) 2020
 * 
 */
#include "main.h"

void accIntHandler(void);

/** The LIS3DH sensor */
LIS3DH accSensor(I2C_MODE, 0x18);

/** Semaphore to wake up the main loop */
SemaphoreHandle_t loopEnable;

/** Required for give semaphore from ISR */
BaseType_t xHigherPriorityTaskWoken = pdFALSE;

/**
 * @brief Initialize LIS3DH 3-axis 
 * acceleration sensor
 * 
 * @return true If sensor was found and is initialized
 * @return false If sensor initialization failed
 */
bool initACC(void)
{
	// Setup interrupt pin
	pinMode(INT1_PIN, INPUT);

	accSensor.settings.accelSampleRate = 10; //Hz.  Can be: 0,1,10,25,50,100,200,400,1600,5000 Hz
	accSensor.settings.accelRange = 2;		 //Max G force readable.  Can be: 2, 4, 8, 16

	accSensor.settings.adcEnabled = 0;
	accSensor.settings.tempEnabled = 0;
	accSensor.settings.xAccelEnabled = 1;
	accSensor.settings.yAccelEnabled = 1;
	accSensor.settings.zAccelEnabled = 1;

	if (accSensor.begin() != 0)
	{
		return false;
	}

	uint8_t dataToWrite = 0;
	dataToWrite |= 0x20;								   //Z high
	dataToWrite |= 0x08;								   //Y high
	dataToWrite |= 0x02;								   //X high
	accSensor.writeRegister(LIS3DH_INT1_CFG, dataToWrite); // Enable interrupts on high tresholds for x, y and z

	dataToWrite = 0;
	//dataToWrite |= 0x10;								   // Previous Value
	dataToWrite |= 0x02;								   // Increased Sensitivity
	accSensor.writeRegister(LIS3DH_INT1_THS, dataToWrite); // 1/8th range

	dataToWrite = 0;
	dataToWrite |= 0x01; // 1 * 1/50 s = 20ms
	accSensor.writeRegister(LIS3DH_INT1_DURATION, dataToWrite);

	accSensor.readRegister(&dataToWrite, LIS3DH_CTRL_REG5);
	dataToWrite &= 0xF3;									//Clear bits of interest
	dataToWrite |= 0x08;									//Latch interrupt (Cleared by reading int1_src)
	accSensor.writeRegister(LIS3DH_CTRL_REG5, dataToWrite); // Set interrupt to latching

	dataToWrite = 0;
	dataToWrite |= 0x40; //AOI1 event (Generator 1 interrupt on pin 1)
	dataToWrite |= 0x20; //AOI2 event ()
	accSensor.writeRegister(LIS3DH_CTRL_REG3, dataToWrite);

	accSensor.writeRegister(LIS3DH_CTRL_REG6, 0x00); // No interrupt on pin 2

	accSensor.writeRegister(LIS3DH_CTRL_REG2, 0x01); // Enable high pass filter

	// Create the semaphore
	loopEnable = xSemaphoreCreateBinary();
	xSemaphoreGive(loopEnable);

	// Take the semaphore. Will be given back from the interrupt callback function
	xSemaphoreTake(loopEnable, (TickType_t)10);

	clearAccInt();

	// Set the interrupt callback function
	attachInterrupt(INT1_PIN, accIntHandler, RISING);

	return true;
}

/**
 * @brief ACC interrupt handler
 * @note gives semaphore to wake up main loop
 * 
 */
void accIntHandler(void)
{
	xSemaphoreGiveFromISR(loopEnable, &xHigherPriorityTaskWoken);
}

/**
 * @brief Clear ACC interrupt register to enable next wakeup
 * 
 */
void clearAccInt(void)
{
	uint8_t dataRead;
	accSensor.readRegister(&dataRead, LIS3DH_INT1_SRC);
	if (dataRead & 0x40)
		Serial.printf("Interrupt Active 0x%X\n", dataRead);
	if (dataRead & 0x20)
		Serial.println("Z high");
	if (dataRead & 0x10)
		Serial.println("Z low");
	if (dataRead & 0x08)
		Serial.println("Y high");
	if (dataRead & 0x04)
		Serial.println("Y low");
	if (dataRead & 0x02)
		Serial.println("X high");
	if (dataRead & 0x01)
		Serial.println("X low");
}
