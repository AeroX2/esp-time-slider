#include <Arduino.h>
#include <ArduinoJson.h>
#include <DNSServer.h>
#include <ElegantOTA.h>
#include <WebServer.h>
#include <WiFi.h>
#include <WiFiClient.h>
#include <WiFiManager.h>
#include <LittleFS.h>

#include <RemoteDebug.h>
RemoteDebug Debug;

#include "CustomAccelStepper.h"

WebServer server(80);

static ShiftRegister* shift_registers[4];
static CustomAccelStepper* steppers[8];

const int STEPS = 1128;

const int position_left[10] = {5, 0, 7, 9, 3, 11, 5, 1, 5, 11};
const int position_right[10] = {-6, 0, -11, -7, -4, -9, -9, -2, -1, -1};

// const float speedLeft[13] = {700,600,500,400,700,700,700,400,700,700,300,700,700};
// const float speedRight[13] = {600,700,700,700,400,500,300,700,300,300,600,300,400};

// const unsigned long durationLeft[13] = {11500,11000,17000,6500,11500,14500,11500,12000,8000,11500,8000,8000,11500};
// const unsigned long durationRight[13] = {11000,11500,19500,8000,9500,12500,0,13000,4500,0,11500,4500,9500};

static void moveMotor() {
  String message = server.arg("plain");  // Get the raw POST data
  debugD("Received POST message: %s", message);

  // Parse the JSON data
  JsonDocument jsonBuffer;  // Adjust the buffer size as needed
  DeserializationError error = deserializeJson(jsonBuffer, message);

  // Check for parsing errors
  if (error) {
    debugD("Failed to parse JSON: %s", error.c_str());
    server.send(400, "text/plain", "Failed to parse JSON");
    return;
  }

  // Extract values from JSON
  int motor = jsonBuffer["motor"];
  int amount = jsonBuffer["amount"];
  debugD("Moving motor %d, amount %d", motor, amount);

  if (motor < 0 || motor > 7) {
    debugD("Motor not found");
    return;
  }

  CustomAccelStepper* stepper = steppers[motor];
  if (motor % 2 == 0) {
    stepper->moveTo(stepper->currentPosition() + amount);
  } else {
    stepper->moveTo(stepper->currentPosition() - amount);
  }

  server.send(200, "text/plain", "OK");
}

static void setTime() {
  String message = server.arg("plain");  // Get the raw POST data
  // Parse the JSON data
  JsonDocument jsonBuffer;  // Adjust the buffer size as needed
  DeserializationError error = deserializeJson(jsonBuffer, message);

  // Check for parsing errors
  if (error) {
    debugD("Failed to parse JSON: %s", error.c_str());
    server.send(400, "text/plain", "Failed to parse JSON");
    return;
  }

  // Extract values from JSON
  int hour = jsonBuffer["hour"];
  int minute = jsonBuffer["minute"];

  struct tm timeinfo = {
      .tm_sec = 0,
      .tm_min = minute,
      .tm_hour = hour,
      .tm_mday = 1,
      .tm_mon = 0,
      .tm_year = 1970 - 1900,
      .tm_wday = 0,
      .tm_yday = 0,
      .tm_isdst = 0,
  };

  // Set the system time
  time_t timestamp = mktime(&timeinfo);
  struct timeval timeval = {
      .tv_sec = timestamp,
      .tv_usec = 0};
  settimeofday(&timeval, NULL);
}

boolean paused = false;
static void pauseTime() {
  paused = !paused;
  server.send(200, "text/plain", paused ? "PAUSED" : "NOT PAUSED");
}

void setup() {
  shift_registers[0] = new ShiftRegister(27, 32, 33);
  shift_registers[1] = new ShiftRegister(19, 21, 22);
  shift_registers[2] = new ShiftRegister(23, 25, 26);
  shift_registers[3] = new ShiftRegister(16, 17, 18);

  steppers[0] = new CustomAccelStepper(shift_registers[0], true);
  steppers[1] = new CustomAccelStepper(shift_registers[0], false);
  steppers[2] = new CustomAccelStepper(shift_registers[1], true);
  steppers[3] = new CustomAccelStepper(shift_registers[1], false);
  steppers[4] = new CustomAccelStepper(shift_registers[2], true);
  steppers[5] = new CustomAccelStepper(shift_registers[2], false);
  steppers[6] = new CustomAccelStepper(shift_registers[3], true);
  steppers[7] = new CustomAccelStepper(shift_registers[3], false);

  Serial.begin(115200);

  WiFi.hostname("james_time_slider");
  WiFiManager wm;
  bool res = wm.autoConnect();
  if (res) {
    // if you get here you have connected to the WiFi
    Serial.println("Wifi connected...yay! :)");
  } else {
    Serial.println("Failed to connect to Wifi :(");
    ESP.restart();
  }

  if (!LittleFS.begin()) {
    Serial.println("Error mounting file system");
    ESP.restart();
  }

  server.serveStatic("/time", LittleFS, "/index.html");
  server.on("/move", HTTP_POST, moveMotor);
  server.on("/pauseTime", HTTP_POST, pauseTime);
  server.on("/setTime", HTTP_POST, setTime);

  ElegantOTA.begin(&server);
  server.begin();

  for (ShiftRegister* shift_register : shift_registers) {
    shift_register->init();
  }

  for (CustomAccelStepper* stepper : steppers) {
    stepper->setMaxSpeed(5000.0);
    stepper->setAcceleration(200.0);
  }

  configTime(0, 0, "pool.ntp.org", "time.nist.gov");
  // https://github.com/nayarsystems/posix_tz_db;
  setenv("TZ", "AEST-10AEDT,M10.1.0,M4.1.0/3", 1);
  tzset();

  Debug.begin("james_time_slider");

  delay(3000);
  debugD("Ready!");
}

struct tm previous_time = {0};

void loop() {
  server.handleClient();

  for (CustomAccelStepper* stepper : steppers) {
    stepper->run();
    if (!stepper->isRunning()) {
      stepper->disableOutputs();
    }
  }

  struct tm timeinfo;
  if (!getLocalTime(&timeinfo)) {
    debugD("Failed to obtain time");
    return;
  }

  if (previous_time.tm_min != timeinfo.tm_min && !paused) {
    int left_h = timeinfo.tm_hour / 10;
    int right_h = timeinfo.tm_hour % 10;

    int left_m = timeinfo.tm_min / 10;
    int right_m = timeinfo.tm_min % 10;

    // steppers[0]->moveTo(position_left[left_h] * STEPS);
    // steppers[1]->moveTo(position_right[left_h] * STEPS);

    // steppers[2]->moveTo(position_left[right_h] * STEPS);
    // steppers[3]->moveTo(position_right[right_h] * STEPS);

    // steppers[4]->moveTo(position_left[left_m] * STEPS);
    // steppers[5]->moveTo(position_right[left_m] * STEPS);

    steppers[6]->moveTo(position_left[right_m] * STEPS);
    steppers[7]->moveTo(position_right[right_m] * STEPS);

    previous_time = timeinfo;
  }

  Debug.handle();
  ElegantOTA.loop();
  delay(1);
}
