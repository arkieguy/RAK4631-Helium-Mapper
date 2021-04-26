/**
 * @file bat.cpp
 * @author Bernd Giesecke (bernd@giesecke.tk)
 * @brief Battery level functions
 * @version 0.1
 * @date 2020-07-11
 * 
 * @copyright Copyright (c) 2020
 * 
 */
#include "main.h"

/**
 * @brief Returns battery status as value between 0 and 255
 * 
 * @return uint8_t battery status
 */
uint8_t lorawanBattLevel(void)
{
	return (uint8_t)(readBatt() * 2.55);
}

/**
 * @brief Read battery voltage from vbat_pin analog input
 * 
 * @return float value of converted raw value to compensated mv, taking the 
 * resistor-divider into account (providing the actual LIPO voltage)
 */
float readVBAT(void)
{
	float raw;

	// Get the raw 12-bit, 0..3000mV ADC value
	raw = analogRead(PIN_VBAT);

	// Convert the raw value to compensated mv, taking the resistor-
	// divider into account (providing the actual LIPO voltage)
	// ADC range is 0..3000mV and resolution is 12-bit (0..4095)
	return raw * REAL_VBAT_MV_PER_LSB;
}

/**
 * @brief Converts mV to percentage of battery level
 * 
 * @param mvolts  voltage in milli volt
 * @return uint8_t battery charge as percent
 */
uint8_t mvToPercent(float mvolts)
{
	if (mvolts < 3300)
		return 0;

	if (mvolts < 3600)
	{
		mvolts -= 3300;
		return mvolts / 30;
	}

	mvolts -= 3600;
	return 10 + (mvolts * 0.15F); // thats mvolts /6.66666666
}

/**
 * @brief Convert battery level into LoRaWan battery level
 * LoRaWan expects the battery level as a value from 0 to 255
 * where 255 equals 100% battery level
 * 
 * @param mvolts voltage in milli volt
 * @return uint8_t battery charge as value between 0 and 255
 */
uint8_t mvToLoRaWanBattVal(float mvolts)
{ 
	if (mvolts < 3300)
		return 0;

	if (mvolts < 3600)
	{
		mvolts -= 3300;
		return mvolts / 30 * 2.55;
	}

	mvolts -= 3600;
	return (10 + (mvolts * 0.15F)) * 2.55;
}

/**
 * @brief  Initialize ADC for reading of 
 * battery values and analog sensor values.
 * Reference voltage is set to 3.0V
 * and resolution is set to 12 bit
 * @note   Dummy for boards that cannot read battery level
 * @retval None
 */
void initReadVBAT(void)
{
	// Set the analog reference to 3.0V (default = 3.6V)
	analogReference(AR_INTERNAL_3_0);

	// Set the resolution to 12-bit (0..4095)
	analogReadResolution(12); // Can be 8, 10, 12 or 14

	// Let the ADC settle
	delay(1);

	// Get a single ADC sample and throw it away
	readVBAT();
}

/**
 * @brief  Read battery value from VBAT_PIN and convert it to %.
 * @retval uint8_t percent
 * 			Battery level as percentage
 */
uint8_t readBatt(void)
{
	float vbat_mv = readVBAT();
	return mvToPercent(vbat_mv);
}

uint8_t readBattVolts(void) 
{
	return( ceil( readVBAT() / 100) );
}
