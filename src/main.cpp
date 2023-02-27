#include "esp_system.h"
#include <Arduino.h>
#include <LoRa.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

#define SCREEN_WIDTH 128    // OLED display width, in pixels
#define SCREEN_HEIGHT 64    // OLED display height, in pixels
#define OLED_RESET -1       // Reset pin # (or -1 if sharing Arduino reset pin)

#define csPin 5          // LoRa radio chip select ss
#define resetPin 14       // LoRa radio reset
#define irqPin 2         // change for your board; must be a hardware interrupt pin pio0

#define localAddress 0xFF     // address of this device
#define destination 0xBB      // destination to send to

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

long lastSendTime = 0;        // last send time
int interval = 2000;          // interval between sends
static TaskHandle_t xTaskToNotify = NULL;


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

void IRAM_ATTR onReceive(int packetSize) {
  BaseType_t xHigherPriorityTaskWoken = pdFALSE;

  if (packetSize == 0) return;          // if there's no packet, return
  configASSERT( xTaskToNotify != NULL );
  vTaskNotifyGiveFromISR(xTaskToNotify, &xHigherPriorityTaskWoken);
  // context switch
  if(xHigherPriorityTaskWoken) portYIELD_FROM_ISR();
}

void parseLora(void * pvParameters ) {
  while (true) {
    if (ulTaskNotifyTake(pdTRUE, portMAX_DELAY)){
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
      displayText("RSSI: " + String(LoRa.packetRssi()));
      displayText("Snr: " + String(LoRa.packetSnr()));
      // display.clearDisplay();
      // display.setTextSize(1);
      // display.setTextColor(WHITE);
      // display.setCursor(0, 0);
      // display.println("From: 0x" + String(sender, HEX) + " To: 0x" + String(recipient, HEX));
      // display.println("Message ID: " + String(incomingMsgId));
      // display.println("RSSI: " + String(LoRa.packetRssi()));
      // display.println("Snr: " + String(LoRa.packetSnr()));
      // display.display();
    }
  }
}


void setupLora(int ss, int reset, int dio0) {
  LoRa.setPins(ss, reset, dio0);// set CS, reset, IRQ pin
  while (!LoRa.begin(433E6)) delay(100);  // initialize ratio at 433 MHz
}


void setup() {
  Serial.begin(9600);
  // HW setup
  static uint8_t ucParameterToPass;

  xTaskCreate(
    parseLora, //  Pointer to the task entry function.
    "Lora_Reciever", //  A descriptive name for the task.
    2048, // The size of the task stack specified as the number of bytes
    &ucParameterToPass, // Pointer that will be used as the parameter for the task being created.
    tskIDLE_PRIORITY, // The priority at which the task should run.
    &xTaskToNotify // Used to pass back a handle by which the created task can be referenced.
  );

  displayText("LoRa Receiver", 0, 10);

  setupLora(csPin, resetPin, irqPin);
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