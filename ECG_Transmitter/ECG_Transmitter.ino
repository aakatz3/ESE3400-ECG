// ECG Transmitter Sketch, adapted from Adafruit Bluefruit examples
#include <Arduino.h>
#include <Adafruit_TinyUSB.h>  // for Serial
#include "Simpletimer.h"
#include <bluefruit.h>


// LED Threshold value
// Empirical value: Peaks occur with 2mV input signal at more than 530 counts
#define THRESH 530
// LED Pin. LED blinks when peak is detected
#define LED 13
// ADC Pin
// A0 for custom circuit, A4 for AD8232 breakout board
#define adcin A0
// #define adcin A4;

// Delay between samples
#define del 10

// BLE Service
BLEDfu  bledfu;  // OTA DFU service
BLEDis  bledis;  // device information
BLEUart bleuart; // uart over ble
BLEBas  blebas;  // battery

// Variable to store ADC value. Create globally to make sure we don't need to allocate and deallocate
int adcvalue = 0;

// Time (in ms) to turn LED back off after turning on
long ledOnTime = 0;

// Define software timers
Simpletimer timer1{};
Simpletimer timer2{};


// Primary callback, for timer 1. Handles reading ADC and data transmission
void callback1() {
  // Read ADC
  adcvalue = analogRead(adcin);
  
  // if ADC counts is higher than programmed threshold, update time for LED to turn off
  if(adcvalue > THRESH)
    ledOnTime = millis()+100;
  
  // Create a buffer of 2 8 bit unsigned ints for bluefruit library
  uint8_t buf [2];
  // Take the lower 8 bits of the ADC counts and put it in first buffer position
  buf[0] = 0xFF & adcvalue;
  // Take the second lowest 8 bits of the ADC counts and put it in first buffer position
  buf[1] = (0xFF00 & adcvalue) >> 8;
  // Write the buffer to BLE
  bleuart.write( (uint8_t*) buf, 2);
}

// LED callback, runs every 40ms
void callback2() {
  // If LED should be on, turn it on. If it should be off, turn it off.
  digitalWrite(LED, millis() <= ledOnTime);
}


void setup() {
  Serial.begin(115200);
  pinMode(13,OUTPUT);

  timer1.register_callback(callback1);
  timer2.register_callback(callback2);
 
  // Setup the BLE LED to be enabled on CONNECT
  // Note: This is actually the default behavior, but provided
  // here in case you want to control this LED manually via PIN 19
  Bluefruit.autoConnLed(true);

  // Config the peripheral connection with maximum bandwidth 
  // more SRAM required by SoftDevice
  // Note: All config***() function must be called before begin()
  Bluefruit.configPrphBandwidth(BANDWIDTH_MAX);

  Bluefruit.begin();
  Bluefruit.setTxPower(4);    // Check bluefruit.h for supported values
  Bluefruit.setName(getMcuUniqueID()); // useful testing with multiple central connections
  Bluefruit.Periph.setConnectCallback(connect_callback);
  Bluefruit.Periph.setDisconnectCallback(disconnect_callback);

  // Add BLE DFU, even though we probably won't use it
  bledfu.begin();

  // Configure and Start Device Information Service
  bledis.setManufacturer("Adafruit Industries");
  bledis.setModel("Bluefruit Feather52");
  bledis.begin();

  // Configure and Start BLE Uart Service
  bleuart.begin();

  // Start BLE Battery Service
  blebas.begin();
  blebas.write(100);

  // Set up and start advertising
  startAdv();

  Serial.println("Please connect reciever now if it is not already connected");
}
void startAdv(void)
{
  // Advertising packet
  Bluefruit.Advertising.addFlags(BLE_GAP_ADV_FLAGS_LE_ONLY_GENERAL_DISC_MODE);
  Bluefruit.Advertising.addTxPower();

  // Include bleuart 128-bit uuid
  Bluefruit.Advertising.addService(bleuart);

  // Secondary Scan Response packet for name of device
  Bluefruit.ScanResponse.addName();
  

  Bluefruit.Advertising.restartOnDisconnect(true); //  Enable auto advertising if disconnected
  Bluefruit.Advertising.setInterval(32, 244);     // in unit of 0.625 ms, fast and slow mode intervals (20ms, 152.5ms)
  Bluefruit.Advertising.setFastTimeout(30);       // number of seconds in fast mode
  Bluefruit.Advertising.start(0);                 // 0 = Don't stop advertising until connected  
}
void loop() {
    // Update/check software timers
     timer1.run(del);
     timer2.run(40);
}

// callback invoked when central connects
void connect_callback(uint16_t conn_handle)
{
  // Get the reference to current connection
  BLEConnection* connection = Bluefruit.Connection(conn_handle);

  char central_name[32] = { 0 };
  connection->getPeerName(central_name, sizeof(central_name));

  Serial.print("Connected to ");
  Serial.println(central_name);
}

/**
 * Callback invoked when a connection is dropped
 * @param conn_handle connection where this event happens
 * @param reason is a BLE_HCI_STATUS_CODE which can be found in ble_hci.h
 * From Adafruit example
 */
void disconnect_callback(uint16_t conn_handle, uint8_t reason)
{
  (void) conn_handle;
  (void) reason;

  Serial.println();
  Serial.print("Disconnected, reason = 0x"); Serial.println(reason, HEX);
}