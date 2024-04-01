/****************************************************************************************************************************
   checkWaitingDRD.ino
   For ESP8266 / ESP32 boards

   ESP_DoubleResetDetector is a library for the ESP8266/Arduino platform
   to enable trigger configure mode by resetting ESP32 / ESP8266 twice.

   Based on and modified from DataCute https://github.com/datacute/DoubleResetDetector

   Built by Khoi Hoang https://github.com/khoih-prog/ESP_DoubleResetDetector
   Licensed under MIT license
 *****************************************************************************************************************************/

// These defines must be put before #include <ESP_DoubleResetDetector.h>
// to select where to store DoubleResetDetector's variable.
// For ESP32, You must select one to be true (EEPROM or SPIFFS)
// For ESP8266, You must select one to be true (RTC, EEPROM, LITTLEFS or SPIFFS)
// Otherwise, library will use default EEPROM storage

// This example demonstrates how to use new function waitingForDRD() to signal the stage of DRD
// waitingForDRD() returns true if in DRD_TIMEOUT, false when out of DRD_TIMEOUT
// In this example, LED_BUILTIN will blink in DRD_TIMEOUT period, ON when DR has been detected, OFF otherwise

#ifdef ESP8266
  #define ESP8266_DRD_USE_RTC     false   //true
#endif

#define ESP_DRD_USE_LITTLEFS    true
#define ESP_DRD_USE_SPIFFS      false
#define ESP_DRD_USE_EEPROM      false

// Uncomment to have debug
//#define DOUBLERESETDETECTOR_DEBUG       true

#include <ESP_DoubleResetDetector.h>      //https://github.com/khoih-prog/ESP_DoubleResetDetector

// Number of seconds after reset during which a 
// subsequent reset will be considered a double reset.
#define DRD_TIMEOUT 10

// RTC Memory Address for the DoubleResetDetector to use
#define DRD_ADDRESS 0

DoubleResetDetector* drd;

#ifdef ESP32

  // For ESP32
  #ifndef LED_BUILTIN
    #define LED_BUILTIN       2         // Pin D2 mapped to pin GPIO2/ADC12 of ESP32, control on-board LED
  #endif

  #define LED_OFF     LOW
  #define LED_ON      HIGH

#else

  // For ESP8266
  #define LED_ON      LOW
  #define LED_OFF     HIGH

#endif

bool DRD_Detected = false;

void check_status()
{
  static ulong checkstatus_timeout  = 0;
  static bool LEDState = LED_OFF;

  static ulong current_millis;

#define DRD_CHECK_INTERVAL    500L

  current_millis = millis();

  // If DRD_Detected, don't need to blink, just keep LED_BUILTIN ON
  if ( !DRD_Detected && ((current_millis > checkstatus_timeout) || (checkstatus_timeout == 0)) )
  {
    // If in DRD checking loop, blinking the LED_BUILTIN
    if ( drd->waitingForDRD() )
    {
      digitalWrite(LED_BUILTIN, LEDState);

      LEDState = !LEDState;    
    }
    else
    {
      digitalWrite(LED_BUILTIN, LED_OFF);
    }
    
    checkstatus_timeout = current_millis + DRD_CHECK_INTERVAL;
  }
}

void setup()
{
  pinMode(LED_BUILTIN, OUTPUT);
  
  Serial.begin(115200);
  while (!Serial);

  delay(200);

  Serial.print("\nStarting checkWaitingDRD on"); Serial.println(ARDUINO_BOARD);
  Serial.println(ESP_DOUBLE_RESET_DETECTOR_VERSION);
   
  drd = new DoubleResetDetector(DRD_TIMEOUT, DRD_ADDRESS);

  if (drd->detectDoubleReset()) 
  {
    Serial.println("Double Reset Detected");
    digitalWrite(LED_BUILTIN, LED_ON);
    DRD_Detected = true;
  } 
  else 
  {
    Serial.println("No Double Reset Detected");
    digitalWrite(LED_BUILTIN, LED_OFF);
  }
}

void loop()
{
  // Call the double reset detector loop method every so often,
  // so that it can recognise when the timeout expires.
  // You can also call drd.stop() when you wish to no longer
  // consider the next reset as a double reset.
  drd->loop();

  check_status();
}
