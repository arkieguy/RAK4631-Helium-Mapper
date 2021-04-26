/**
 * @file main.h
 * @author Bernd Giesecke (bernd@giesecke.tk)
 * @brief Global variable and function declarations  
 * @version 0.1
 * @date 2020-07-25
 * 
 * @copyright Copyright (c) 2020
 * 
 */
#include <Arduino.h>
#include <SPI.h>
#include <Wire.h>

// Generic
extern char dbgBuffer[];
extern uint8_t wakeReason;
extern BaseType_t xHigherPriorityTaskWoken;

// Display functions
#include "nRF_SSD1306Wire.h"
/** Width of the display in pixel */
#define OLED_WIDTH 128
/** Height of the display in pixel */
#define OLED_HEIGHT 64
/** Height of the status bar in pixel */
#define STATUS_BAR_HEIGHT 11
/** Height of a single line */
#define LINE_HEIGHT 10
/** Number of message lines */
#define NUM_OF_LINES (OLED_HEIGHT - STATUS_BAR_HEIGHT) / LINE_HEIGHT
void initDisplay(void);
void dispAddLine(char *line);
void dispShow(void);
void dispWriteHeader(void);

// ACC functions
#include <SparkFunLIS3DH.h>
#define INT1_PIN 21
bool initACC(void);
void clearAccInt(void);
extern SemaphoreHandle_t loopEnable;

// GPS functions
#include "TinyGPS++.h"
#include <SoftwareSerial.h>
void initGPS(void);
bool pollGPS(void);
// extern byte coords[];

// Battery functions
/** Definition of the Analog input that is connected to the battery voltage divider */
#define PIN_VBAT A0
/** Definition of milliVolt per LSB => 3.0V ADC range and 12-bit ADC resolution = 3000mV/4096 */
#define VBAT_MV_PER_LSB (0.73242188F)
/** Voltage divider value => 1.5M + 1M voltage divider on VBAT = (1.5M / (1M + 1.5M)) */
#define VBAT_DIVIDER (0.4F)
/** Compensation factor for the VBAT divider */
#define VBAT_DIVIDER_COMP (1.73)
/** Fixed calculation of milliVolt from compensation value */
#define REAL_VBAT_MV_PER_LSB (VBAT_DIVIDER_COMP * VBAT_MV_PER_LSB)
void initReadVBAT(void);
float readVBAT(void);
uint8_t readBatt(void);
uint8_t readBattVolts(void) ;
uint8_t lorawanBattLevel(void);
extern uint8_t battLevel;

// BLE
#include <bluefruit.h>
void initBLE();
void startAdv(void);
void connect_callback(uint16_t conn_handle);
void disconnect_callback(uint16_t conn_handle, uint8_t reason);
extern bool bleUARTisConnected;
extern BLEUart bleuart;

// LoRaWan functions
uint8_t initLoRaHandler(void);
void sendLoRaFrame(void);
bool lmhJoined(void);
uint32_t lmhAddress(void);
struct tracker_data_s
{
	uint8_t lat_1; // 1
	uint8_t lat_2; // 2
	uint8_t lat_3; // 3
	uint8_t lat_4; // 4
	uint8_t lng_1; // 5
	uint8_t lng_2; // 6
	uint8_t lng_3; // 7
	uint8_t lng_4; // 8
	uint8_t alt_1; // 9
	uint8_t alt_2; // 10
	uint8_t hdop; // 11
	uint8_t batt; // 12
	uint8_t sp_1; // 13
	uint8_t sp_2; // 14
};
extern tracker_data_s trackerData;
#define TRACKER_DATA_LEN 14 // sizeof(trackerData)
