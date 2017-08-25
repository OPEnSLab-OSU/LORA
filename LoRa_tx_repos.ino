/* Author: Marissa Kwon
  Date: 2/28/2017
  Last Updated: 08/25/2017
  Description: Code for "Transmitter" LoRa radio breakout; initializes
  LoRa radio transmitter and sends data to LoRa receiver on the same
  frequency.
  ** This is an older version of our LoRa code that has been updated to read from with our temp/humidity/loadcell/light sensors
     for use on Arduino Uno.  Adafruit Protrinket

  This code originated from Adafruit's website at
  https://learn.adafruit.com/adafruit-rfm69hcw-and-rfm96-rfm95-rfm98-lora-packet-padio-breakouts/rfm9x-test
  and has been adjusted for use in the Internet of Agriculture project.
  URSA Engage Student Research at OPEnS Lab at Oregon State
  University.


*/
// LoRa 9x_TX
// -*- mode: C++ -*-
// Example sketch showing how to create a simple messaging client(transmitter)
// with the RH_RF95 class. RH_RF95 class does not provide for addressing or
// reliability, so you should only use RH_RF95 if you do not need the higher
// level messaging abilities.
// It is designed to work with the other example LoRa9x_RX

#include <SPI.h>
//------------------------------------------------------------------------
// Radio Head Library --------------------
//------------------------------------------------------------------------
#include <RH_RF95.h> // Important Example code found at https://learn.adafruit.com/adafruit-rfm69hcw-and-rfm96-rfm95-rfm98-lora-packet-padio-breakouts/rfm9x-test
//------------------------------------------------------------------------
// Load Cell Settings Library --------------------
// ------------------------------------------------------------------------
#include "HX711.h" //https://learn.sparkfun.com/tutorials/load-cell-amplifier-hx711-breakout-hookup-guide

// LoRa radio pins
#define RFM95_CS 10
#define RFM95_RST 9
#define RFM95_INT 3

volatile bool resFlag = false; //true when receipt matches personal ID

//super validator calibration variable (in order to estimate you will first need to run calibration Evaporometer)
#define calibration_factor -890 //This value is obtained using the Calibration sketch (grams)

//load cell variables
#define DOUT A4 //connecting the out and clock pins for the load cell
#define CLK A5

//load cell initialization
HX711 scale(DOUT, CLK);

// Change to 434.0 or other frequency, must match RX's freq!
#define RF95_FREQ 430.0

// Singleton instance of the radio driver
RH_RF95 rf95(RFM95_CS, RFM95_INT);


//packet length for radiopacket array
int16_t packet_len = 0;

//initialize transmitter id and light variables
int ID = 100;

//initialize sensor data variables
int loadData;

// initialize String values for transmit
String IDString, loadString, transmitString;

int transmitBufLen; //length of transmit buffer


void setup()
{

  //LoRa transmission
  pinMode(RFM95_RST, OUTPUT);
  digitalWrite(RFM95_RST, HIGH);
  Serial.begin(9600);
  while (!Serial); // waits for serial hardware to start up

  //report all sensors present on system
  Serial.println("Arduino LoRa TX Test!");
  Serial.println("HX711 scale");

  //manually reset LoRa
  digitalWrite(RFM95_RST, LOW);
  delay(10);
  digitalWrite(RFM95_RST, HIGH);
  delay(10);
  while (!rf95.init()) {
    Serial.println("LoRa radio init failed"); //if print wiring may be wrong
    while (1);
  }
  Serial.println("LoRa radio init OK!");
  // checks if frequency is initialized
  // Defaults after init are 434.0MHz, modulation GFSK_Rb250Fd250, +13dbM
  if (!rf95.setFrequency(RF95_FREQ)) {
    Serial.println("setFrequency failed");
    while (1);
  }
  Serial.print("Set Freq to: "); Serial.println(RF95_FREQ);

  // Defaults after init are 434.0MHz, 13dBm, Bw = 125 kHz, Cr = 4/5, Sf = 128chips / symbol, CRC on
  // The default transmitter power is 13dBm, using PA_BOOST.
  // If you are using RFM95/96/97/98 modules which uses the PA_BOOST transmitter pin, then
  // you can set transmitter powers from 5 to 23 dBm:
  rf95.setTxPower(23, false);

  // Load cell setup HX711
  // calibration
  scale.set_scale(calibration_factor); //This value is obtained by using the Calibration sketch
  scale.tare(); //Assuming there is no weight on the scale at start up, reset the scale to 0
}

void loop() {

  //load cell data:
  Serial.print("Reading..."); //You can change this to kg but you'll need to refactor the calibration_factor
  int16_t cell_data = (scale.get_units() * 10000); //scale.get_units() returns a float. The 4 after the comma tells the function to print 4 decimals.
  Serial.println(cell_data); //verify changed measurement
  Serial.println("Sending to rf95_server");

  //...can add more sensor data here later

  // convert data into strings
  IDString = String(ID, DEC);
  loadString = String(loadData, 6); //saves as string with 6 decimal places

  // concatenate strings into transmitString
  transmitString = String(IDString + "," + loadString + "\0"); // commas needed as delimiter for transmit
  transmitBufLen = 1 + (char)transmitString.length();

  // instantiate a transmit buffer of x len based on len of concat string above
  char transmitBuf[transmitBufLen];

  // copies long string of data into a created character array for transmitting
  transmitString.toCharArray(transmitBuf, transmitBufLen);

  // Send a message to rf95_server
  // We want each different radio packet to send a uniqe X digit id first and then send other sensor info:
  // transmit 3x and wait for device ID before giving up
  for (int i = 0; i < 3; i++) {
    int recID = 0; // will become the receipt from rx
    if (resFlag == false) {
      Serial.print("Sending ");
      Serial.println(transmitBuf);
      Serial.println("Sending value...");
      if (transmitBufLen > 0)
      {
        // sends data to receiver at this time
        rf95.send((uint8_t *)transmitBuf, transmitBufLen); //RF95 function sends the transmitBuf via radio (binary)
      }

      Serial.print("Attempt ");
      Serial.println(i + 1);
      Serial.println("Waiting for packet to complete...");

      rf95.waitPacketSent();
      // Now wait for a reply
      uint8_t buf[RH_RF95_MAX_MESSAGE_LEN];
      uint8_t len = sizeof(buf);
      // following codeexecutes after all receiver code runs
      
      if (rf95.waitAvailableTimeout(1000))
      {
        // Should be a reply message for us now
        if (rf95.recv(buf, &len))
        {
          recID = atoi(buf);
          if ((int)recID == ID) {
            Serial.println("ID confirmed. Message received! \n");
            Serial.println("Got reply: ");
            Serial.println("\nData received:");
            Serial.println((char*)buf);
            Serial.print("RSSI: ");
            Serial.println(rf95.lastRssi(), DEC); // prints RSSI as decimal value
            Serial.println();
            resFlag = true;
          }
        }
        else //happens when there is a receiver but bad message
        {
          Serial.println("Receive failed");
        }
      }
      else //happens when there is no receiver on the same freq to listen to
      {
        Serial.println("No reply, is there a listener around?");
      }
    }
    if (i == 2 && resFlag == false) { //print no receipt
      Serial.println("No receipts matched device ID. Try resending.");
    }
  }
  resFlag = false;
  delay(10000); //few seconds delay
} 

