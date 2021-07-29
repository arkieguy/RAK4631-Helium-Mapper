/**
 * @file gps.cpp
 * @author Bernd Giesecke (bernd.giesecke@rakwireless.com)
 * @brief GPS functions and task
 * @version 0.1
 * @date 2020-07-24
 * 
 * @copyright Copyright (c) 2020
 * 
 */
#include "main.h"

// The GPS objects for the different modules options
TinyGPSPlus myGPS;			// RAK1910_GPS
SFE_UBLOX_GNSS g_myGNSS;	// RAK12500_GPS

/** GPS polling function */
bool pollGPS(void);

/** Location data as byte array */
tracker_data_s trackerData;

/**
 * @brief Initialize the GPS
 * 
 * @return The GPS module to module option detected
 * 
 */
uint8_t initGPS(void)
{
	pinMode(17, OUTPUT);
	digitalWrite(17, HIGH);
	pinMode(34, OUTPUT);
	digitalWrite(34, LOW);
	delay(1000);
	digitalWrite(34, HIGH);
	delay(2000);

	// Initialize connection to GPS module
	// First, check if the RAK12500 is connected, otherwise initialize the
	// RAK1910
	Serial.println(F("Trying to initialize RAK12500"));
	Wire.begin();
	if (g_myGNSS.begin() == false) //Connect to the u-blox module using Wire port
	{
		Serial.println(F("u-blox GNSS not detected at default I2C address"));
	} else {
		g_myGNSS.setI2COutput(COM_TYPE_UBX); //Set the I2C port to output UBX only (turn off NMEA noise)
		g_myGNSS.saveConfigSelective(VAL_CFG_SUBSEC_IOPORT); //Save (only) the communications port settings to flash and BBR
		Serial.println(F("Detected RAK12500_GPS"));
		return RAK12500_GPS;
	}
	Wire.end();

	Serial.println(F("Initialize RAK1910"));
	Serial1.begin(9600);
	while (!Serial1)
		;
	return RAK1910_GPS;
}

/**
 * @brief Check GPS module for position
 * 
 * @param gpsOption The GPS module to use
 * 
 * @return true Valid position found
 * @return false No valid position
 */
bool pollGPS(uint8_t gpsOption)
{
	time_t timeout = millis();
	bool hasPos = false;	// This flag indicate if the GPS fix is valid by checking the .getGnssFixOk() or .location.isValid() methods.
	bool hasAlt = false;
	bool hasTime = false;
	bool hasSpeed = false;
	bool hasHdop = false;
	bool hasSats = false;
	int64_t latitude = 0;
	int64_t longitude = 0;
	int32_t altitude = 0;
	uint16_t speed = 0;
	uint8_t hdop = 0;
	uint8_t sats = 0;

	Serial.println(F("Trying to get fix from GPS"));
	if (bleUARTisConnected) {
		bleuart.print("Trying to get fix from GPS\n");
	}

	digitalWrite(LED_BUILTIN, HIGH);

	// Poll the GPS according to the initialized module
	switch(gpsOption) {

		case RAK1910_GPS:

			Serial.println(F("Polling RAK1910"));
			if (bleUARTisConnected) {
				bleuart.print("Polling RAK1910\n");
			}

			while ((millis() - timeout) < 10000) {
				while (Serial1.available() > 0)
				{
					// if (myGPS.encode(ss.read()))
					if (myGPS.encode(Serial1.read()))
					{
						digitalToggle(LED_BUILTIN);
						if (myGPS.location.isUpdated() && myGPS.location.isValid())
						{
							hasPos = true;
							latitude = myGPS.location.lat() * 100000;
							longitude = myGPS.location.lng() * 100000;
						}
						else if (myGPS.altitude.isUpdated() && myGPS.altitude.isValid())
						{
							hasAlt = true;
							altitude = myGPS.altitude.meters();
						}
						else if (myGPS.speed.isUpdated() && myGPS.speed.isValid())
						{
							hasSpeed = true;
							speed = myGPS.speed.mps();
						}
						else if (myGPS.hdop.isUpdated() && myGPS.hdop.isValid())
						{
							hasHdop = true;
							hdop = myGPS.hdop.hdop();
						}
						else if (myGPS.satellites.isUpdated() && myGPS.satellites.isValid()) 
						{
							hasSats = true;
							sats = myGPS.satellites.value();
						}
					}
					// if (hasPos && hasAlt && hasDate && hasTime && hasSpeed && hasHdop)
					if (hasPos && hasAlt && hasSpeed && hasHdop && hasTime && hasSats)
					{
						break;
					}
				}
				if (hasPos && hasAlt && hasSpeed && hasHdop)
				{
					break;
				}
			}
			break;

		case RAK12500_GPS:

			Serial.println(F("Polling RAK12500"));
			if (bleUARTisConnected) {
				bleuart.print("Polling RAK12500\n");
			}

			if (g_myGNSS.getGnssFixOk()) {
				latitude = g_myGNSS.getLatitude() / 100;
				longitude = g_myGNSS.getLongitude() / 100;
				altitude = g_myGNSS.getAltitude();
				hdop = g_myGNSS.getHorizontalDOP();
				sats = g_myGNSS.getSIV();
				hasPos = true;
			}

			break;

		default:
			Serial.println(F("No valid gpsOption provided"));
			if (bleUARTisConnected) {
				bleuart.print("No valid gpsOption provided\n");
			}
	}

	digitalWrite(LED_BUILTIN, LOW);
	delay(10);
	Serial.println("GPS poll finished ");

	// Set trackerData if the GPS fix is valid.
	if (hasPos)
	{
		Serial.printf("Lat: %.4f Lon: %.4f\n", latitude/100000.0, longitude/100000.0);
		Serial.printf("Alt: %.4f Speed: %.4f\n", altitude * 1.0, speed * 1.0);

		trackerData.lat_1 = latitude;
		trackerData.lat_2 = latitude >> 8;
		trackerData.lat_3 = latitude >> 16;
		trackerData.lat_4 = latitude >> 24;

		trackerData.lng_1 = longitude;
		trackerData.lng_2 = longitude >> 8;
		trackerData.lng_3 = longitude >> 16;
		trackerData.lng_4 = longitude >> 24;

		trackerData.alt_1 = altitude;
		trackerData.alt_2 = altitude >> 8;

		trackerData.hdop = hdop;

		trackerData.sp_1 = speed;
		trackerData.sp_2 = speed >> 8;

		trackerData.sats = sats;

		return true;
	}
	else
	{
		Serial.printf("No valid location found\n");
	}

	return false;
}