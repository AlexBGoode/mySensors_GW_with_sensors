  /**
 * The MySensors Arduino library handles the wireless radio link and protocol
 * between your home built sensors/actuators and HA controller of choice.
 * The sensors forms a self healing radio network with optional repeaters. Each
 * repeater and gateway builds a routing tables in EEPROM which keeps track of the
 * network topology allowing messages to be routed to nodes.
 *
 * Created by Henrik Ekblad <henrik.ekblad@mysensors.org>
 * Copyright (C) 2013-2015 Sensnology AB
 * Full contributor list: https://github.com/mysensors/Arduino/graphs/contributors
 *
 * Documentation: http://www.mysensors.org
 * Support Forum: http://forum.mysensors.org
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * version 2 as published by the Free Software Foundation.
 *
 *******************************
 *
 * DESCRIPTION
 * The ArduinoGateway prints data received from sensors on the serial link. 
 * The gateway accepts input on seral which will be sent out on radio network.
 *
 * The GW code is designed for Arduino Nano 328p / 16MHz
 *
 * Wire connections (OPTIONAL):
 * - Inclusion button should be connected between digital pin 3 and GND  
 * - RX/TX/ERR leds need to be connected between +5V (anode) and digital pin 6/5/4 with resistor 270-330R in a series
 *
 * LEDs (OPTIONAL):
 * - To use the feature, uncomment MY_LEDS_BLINKING_FEATURE in MyConfig.h
 * - RX (green) - blink fast on radio message recieved. In inclusion mode will blink fast only on presentation recieved
 * - TX (yellow) - blink fast on radio message transmitted. In inclusion mode will blink slowly
 * - ERR (red) - fast blink on error during transmission error or recieve crc error 
 * 
 */

// Enable debug prints to serial monitor
#define MY_DEBUG 


// Enable and select radio type attached
#define MY_RADIO_NRF24
//#define MY_RADIO_RFM69

// Set LOW transmit power level as default, if you have an amplified NRF-module and
// power your radio separately with a good regulator you can turn up PA level. 
#define MY_RF24_PA_LEVEL RF24_PA_LOW

// Enable serial gateway
#define MY_GATEWAY_SERIAL

// Define a lower baud rate for Arduino's running on 8 MHz (Arduino Pro Mini 3.3V & SenseBender)
#if F_CPU == 8000000L
#define MY_BAUD_RATE 38400
#endif

// Flash leds on rx/tx/err
#define MY_LEDS_BLINKING_FEATURE
// Set blinking period
#define MY_DEFAULT_LED_BLINK_PERIOD 300

// Inverses the behavior of leds
//#define MY_WITH_LEDS_BLINKING_INVERSE

// Enable inclusion mode
#define MY_INCLUSION_MODE_FEATURE
// Enable Inclusion mode button on gateway
#define MY_INCLUSION_BUTTON_FEATURE

// Inverses behavior of inclusion button (if using external pullup)
//#define MY_INCLUSION_BUTTON_EXTERNAL_PULLUP

// Set inclusion mode duration (in seconds)
#define MY_INCLUSION_MODE_DURATION 60 
// Digital pin used for inclusion mode button
#define MY_INCLUSION_MODE_BUTTON_PIN  3 

// Uncomment to override default HW configurations
//#define MY_DEFAULT_ERR_LED_PIN 4  // Error led pin
//#define MY_DEFAULT_RX_LED_PIN  6  // Receive led pin
//#define MY_DEFAULT_TX_LED_PIN  5  // the PCB, on board LED

#include <SPI.h>
#include <MySensors.h>  

#define CHILD_ID_TEMP 0
#define TEMP_SENSOR_ANALOG_PIN 7
unsigned long SLEEP_TIME = 30*1000; // Sleep time between reads (in milliseconds)
//const float c = 10/(1100/1024); // reading portion for 1 degree Celcius
const float c = 9.30909;
//int lastTEMP = 0;
uint16_t reading;
float tempC;

// Initialize temperature message
MyMessage msgTemp(CHILD_ID_TEMP, V_TEMP);

void setup() { 
  // Setup locally attached sensors
  // sets ref voltage for conversion to 1.1 vs the default 5 V
  // This gives much better accuracy since the 10 bit ADC is spread over 1.1V vs the default 5V
  analogReference(INTERNAL);  // http://playground.arduino.cc/Main/LM35HigherResolution
}

void presentation() {
 // Present locally attached sensors 
  sendSketchInfo("GW with Temp Sensor", "1.2");
  present(CHILD_ID_TEMP, S_TEMP);
  Serial.print("The constant: ");
  Serial.println(c);
  
  Serial.print("Reading 1st time to warm up: ");
  Serial.println(analogRead(TEMP_SENSOR_ANALOG_PIN));
}

void loop() { 
  // Send locally attached sensor data here 
  // http://playground.arduino.cc/Main/LM35HigherResolution
  // using aRef 1.1V instead of aRef 5V
  // divide 1.1V over 1024 of ADC resolution = 0.00107421875 V = 1.07421875 mV per step
  // 10 mV is equal to 1 degree Celcius, 10 / 1.07421875 = 9.30(90) = ~9.31
  // for every change of 9.31 in the analog reading, there is one degree of temperature change
  reading = analogRead(TEMP_SENSOR_ANALOG_PIN);// Get TEMP value
  tempC = reading / c;

  Serial.print("Raw Signal Value (0-1023): ");
  Serial.print(reading);
  
  Serial.print(" - Temp: ");
  Serial.println(tempC);

//  if (ceil(tempC) != lastTEMP) {
//      send(msgTemp.set((int)ceil(tempC)));
//      lastTEMP = ceil(tempC);
//  }  

  // Return float XX.X C
  send(msgTemp.set(tempC,1));

  sleep(SLEEP_TIME);
}





