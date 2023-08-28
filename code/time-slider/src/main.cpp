#include <Arduino.h>
#include <ElegantOTA.h>
#include <WebServer.h>
#include <WiFi.h>
#include <WiFiClient.h>
#include <WifiManager.h>

WebServer server(80);

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
}

void loop() {
  server.handleClient();

  delay(1);
}
