/**
 * @file main.cpp
 * @author Bernd Giesecke (bernd@giesecke.tk)
 * @brief Arduino framework setup and main functions
 * @version 0.1
 * @date 2020-07-11
 * 
 * @copyright Copyright (c) 2020
 * 
 */

#include "main.h"

/** Buffer for debug output */
char dbgBuffer[256] = {0};

/** Battery level in percentage */
uint8_t battLevel = 0;

/** Flag if OTAA join message was shown */
bool msgJoined = false;
/** Flag if initial LoRaWan package was sent */
bool initMsg = false;

/** Timer since last position message was sent */
time_t lastPosSend = 0;
/** Timer for delayed sending to keep duty cycle */
SoftwareTimer delayedSending;
/** Timer for periodic sending */
SoftwareTimer periodicSending;

// Forward declaration
void sendDelayed(TimerHandle_t unused);
void sendPeriodic(TimerHandle_t unused);

/**
 * @brief Arduino setup function
 * @note Initialize peripherals
 * 
 */
void setup()
{
	pinMode(LED_BUILTIN, OUTPUT);
	digitalWrite(LED_BUILTIN, HIGH);

	pinMode(LED_CONN, OUTPUT);
	digitalWrite(LED_CONN, LOW);

	pinMode(17, OUTPUT);
	digitalWrite(17, HIGH);
	pinMode(34, OUTPUT);
	digitalWrite(34, LOW);
	delay(1000);
	digitalWrite(34, HIGH);
	delay(2000);

	initDisplay();
	dispWriteHeader();

	// Initialize Serial for debug output
	Serial.begin(115200);

	time_t timeout = millis();
	dispAddLine((char *)"Waiting for Serial");
	while (!Serial)
	{
		if ((millis() - timeout) < 5000)
		{
			delay(100);
			digitalWrite(LED_BUILTIN, !digitalRead(LED_BUILTIN));
		}
		else
		{
			break;
		}
	}

	digitalWrite(LED_BUILTIN, LOW);
	Serial.println("=====================================");
	Serial.println("RAK4631 LoRaWan tracker");
	Serial.println("=====================================");

	// Start BLE
	dispAddLine((char *)"Init BLE");
	initBLE();

	pinMode(37, OUTPUT);
	digitalWrite(37, HIGH);

	// Initialize battery level functions
	dispAddLine((char *)"Init Batt");
	initReadVBAT();

	// Initialize accelerometer
	dispAddLine((char *)"Init ACC");
	if (!initACC())
	{
		Serial.println("ACC init failed");
		dispAddLine((char *)"ACC init failed");
	}

	// Initialize GPS module
	dispAddLine((char *)"Init GPS");
	initGPS();

	digitalWrite(LED_BUILTIN, HIGH);

	// Initialize LoRaWan and start join request
	dispAddLine((char *)"Init LoRaWan");
	uint8_t loraInitResult = initLoRaHandler();

	if (loraInitResult != 0)
	{
		switch (loraInitResult)
		{
		case 0:
			sprintf(dbgBuffer, "LoRaWan success");
			break;
		case 1:
			sprintf(dbgBuffer, " HW init failed");
			break;
		case 2:
			sprintf(dbgBuffer, "LoRaWan failed");
			break;
		case 3:
			sprintf(dbgBuffer, "Subband error");
			break;
		case 4:
			sprintf(dbgBuffer, "LoRa Task error");
			break;
		}
		dispAddLine(dbgBuffer);
		Serial.println("dbgBuffer");
	}

	// Prepare timers
	delayedSending.begin(10000, sendDelayed, NULL, false);
	periodicSending.begin(60000, sendPeriodic);
	periodicSending.start();
}

/**
 * @brief Timer function used to avoid sending packages too often.
 * 			Delays the next package by 10 seconds
 * 
 * @param unused 
 * 			Timer handle, not used
 */
void sendDelayed(TimerHandle_t unused)
{
	xSemaphoreGiveFromISR(loopEnable, &xHigherPriorityTaskWoken);
}

/**
 * @brief Timer function called frequently every 60 seconds to send
 * 			send the position, independant of any movement
 * 
 * @param unused 
 * 			Timer handle, not used
 */
void sendPeriodic(TimerHandle_t unused)
{
	if ((millis() - lastPosSend) > 10000)
	{
		xSemaphoreGiveFromISR(loopEnable, &xHigherPriorityTaskWoken);
	}
}

/**
 * @brief Arduino main loop
 * @note This loop is repeatedly called
 * 
 */
void loop()
{
	if (xSemaphoreTake(loopEnable, portMAX_DELAY) == pdTRUE)
	{
		Serial.println("Got semaphore");
		if (bleUARTisConnected)
		{
			bleuart.println("Got semaphore");
		}
		clearAccInt();
		if (lmhJoined())
		{
			if (!msgJoined)
			{
				msgJoined = true;
				sprintf(dbgBuffer, "OTAA addr %08lX\n", lmhAddress());
				dispAddLine(dbgBuffer);
				digitalWrite(LED_BUILTIN, LOW);
				if (bleUARTisConnected)
				{
					bleuart.println(dbgBuffer);
				}
				initMsg = true;
			}
			if (((millis() - lastPosSend) > 10000) || initMsg)
			{
				initMsg = false;
				Serial.println("More than 10 seconds since last position message, send now");
				if (bleUARTisConnected)
				{
					bleuart.println("More than 10 seconds since last position message, send now");
				}
				lastPosSend = millis();
				if (pollGPS())
				{
					Serial.println("Valid GPS position");
					if (bleUARTisConnected)
					{
						bleuart.println("Valid GPS position");
					}
				}
				else
				{
					Serial.println("No valid GPS position");
					if (bleUARTisConnected)
					{
						bleuart.println("No valid GPS position");
					}
				}

				// Get battery level
				battLevel = readBattVolts();
				trackerData.batt = battLevel;
				Serial.println( trackerData.batt );
				// coords[9] = battLevel;

				// Send the location information
				sendLoRaFrame();
			}
			else
			{
				Serial.println("Less than 10 seconds since last position message, send delayed");
				delayedSending.stop();
				delayedSending.start();
			}
		}
		else
		{
			Serial.println("Did not join network yet!");
		}
		// Take the semaphore. Will be given back from the interrupt callback function
		xSemaphoreTake(loopEnable, (TickType_t)10);
	}

	delay(10);
}
