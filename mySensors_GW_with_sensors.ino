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
#include <DHT.h>
#include <Adafruit_Sensor.h>
//https://github.com/RobTillaart/Arduino/tree/master/libraries/RunningMedian
#include <RunningMedian.h>

// virtual group 1#
#define CHILD_ID_LM35_TEMP 0        // real temp
#define CHILD_ID_LM35_TEMP_AVG 1    // calculated average for last 3 values
#define CHILD_ID_LM35_TEMP_INT 2    // calculated int of the average
// virtual group 2#
#define CHILD_ID_DHT11_TEMP 3       // real temp
#define CHILD_ID_DHT11_HUM 4        // real humidity
#define CHILD_ID_DHT11_TEMP_AVG 5   // calculated average for last 3 values

#define LM35_SENSOR_ANALOG_PIN 7
#define LM35_C 9.30909

#define DHT11_SENSOR_DIGITAL_PIN 5
#define DHTTYPE DHT11
#define DHT_READ_INTERVAL 2500      // wait at least 2.5 seconds between readings

#define SLEEP_TIME 30*1000          // sleep time between reads (in milliseconds)


RunningMedian rmDHT11 = RunningMedian( 5 );
RunningMedian rmLM35 = RunningMedian( 3 );

float t;
float h;
float averageDHT11;
uint16_t readingLM35, lastReadingLM35 = 0;
float tempLM35;
float averageLM35;
int intLM35;

DHT dht( DHT11_SENSOR_DIGITAL_PIN, DHT11 );

// Initialize temperature message
// virtual group 1#
MyMessage msgLM35Temp( CHILD_ID_LM35_TEMP, V_TEMP );
MyMessage msgLM35TempInt( CHILD_ID_LM35_TEMP_INT, V_HUM );
//MyMessage msgLM35TempAgv( CHILD_ID_LM35_TEMP_AVG, V_PRESSURE );
// virtual group 2#
MyMessage msgDHT11Temp( CHILD_ID_DHT11_TEMP, V_TEMP );
MyMessage msgDHT11Humidity( CHILD_ID_DHT11_HUM, V_HUM );
//MyMessage msgDHT11TempAgv( CHILD_ID_DHT11_TEMP_AVG, V_PRESSURE );

void setup() { 
  // Setup locally attached sensors

  // Prepare LM35
  // Sets ref voltage for conversion to 1.1 vs the default 5 V
  // This gives much better accuracy since the 10 bit ADC is spread over 1.1V vs the default 5V
  analogReference( INTERNAL );  // http://playground.arduino.cc/Main/LM35HigherResolution

  // Prepare DHT11
  dht.begin();  // does nothing else but setting pinMode(_pin, INPUT_PULLUP);
}

void presentation() {
 // Present locally attached sensors 
  sendSketchInfo( "GW with Sensors", "1.3" );
  
  // should be auto grouped according to
  // https://forum.mysensors.org/topic/5132/ds18b20-ans-sht31-d-show-up-as-combined-sensors-on-domoticz
  // as Group 1#
  present( CHILD_ID_LM35_TEMP, S_TEMP, "LM35 float temp" );
  present( CHILD_ID_LM35_TEMP_INT, S_HUM, "LM35 integer temp" );
//  present( CHILD_ID_LM35_TEMP_AVG, S_BARO, "LM35 average temp" );
  // as Group 2#
  present( CHILD_ID_DHT11_TEMP, S_TEMP, "DHT11 float temp" );
  present( CHILD_ID_DHT11_HUM, S_HUM, "DHT11 humitity" );
//  present( CHILD_ID_DHT11_TEMP_AVG, S_BARO, "DHT11 integer temp" );

  Serial.print( "The constant for LM35: ");
  Serial.println( LM35_C );
  
  Serial.print( "Reading DHT11 1st time to warm it up: ");
  Serial.println( analogRead( LM35_SENSOR_ANALOG_PIN ));
}

void loop() { 
  // Send locally attached sensor data here 

  sleep( DHT_READ_INTERVAL ); // wait between readings
  // Reading temperature or humidity takes about 250 milliseconds!
  // Sensor readings may also be up to 2 seconds 'old' (its a very slow sensor)

  h = dht.readHumidity();
  t = dht.readTemperature();       // Read temperature as Celsius (the default)

  // Check if any reads failed and exit early (to try again).
  if (isnan(h) || isnan(t)) {
    Serial.println( "Failed to read from DHT sensor!" );
    return;
  }

  rmDHT11.add( t );
  averageDHT11 = rmDHT11.getAverage();

  Serial.print(" - DHT11 Temp ");
  Serial.print( t );
  Serial.print(" | Humidity ");
  Serial.print( h );
  Serial.print(" | Running average ");
  Serial.println( averageDHT11 );

  send( msgDHT11Temp.set( t, 1));                 // report float value
//  send( msgDHT11TempAgv.set( averageDHT11, 1 ));  // report float value
  send( msgDHT11Humidity.set( h, 0 ));            // report int value


  // http://playground.arduino.cc/Main/LM35HigherResolution
  // using aRef 1.1V instead of aRef 5V
  readingLM35 = analogRead( LM35_SENSOR_ANALOG_PIN ); // Get TEMP value

  tempLM35 = readingLM35 / LM35_C;
  rmLM35.add( tempLM35 );

  averageLM35 = rmLM35.getAverage();
  intLM35 = round( averageLM35 );

  Serial.print( " - LM35 Raw Signal Value (0-1023): ");
  Serial.print( readingLM35 );
  
  Serial.print(" - Temp ");
  Serial.print( tempLM35 );
  Serial.print(" | Average temp ");
  Serial.print( averageLM35 );
  Serial.print(" | Int temp ");
  Serial.println( intLM35 );

  send( msgLM35Temp.set( tempLM35, 1 ));          // report float value
//  send( msgLM35TempAgv.set( averageLM35, 1 ));    // report float value
  send( msgLM35TempInt.set( intLM35 ));           // report int value



  sleep( SLEEP_TIME );
}





