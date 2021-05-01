
| | |
| :-: | :-: |
| <center><img src="./assets/rakstar.jpg" alt="RAKstar" width=25%></center> | <center><img src="./assets/WisBlock.svg" alt="WisBlock" width=75%></center> |    

RAK4631 LoRaWan® Helium Mapper [RAKwireless](./assets/RAK-Whirls.png)
===

Based extensively on: beegee-tokyo/RAK4631-LPWAN-Tracker
    
This module provides data that can be fed into the [Helium Cargo](https://docs.helium.com/use-the-network/console/integrations/cargo/) app and / or the [Helium Coverage Mapper](https://docs.helium.com/use-the-network/coverage-mapping/). 

It gets location information from an attached uBlox GPS module and sends that data via LoRaWAN to the [Helium Console](https://console.helium.com). In addition an acceleration sensor is used to detect if the tracker is moving. If movement of the tracker is detected, location information is sent immediately. If the tracker is stationary, the location data is sent every 1 minute 

Solution
---
This solution shows
- how to initiate a LoRaWan connection with OTAA network join on the WisCore RAK4631
- how to initiate BLE with  on the WisCore RAK4631
  - BLE UART characteristic for debug output. Requires a BLE-UART app like the [Serial Bluetooth Terminal](https://play.google.com/store/apps/details?id=de.kai_morich.serial_bluetooth_terminal) for Android
  - BLE OTA DFU for firmware updates
- how to use the WisSensor RAK1910 GPS module to aquire location information
- how to use the WisSensor RAK1904 acceleration sensor to detect movements
- how to read the battery level from the WisCore RAK5005-O board
- how to convert the sensor values into a byte array to create the smallest possible LoRa package size

Hardware required
---
To build this solution the following hardware is required
- [WisBlock Starter Kit -WisBase RAK5005-O & WisCore RAK4631](https://shop.parleylabs.com/collections/wisblock/products/wisblock-starter-kit) - $29.99
- [WisSensor RAK1904](https://shop.parleylabs.com/collections/wisblock/products/rak1904-lis3dh-3-axis-acceleration-sensor) - $3.95
- [WisSensor RAK1910](https://shop.parleylabs.com/collections/wisblock/products/rak1910-max-7q-gnss-location-sensor) - $19.25
- [WisIO RAK1921 - optional](https://shop.parleylabs.com/collections/wisblock/products/rak1921-oled-display-panel) - $6.00
- [18650 LiPo battery](https://amzn.to/3ewP896) - $21.99 (4 batteries + flashlight) OR [$9.95](https://www.digikey.com/en/products/detail/adafruit-industries-llc/1781/5054543?utm_adgroup=Batteries%20Rechargeable%20%28Secondary%29&utm_source=google&utm_medium=cpc&utm_campaign=Shopping_Product_Battery%20Products&utm_term=&utm_content=Batteries%20Rechargeable%20%28Secondary%29&gclid=Cj0KCQjw1a6EBhC0ARIsAOiTkrEgWo3i0Go5FBQo4EYKaHvbP68WP44AzHrlkkZNG6KdSXsDwEN2ljMaAjTlEALw_wcB) (battery by itself)
- [Waterproof Case](https://amzn.to/3dRlI6q) - $14.99
- [Antenna](https://amzn.to/3dS3vWw) - $10.99 (4)
- [Battery Holder](https://amzn.to/3tTTcqo) - $6.99 (5)

Total Cost: $96.11
---

| | | |
| :-: | :-: | :-: |
| <center><img src="./assets/PXL_20210427_015839254.jpg" width=75%></center> | <center><img src="./assets/PXL_20210427_015900466.jpg" width=75%></center> | <center><img src="./assets/PXL_20210427_024117331.jpg" width=75%></center> |

Software required
---
To build this solution the following is required
- [PlatformIO](https://platformio.org/)
- [RAK4630 BSP](https://github.com/RAKWireless/RAK-nRF52-Arduino)
- [SX126x-Arduino](https://github.com/beegee-tokyo/SX126x-Arduino)
- [nRF52_OLED](https://github.com/beegee-tokyo/nRF52_OLED)
- [Sparkfun LIS3DH](https://github.com/sparkfun/SparkFun_LIS3DH_Arduino_Library)

Setting Up your WisBlock core  on PlatformIO
---
PlatformIO is a great platform for developing embedded systems.  [Here are the instructions for getting Rak WisBlocks set up on PlatformIO.](https://github.com/RAKWireless/WisBlock/tree/master/PlatformIO/RAK4630)  Be aware that this project is based on the 2021 firmware (probably newer than what came on your WisBlock Core.  [To upgrade the firmware, follow the instructions here.](https://github.com/RAKWireless/WisBlock/tree/master/bootloader/RAK4630/new)

LoRaWan server required
---
In order to get this code working you need access to the [Helium Network](https://helium.com/).  Getting access to Helium is quite simple and there is a good tutorial at [Getting WisBlock by RAKwireless Up and Running on The People’s Network](https://blog.helium.com/my-take-on-the-wisblock-e8b934ceb64).

The region you live in defines the frequency your LoRaWan® gateways will use. So you need to setup your device to work on the correct frequency.    
##### With the new LoRaWAN® library SX126x-Arduino **V2.0.0** the region is set as a parameter in **`lmh_init()`**.

In the call **`lmh_init()`** the last parameter defines the LoRaWAN® region. 

Short explanation about the new **`lmh_init()`** call:    
```cpp
/**@brief Lora Initialisation
 *
 * @param callbacks   Pointer to structure containing the callback functions
 * @param lora_param  Pointer to structure containing the parameters
 * @param otaa        Choose OTAA (true) or ABP (false) activation
 * @param nodeClass   Choose node class CLASS_A, CLASS_B or CLASS_C, default to CLASS_A
 * @param region      Choose LoRaWAN region to set correct region parameters, defaults to EU868
 *
 * @retval error status
 */
	lmh_error_status lmh_init(lmh_callback_t *callbacks, lmh_param_t lora_param, bool otaa, 
	                          eDeviceClass nodeClass = CLASS_A, 
	                          LoRaMacRegion_t region = LORAMAC_REGION_EU868);
```
The first three parameters are the same as before. Two new parameters have been added.

### eDeviceClass nodeClass
Even this parameter was defined in V1.x, the **`lmh_init()`** ignored it and initialized the node **ALWAYS** as a node Class A.    
Now you can explicit set your node to **CLASS_A** or **CLASS_C**. Please take note that **CLASS_B** is still not supported by the library.

### LoRaMacRegion_t region
This parameter selects the LoRaWAN region for your application. Allowed values for the region are:    
- _**LORAMAC_REGION_AS923**_    
- _**LORAMAC_REGION_AU915**_    
- _**LORAMAC_REGION_CN470**_    
- _**LORAMAC_REGION_CN779**_    
- _**LORAMAC_REGION_EU433**_    
- _**LORAMAC_REGION_EU868**_    
- _**LORAMAC_REGION_IN865**_    
- _**LORAMAC_REGION_KR920**_    
- _**LORAMAC_REGION_US915**_ --Default--  
- _**LORAMAC_REGION_US915_HYBRID**_

Some explanation for the code
---

Due to the complexity of the code, it is split into functional parts.
- main.h 
   - All the includes, global definitions and forward declarations for the app
- main.cpp
   - Setup function where we initialize all peripherals
   - Main loop
   - Timers callback functions for periodic and delayed sending of packages
- acc.cpp
   - Accelerometer initialization, interrupt callback function and interrupt clearing functions
- bat.cpp
   - Battery level functions
- ble.cpp
   - BLE initialization and BLE UART callback functions
- display.cpp
   - Display initialization and handling functions
- gps.cpp
   - GPS initialization and and data poll functions
- loraHandler.cpp
   - LoRaWan initialization function, LoRaWan handling task and LoRaWan event callbacks
- include/config.cpp.example
   - This is a sample config file.  Copy it to include/config.cpp and configure to match your Helium settings.

How to achieve power saving with nRF52 cores on Arduino IDE
----
Within the nRF52 Arduino framework is no specific function to send the MCU into sleep mode. The MCU will go into sleep mode when 
- all running tasks are calling delay()
- the running tasks are waiting for a semaphore

In this example you use the semaphore method.    

Two tasks are running independently, the loop() task and the loraTask() task which is started by the SX126x-Arduino library and runs in the background.
A semaphores are used to control the activity of the loop() tasks.    
```cpp
/** Semaphore to wake up the main loop */
SemaphoreHandle_t loopEnable;
```

After setup and starting the LoRaWan join process, the semaphore _**loopEnable**_ is taken.  
This means the loop task will go into waiting mode while the loraTask is handling LoRaWan events in the background. The LoRaWan task is in sleep mode until a LoRa event occurs.    
At this point no more task is active and the device will go into sleep mode.  
  
**Events to wake up the MCU**
1. Accelerometer detect movement
If the ACC sensor detects movement, the MCU receives an interrupt. In the interrupt callback the semaphore _**loopEnable**_ is given again, which allows the loop task to run.
2. Timer event
If the periodic sending timer is triggered, the MCU will wake up and the callback function of the timer will give the semaphore _**loopEnable**_, which allows the loop task to run.

Once the loop task is enabled, it will poll the position from the GPS module and requests sending a data package by calling sendLoRaFrame(). Then it takes the semaphore _**loopEnable**_, which puts herself back into waiting mode until the next event.

LoRa® is a registered trademark or service mark of Semtech Corporation or its affiliates. LoRaWAN® is a licensed mark. 
