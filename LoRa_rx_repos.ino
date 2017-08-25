// Marissa Kwon
// Date: 08/25/2017
// Description: Code for "Receiver" LoRa radio breakout; initilizes
// LoRa radio receiver and responds to LoRa transmitter on the same
// frequency.

//Arduino9x_RX
// -*- mode: C++ -*-
// Example sketch showing how to create a simple messaging client (receiver)
// with the RH_RF95 class. RH_RF95 class does not provide for addressing or
// reliability, so you should only use RH_RF95 if you do not need the higher
// level messaging abilities.
// It is designed to work with the other example Arduino9x_TX

// This code originated from Adafruit's website
// https://learn.adafruit.com/adafruit-rfm69hcw-and-rfm96-rfm95-rfm98-lora-packet-padio-breakouts/rfm9x-test
// and has been adjusted for use in the Internet of Agriculture project.
// URSA Engage Student Research at OPEnS Lab at Oregon State University.


#include <SPI.h>
#include <RH_RF95.h>
#define RFM95_CS 10
#define RFM95_RST 9
#define RFM95_INT 3

// Change to 434.0/915.0 or other frequency, must match RX's freq!
#define RF95_FREQ 430.0

// Singleton instance of the radio driver
RH_RF95 rf95(RFM95_CS, RFM95_INT);

// Blinky on receipt
#define LED 13

//Global setup
String IDRead, loadCellRead;

char * data_array[8];

void setup() {
  while (!Serial);
  Serial.begin(9600);
  delay(100); Serial.println("test");
  //pinMode(LED, OUTPUT);
  pinMode(RFM95_RST, OUTPUT);
  digitalWrite(RFM95_RST, HIGH);
  Serial.println("Arduino LoRa RX Test!");
  // manual reset
  digitalWrite(RFM95_RST, LOW);
  delay(10);
  digitalWrite(RFM95_RST, HIGH);
  delay(10);
  while (!rf95.init()) {
    Serial.println("LoRa radio init failed");
    while (1);
  }
  Serial.println("LoRa radio init OK!");
  // Defaults after init are 434.0MHz, modulation GFSK_Rb250Fd250, +13dbM
  if (!rf95.setFrequency(RF95_FREQ)) {
    Serial.println("setFrequency failed");
    while (1);
  }
  Serial.print("Set Freq to: "); Serial.println(RF95_FREQ);
  // Defaults after init are 434.0MHz, 13dBm, Bw = 125 kHz, Cr = 4/5, Sf = 128chips / symbol, CRC on
  // The default transmitter power is 13dBm, using PA_BOOST.
  // If you are using RFM95/96/97/98 modules which uses the PA_BOOST transmitter pin, then
  // you can set transmitter powers from 5 to 23 dBm: rf95.setTxPower(23, false);
  Serial.println("test");

}
void loop()
{
  if (rf95.available())
  {
    // Should be a message for us now
    uint8_t buf[RH_RF95_MAX_MESSAGE_LEN];
    //data array that sensor value will be copied into
    uint8_t len = sizeof(buf);
    if (rf95.recv(buf, &len))
    {
      digitalWrite(LED, HIGH);
      //prints message in hexadecimal value
      //copies data received into buf array
      RH_RF95::printBuffer("Received: ", buf, len);
      //prints
      Serial.println("Got: ");
      //casts buf to an array of chars
      Serial.println((char*)buf);
      Serial.print("RSSI: ");
      Serial.println(rf95.lastRssi(), DEC);

      // data transmitted in order:
      // ID, timestamp, temperature, humidity, loadcell, (IR spectrum, full spectrum, battery voltage not added yet)
      // break up data into separate values
      // sourced from: https://stackoverflow.com/questions/15472299/split-string-into-tokens-and-save-them-in-an-array
      char * data_point;
      data_point = strtok((char*)buf, ","); //dont forget to typecast buf to char array
      short i = 0; //short i used as a placeholder for array iterations - no need for int
      while (data_point != NULL) {
        data_array[i++] = data_point;
        data_point = strtok(NULL, ",");
      }

      // Send a reply
      if (len > 0) { //sends nothing if there is length 0
        rf95.send(data_array[0], sizeof(data_array[0]) + 1);
      }
      rf95.waitPacketSent();
      Serial.print("Sent a reply \n");
      Serial.println(data_array[0]);
      digitalWrite(LED, LOW);
    } else {
      Serial.println("Receive failed");
    }
  }
}

