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

// The GPS object
TinyGPSPlus myGPS;

/** GPS polling function */
bool pollGPS(void);

/** Location data as byte array */
tracker_data_s trackerData;

/**
 * @brief Initialize the GPS
 * 
 */
void initGPS(void)
{
	pinMode(17, OUTPUT);
	digitalWrite(17, HIGH);
	pinMode(34, OUTPUT);
	digitalWrite(34, LOW);
	delay(1000);
	digitalWrite(34, HIGH);
	delay(2000);

	// Initialize connection to GPS module
	Serial1.begin(9600);
	while (!Serial1)
		;
}

/**
 * @brief Check GPS module for position
 * 
 * @return true Valid position found
 * @return false No valid position
 */
bool pollGPS(void)
{
	time_t timeout = millis();
	bool hasPos = false;
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

	digitalWrite(LED_BUILTIN, HIGH);
	while ((millis() - timeout) < 10000)
	{
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

	digitalWrite(LED_BUILTIN, LOW);
	delay(10);
	Serial.println("GPS poll finished ");
	if (hasPos && myGPS.location.isValid())
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
	}
	else
	{
		Serial.printf("No valid location found\n");
	}

	if (hasPos)
	{
		return true;
	}
	return false;
}