#include <Arduino.h>
#include <ElegantOTA.h>
#include <WebServer.h>
#include <WiFi.h>
#include <WiFiClient.h>
#include <DNSServer.h>
#include <WiFiManager.h>

#include "CustomAccelStepper.h"

WebServer server(80);

// ShiftRegister sr1(16, 17, 18);
ShiftRegister sr2(19, 21, 22);
// ShiftRegister sr3(23, 25, 26);
// ShiftRegister sr4(27, 32, 33);

void setup() {
  Serial.begin(115200);

  WiFiManager wm;
  bool res = wm.autoConnect();
  if (res) {
    // if you get here you have connected to the WiFi
    Serial.println("Wifi connected...yay! :)");
  } else {
    Serial.println("Failed to connect to Wifi :(");
    ESP.restart();
  }

  server.on("/", []() { server.send(200, "text/plain", "Hi! I am ESP32."); });

  ElegantOTA.begin(&server);  // Start ElegantOTA
  server.begin();

  // sr1.init();
  sr2.init();
  // sr3.init();
  // sr4.init();
  sr2.stepMotor1(0b1111);
}

int a = 0;
void loop() {
  server.handleClient();

  // sr1.stepMotor1(0b1);
  // sr1.stepMotor2(0b1);
  // sr3.stepMotor1(0b1111);
  // sr3.stepMotor2(0b1111);
  // sr4.stepMotor1(0b1);
  // sr4.stepMotor2(0b1);

  delay(1);
}
