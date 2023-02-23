#include "esp_system.h"
#include <Arduino.h>
#include <LoRa.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

#define SCREEN_WIDTH 128    // OLED display width, in pixels
#define SCREEN_HEIGHT 64    // OLED display height, in pixels
#define OLED_RESET -1       // Reset pin # (or -1 if sharing Arduino reset pin)
// #define LED 2
#define csPin 5          // LoRa radio chip select ss
#define resetPin 14       // LoRa radio reset
#define irqPin 2         // change for your board; must be a hardware interrupt pin pio0

// #define localAddress 0xBB     // address of this device
// #define destination 0xFF      // destination to send to
#define localAddress 0xFF     // address of this device
#define destination 0xBB      // destination to send to

long lastSendTime = 0;        // last send time
int interval = 2000;          // interval between sends

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

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
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(WHITE);
  display.setCursor(x, y);
  display.println(text);
  display.display();  
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

  // if message is for this device, or broadcast, print details:
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(WHITE);
  display.setCursor(0, 0);
  display.println("From: 0x" + String(sender, HEX) + " To: 0x" + String(recipient, HEX));
  display.println("Message ID: " + String(incomingMsgId));
  display.println("RSSI: " + String(LoRa.packetRssi()));
  display.println("Snr: " + String(LoRa.packetSnr()));
  display.display();
}


void setupLora(int ss, int reset, int dio0) {
  LoRa.setPins(ss, reset, dio0);// set CS, reset, IRQ pin
  while (!LoRa.begin(433E6)) // initialize ratio at 433 MHz
  {
    // digitalWrite(LED, HIGH);
    delay(100);
    // digitalWrite(LED, LOW);
  }
}


void setup() {
  // HW setup
  while(!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) delay(100);
  
  setupLora(csPin, resetPin, irqPin);
  
  displayText("LoRa Receiver", 0, 10);
  // LoRa.onReceive(onReceive);
  // LoRa.receive();
}

void loop() {
  
  if (millis() - lastSendTime > interval) {    
    String message = String("HeLoRa World! ") + String(interval);   // send a message
    sendMessage(message, interval);
    lastSendTime = millis();            // timestamp the message
    interval = random(2000) + 1000;     // 2-3 seconds
    // LoRa.receive();                     // go back into receive mode
  }
  onReceive(LoRa.parsePacket());

}