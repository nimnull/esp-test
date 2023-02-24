#include "esp_system.h"
#include <Arduino.h>
#include <LoRa.h>

#define csPin 5          // LoRa radio chip select ss
#define resetPin 14       // LoRa radio reset
#define irqPin 2         // change for your board; must be a hardware interrupt pin pio0

#define localAddress 0xBB     // address of this device
#define destination 0xFF      // destination to send to
// #define localAddress 0xFF     // address of this device
// #define destination 0xBB      // destination to send to

long lastSendTime = 0;        // last send time
int interval = 2000;          // interval between sends

void sendMessage(String outgoing, uint32_t msgId) {
  LoRa.beginPacket();                   // start packet
  LoRa.write(destination);              // add destination address
  LoRa.write(localAddress);             // add sender address
  LoRa.write(msgId);                 // add message ID
  LoRa.write(outgoing.length());        // add payload length
  LoRa.print(outgoing);                 // add payload
  LoRa.endPacket();                     // finish packet and send it
}

void displayText(String text, int16_t x = 0, int16_t y = 0) {
  Serial.println(text);
}

void onReceive(int packetSize) {
  if (packetSize == 0) return;          // if there's no packet, return

  // read packet header bytes:
  int recipient = LoRa.read();          // recipient address
  byte sender = LoRa.read();            // sender address
  byte incomingMsgId = LoRa.read();     // incoming msg ID
  byte incomingLength = LoRa.read();    // incoming msg length

  String incoming = "";

  while (LoRa.available()) {
    incoming += (char)LoRa.read();
  }

  if (incomingLength != incoming.length()) {   // check length for error
    displayText("error: message length does not match");
    return;                             // skip rest of function
  }

  // if the recipient isn't this device or broadcast,
  if (recipient != localAddress && recipient != 0xFF) {
    displayText("This message is not for me.");
    return;                             // skip rest of function
  }
}


void setupLora(int ss, int reset, int dio0) {
  LoRa.setPins(ss, reset, dio0);// set CS, reset, IRQ pin
  while (!LoRa.begin(433E6)) delay(100);  // initialize ratio at 433 MHz
  LoRa.setGain(6);
  LoRa.setTxPower(20);
}


void setup() {
  Serial.begin(9600);
  // HW setup
  setupLora(csPin, resetPin, irqPin);

  displayText("LoRa Receiver", 0, 10);
  LoRa.onReceive(onReceive);
  LoRa.receive();
}

void loop() {
  
  if (millis() - lastSendTime > interval) {    
    String message = String("HeLoRa World! ") + String(interval);   // send a message
    sendMessage(message, interval);
    lastSendTime = millis();            // timestamp the message
    interval = random(2000) + 1000;     // 2-3 seconds
    LoRa.receive();                     // go back into receive mode
  }
}