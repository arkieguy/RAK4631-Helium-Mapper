/**
 * @file loraHandler.cpp
 * @author Bernd Giesecke (bernd@giesecke.tk)
 * @brief Handle LoRaWan in a separate task
 * @version 0.1
 * @date 2020-07-25
 * 
 * @copyright Copyright (c) 2020
 * 
 */
#include "main.h"
#include <LoRaWan-RAK4630.h>

//  Set up your Helium AppEUI, DeviceEUI and AppKey in this file!
#include "config.cpp"

/** Software timer to switch off the LED after sending a LoRaWan package */
SoftwareTimer ledTicker;

// LoRaWan setup definitions
#define SCHED_MAX_EVENT_DATA_SIZE APP_TIMER_SCHED_EVENT_DATA_SIZE /**< Maximum size of scheduler events. */
#define SCHED_QUEUE_SIZE 60										  /**< Maximum number of events in the scheduler queue. */

#define LORAWAN_APP_DATA_BUFF_SIZE 64  /**< Size of the data to be transmitted. */
#define LORAWAN_APP_TX_DUTYCYCLE 30000 /**< Defines the application data transmission duty cycle. 10s, value in [ms]. */
#define APP_TX_DUTYCYCLE_RND 1000	   /**< Defines a random delay for application data transmission duty cycle. 1s, value in [ms]. */
#define JOINREQ_NBTRIALS 8			   /**< Number of trials for the join request. */

/** Lora application data buffer. */
static uint8_t m_lora_app_data_buffer[LORAWAN_APP_DATA_BUFF_SIZE]; ///< Lora user application data buffer.
/** Lora application data structure. */
static lmh_app_data_t m_lora_app_data = {m_lora_app_data_buffer, 0, 0, 0, 0}; ///< Lora user application data structure.

/** LoRaWan callback when join network finished */
static void lorawan_has_joined_handler(void);
/** LoRaWan callback when data arrived */
static void lorawan_rx_handler(lmh_app_data_t *app_data);
/** LoRaWan callback after class change request finished */
static void lorawan_confirm_class_handler(DeviceClass_t Class);
/** LoRaWan Function to send a package */
void sendLoRaFrame(void);

/**@brief Structure containing LoRaWan parameters, needed for lmh_init()
 * 
 * Set structure members to
 * LORAWAN_ADR_ON or LORAWAN_ADR_OFF to enable or disable adaptive data rate
 * LORAWAN_DEFAULT_DATARATE OR DR_0 ... DR_5 for default data rate or specific data rate selection
 * LORAWAN_PUBLIC_NETWORK or LORAWAN_PRIVATE_NETWORK to select the use of a public or private network
 * JOINREQ_NBTRIALS or a specific number to set the number of trials to join the network
 * LORAWAN_DEFAULT_TX_POWER or a specific number to set the TX power used
 * LORAWAN_DUTYCYCLE_ON or LORAWAN_DUTYCYCLE_OFF to enable or disable duty cycles
 *                   Please note that ETSI mandates duty cycled transmissions. 
 */
static lmh_param_t lora_param_init = {LORAWAN_ADR_OFF, HELIUM_DR, LORAWAN_PUBLIC_NETWORK, JOINREQ_NBTRIALS, HELIUM_TX, LORAWAN_DUTYCYCLE_OFF};

/** Structure containing LoRaWan callback functions, needed for lmh_init() */
static lmh_callback_t lora_callbacks = {lorawanBattLevel, BoardGetUniqueId, BoardGetRandomSeed,
										lorawan_rx_handler, lorawan_has_joined_handler, lorawan_confirm_class_handler};


uint32_t nodeDevAddr = 0x26021FB5;
/** Network session key required for ABP network join */
uint8_t nodeNwsKey[16] = {0x32, 0x3D, 0x15, 0x5A, 0x00, 0x0D, 0xF3, 0x35, 0x30, 0x7A, 0x16, 0xDA, 0x0C, 0x9D, 0xF5, 0x3F};
/** Application session key required for ABP network join */
uint8_t nodeAppsKey[16] = {0x3F, 0x6A, 0x66, 0x45, 0x9D, 0x5E, 0xDC, 0xA6, 0x3C, 0xBC, 0x46, 0x19, 0xCD, 0x61, 0xA1, 0x1E};

/** Flag whether to use OTAA or ABP network join method */
bool doOTAA = true;

/** LoRa error code */
uint32_t err_code;

/**
 * @brief LED off function
 */
void ledOff(TimerHandle_t xTimerID)
{
	digitalWrite(LED_BUILTIN, LOW);
}

/**
 * @brief Initialize the LoRaWan
 * 
 * @return uint8_t Result
 * 			0 success
 * 			1 HW init failed
 * 			2 LoRaWan init failed
 * 			3 Wrong sub band channels
 * 			4 LoRa task init failed
 */
uint8_t initLoRaHandler(void)
{
	Serial.println("=====================================");
	Serial.println("SX126x initialization");
	Serial.println("=====================================");

	// Initialize LoRa chip.
	err_code = lora_rak4630_init();
	if (err_code != 0)
	{
		return 1;
	}

	// Setup the EUIs and Keys
	lmh_setDevEui(nodeDeviceEUI);
	lmh_setAppEui(nodeAppEUI);
	lmh_setAppKey(nodeAppKey);
	lmh_setNwkSKey(nodeNwsKey);
	lmh_setAppSKey(nodeAppsKey);
	lmh_setDevAddr(nodeDevAddr);

	// Initialize LoRaWan
	err_code = lmh_init(&lora_callbacks, lora_param_init, doOTAA, CLASS_A, HELIUM_REGION);
	if (err_code != 0)
	{
		return 2;
	}

	//!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
	// Use either
	// lmh_setSingleChannelGateway
	// or
	// lmh_setSubBandChannels
	//
	// DO NOT USE BOTH OR YOUR COMMUNICATION WILL MOST LIKELY NEVER WORK
	//!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
	// Setup connection to a single channel gateway
	// lmh_setSingleChannelGateway(0, DR_3);

	// For some regions we might need to define the sub band the gateway is listening to
	// This must be called AFTER lmh_init()
	/// \todo This is for Dragino LPS8 gateway. How about other gateways???
	if (!lmh_setSubBandChannels(2))
	{
		return 3;
		Serial.println("lmh_setSubBandChannels failed. Wrong sub band requested?");
	}

	// Start Join procedure
	Serial.println("Start network join request");
	lmh_join();

	ledTicker.begin(1000, ledOff, NULL, false);

	return 0;
}

/**
 * @brief LoRa function for handling HasJoined event.
 */
static void lorawan_has_joined_handler(void)
{

	if (doOTAA)
	{
		uint32_t otaaDevAddr = lmh_getDevAddr();
		Serial.printf("OTAA joined and got dev address %08X\n", otaaDevAddr);

		if (bleUARTisConnected)
		{
			bleuart.printf("OTAA joined\n");
			bleuart.printf("Dev addr %08lX\n", otaaDevAddr);
		}
	}
	else
	{
		Serial.println("ABP joined");
		if (bleUARTisConnected)
		{
			bleuart.println("ABP joined");
		}
	}
	lmh_class_request(CLASS_C);
}

/**
 * @brief Function for handling LoRaWan received data from Gateway
 *
 * @param[in] app_data  Pointer to rx data
 */
static void lorawan_rx_handler(lmh_app_data_t *app_data)
{
	Serial.printf("LoRa Packet received on port %d, size:%d, rssi:%d, snr:%d\n",
				  app_data->port, app_data->buffsize, app_data->rssi, app_data->snr);
	char nullTerm[app_data->buffsize + 2] = {0};

	if (bleUARTisConnected)
	{
		snprintf(dbgBuffer, 255, "LoRa Packet received on");
		bleuart.print(dbgBuffer);
		delay(100);
		snprintf(dbgBuffer, 255, " port %d, size:%d,",
				 app_data->port, app_data->buffsize);
		bleuart.print(dbgBuffer);
		delay(100);
		snprintf(dbgBuffer, 255, " rssi:%d, snr:%d\n",
				 app_data->rssi, app_data->snr);
		bleuart.println(dbgBuffer);
		delay(100);
	}
	sprintf(dbgBuffer, "DWN RSSI %d, SNR %d\n", app_data->rssi, app_data->snr);
	dispAddLine(dbgBuffer);

	switch (app_data->port)
	{
	case 3:
		// Port 3 switches the class
		if (app_data->buffsize == 1)
		{
			switch (app_data->buffer[0])
			{
			case 0:
				lmh_class_request(CLASS_A);
				break;

			case 1:
				lmh_class_request(CLASS_B);
				break;

			case 2:
				lmh_class_request(CLASS_C);
				break;

			default:
				break;
			}
		}
		break;

	case LORAWAN_APP_PORT:
		// YOUR_JOB: Take action on received data
		for (int i = 0; i <= app_data->buffsize; i++)
		{
			Serial.printf("0x%0X ", app_data->buffer[i]);
		}
		Serial.println();

		snprintf(nullTerm, app_data->buffsize + 1, (const char *)app_data->buffer);
		Serial.printf(">>%s<<\n", nullTerm);

		if (bleUARTisConnected)
		{
			for (int i = 0; i <= app_data->buffsize; i++)
			{
				bleuart.printf("0x%0X ", app_data->buffer[i]);
			}
			bleuart.println();

			bleuart.printf(">>%s<<\n", nullTerm);
		}
		break;

	default:
		break;
	}
}

/**
 * @brief Callback for class switch confirmation
 * 
 * @param Class The new class
 */
static void lorawan_confirm_class_handler(DeviceClass_t Class)
{
	Serial.printf("switch to class %c done\n", "ABC"[Class]);

	if (bleUARTisConnected)
	{
		bleuart.printf("switch to class %c done\n", "ABC"[Class]);
	}
	// Informs the server that switch has occurred ASAP
	m_lora_app_data.buffsize = 0;
	m_lora_app_data.port = LORAWAN_APP_PORT;
	lmh_send(&m_lora_app_data, HELIUM_CONFIRM );
	xSemaphoreGive(loopEnable);
}

/**
 * @brief Send a LoRaWan package
 * 
 */
void sendLoRaFrame(void)
{
	if (lmh_join_status_get() != LMH_SET)
	{
		//Not joined, try again later
		Serial.println("Did not join network, skip sending frame");
		if (bleUARTisConnected)
		{
			bleuart.println("Did not join network, skip sending frame");
		}
		return;
	}

	// Switch on the indicator lights
	digitalWrite(LED_BUILTIN, HIGH);

	m_lora_app_data.port = LORAWAN_APP_PORT;

	memcpy(m_lora_app_data_buffer, (const void *)&trackerData, TRACKER_DATA_LEN);
	m_lora_app_data.buffsize = TRACKER_DATA_LEN; // TRACKER_DATA_LEN;

	lmh_error_status error = lmh_send(&m_lora_app_data, LMH_UNCONFIRMED_MSG);

	sprintf(dbgBuffer, "UP Lat %.4f Lon %.4f Alt %d Pr %d B %u%%\n",
			(trackerData.lat_1 | trackerData.lat_2 << 8 | trackerData.lat_3 << 16 | trackerData.lat_4 << 24 | (trackerData.lat_4 & 0x80 ? 0xFF << 24 : 0)) / 100000.0,
			(trackerData.lng_1 | trackerData.lng_2 << 8 | trackerData.lng_3 << 16 | trackerData.lng_4 << 24 | (trackerData.lng_4 & 0x80 ? 0xFF << 24 : 0)) / 100000.0,
			(trackerData.alt_1 | trackerData.alt_2 << 8 | (trackerData.alt_2 & 0x80 ? 0xFF << 16 : 0)),
			trackerData.hdop,
			trackerData.batt);
	Serial.print(dbgBuffer);
	if (error == LMH_SUCCESS)
	{
		sprintf(dbgBuffer, "UP Lat %.6f\n",
				(trackerData.lat_1 | trackerData.lat_2 << 8 | trackerData.lat_3 << 16 | trackerData.lat_4 << 24 | (trackerData.lat_4 & 0x80 ? 0xFF << 24 : 0)) / 100000.0);
		dispAddLine(dbgBuffer);
		if (bleUARTisConnected)
		{
			bleuart.printf(dbgBuffer);
		}
		sprintf(dbgBuffer, "UP Lon %.6f\n",
				(trackerData.lng_1 | trackerData.lng_2 << 8 | trackerData.lng_3 << 16 | trackerData.lng_4 << 24 | (trackerData.lng_4 & 0x80 ? 0xFF << 24 : 0)) / 100000.0);
		dispAddLine(dbgBuffer);
		if (bleUARTisConnected)
		{
			bleuart.printf(dbgBuffer);
		}
		sprintf(dbgBuffer, "UP Alt %d Pr %d\n",
				(trackerData.alt_1 | trackerData.alt_2 << 8 | (trackerData.alt_2 & 0x80 ? 0xFF << 16 : 0)),
				trackerData.hdop);
		dispAddLine(dbgBuffer);
		if (bleUARTisConnected)
		{
			bleuart.printf(dbgBuffer);
		}
		sprintf(dbgBuffer, "UP B %u%%\n",
				trackerData.batt);
		dispAddLine(dbgBuffer);
		if (bleUARTisConnected)
		{
			bleuart.printf(dbgBuffer);
		}
	}
	else
	{
		Serial.printf("UP failed %d\n", error);
		sprintf(dbgBuffer, "UP failed %d", error);
		dispAddLine(dbgBuffer);
		if (bleUARTisConnected)
		{
			bleuart.printf("UP result %d\n", error);
		}
	}

	// Start the timer to switch off the indicator LED
	ledTicker.start();
}

/**
 * @brief Get network join status
 * 
 * @return true if joined to network
 * @return false if not joined
 */
bool lmhJoined(void)
{
	if (lmh_join_status_get() == LMH_SET)
	{
		return true;
	}
	else
	{
		return false;
	}
}

/**
 * @brief Get network address after OTAA 
 * 
 * @return uint32_t Network address
 */
uint32_t lmhAddress(void)
{
	return lmh_getDevAddr();
}
