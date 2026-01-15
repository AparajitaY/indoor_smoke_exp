#define BLYNK_TEMPLATE_ID "TMPL3cxUMisX2"
#define BLYNK_TEMPLATE_NAME "Indoor Smoke Monitoring System"
#define BLYNK_AUTH_TOKEN "Tdmpo5-hp1wku_mLb3c4F72j3OlYw0nQ"
#define BLYNK_PRINT Serial
#include <WiFi.h>
#include <WiFiClient.h>
#include <BlynkSimpleEsp32.h>
#include "DHT.h"
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <ESP32Servo.h>
#include <Wire.h>
// ---------------- WiFi ----------------
char ssid[] = "hotspot name";
char pass[] = "pwd";
// ---------------- Pin Definitions ----------------
#define MQ135PIN 34
#define DHTPIN 14
#define SERVO_WINDOW_PIN 15
#define SERVO_FAN_PIN 4
#define LDR_PIN 33
#define BUZZER_PIN 12
// ---------------- Thresholds ----------------
const float TEMP_THRESHOLD = 35.0; // °C
const float HUM_THRESHOLD = 35.0; // %
const float SAFE_PPM = 200.0; // ppm
const float WARNING_PPM = 300.0; // ppm
 const int LDR_THRESHOLD = 750; // lower = darker
// ---------------- DHT Setup ----------------
#define DHTTYPE DHT11
 DHT dht(DHTPIN, DHTTYPE);
// ---------------- OLED ----------------
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET -1
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire,
OLED_RESET);
// ---------------- Servo ----------------
Servo windowServo;
Servo fanServo;
// ---------------- MQ135 Setup ----------------
const float VccSensor = 5.0;
const float RL = 10000.0;
const float DIVIDER_FACTOR = 0.5;
float R0 = 0.0;
const unsigned long CALIBRATION_TIME_MS = 15000UL; // 15s calibration
const unsigned long WARMUP_TIME_MS = 20000UL; // 20s warm-up
const int SMOOTH_COUNT = 8;
bool calibrated = false;
unsigned long startMillis;
// ---------------- Flags ----------------
bool manualControl = false;
bool windowOpen = false;
// ---------------- Blynk Input ----------------
BLYNK_WRITE(V3) {
int servoState = param.asInt();
if (servoState == 1) {
manualControl = true;
windowServo.write(90); // open manually
fanServo.write(180);
windowOpen = true;
Serial.println(" Manual control: Vent and fan ON");
} else {
manualControl = false;
windowServo.write(0); // close manually
fanServo.write(0);
windowOpen = false;
Serial.println(" Manual control OFF — auto resumes");
 }
}
// ---------------- Setup ----------------
void setup() {
Serial.begin(115200);
pinMode(BUZZER_PIN, OUTPUT);
digitalWrite(BUZZER_PIN, LOW);
dht.begin();
windowServo.attach(SERVO_WINDOW_PIN, 500, 2400);
fanServo.attach(SERVO_FAN_PIN, 500, 2400);
windowServo.write(0);
fanServo.write(0);
if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
Serial.println("OLED init failed!");
while (1);
 }
display.clearDisplay();
display.setTextSize(1);
display.setTextColor(SSD1306_WHITE);
display.setCursor(0, 0);
 display.println("Connecting WiFi...");
 display.display();
Blynk.begin(BLYNK_AUTH_TOKEN, ssid, pass);
display.clearDisplay();
display.setCursor(0, 0);
display.println("Blynk Connected!");
display.display();
Serial.println("\nMQ135 starting. Place sensor in clean air.");
Serial.print("Warmup: "); Serial.print(WARMUP_TIME_MS / 1000); Serial.print("s,
");
Serial.print("Calibration: "); Serial.print(CALIBRATION_TIME_MS / 1000);
Serial.println("s");
 startMillis = millis();
}
// ---------------- Loop ----------------
void loop() {
  Blynk.run();
if (!calibrated) {
 performCalibration();
return;
 }
float ppm = readMQ135ppm();
float h = dht.readHumidity();
float t = dht.readTemperature();
int ldrValue = analogRead(LDR_PIN);
if (isnan(h) || isnan(t)) return;
bool poorAir = (ppm >= WARNING_PPM);
bool tooHot = (t >= TEMP_THRESHOLD);
bool tooDark = (ldrValue < LDR_THRESHOLD);
bool hazardCondition = poorAir || tooHot || tooDark;
String status;
if (hazardCondition) status = "HAZARD";
else if (ppm >= SAFE_PPM) status = "WARNING";
else status = "SAFE";
// ---------- Auto Control (only when NOT manual) ----------
if (!manualControl) {
if (hazardCondition && !windowOpen) {
windowServo.write(90); // Open vent
fanServo.write(180); // Turn fan on
windowOpen = true;
 Serial.println("⚠ Auto: Vent and fan ON (hazard detected)");
} else if (!hazardCondition && windowOpen) {
windowServo.write(0); // Close vent
fanServo.write(0); // Stop fan
windowOpen = false;
Serial.println(" Auto: Vent and fan OFF (normal)");
}
}
// ---------- Buzzer ---------
if (!hazardCondition) noTone(BUZZER_PIN);
else tone(BUZZER_PIN, 1000);
 // ---------- OLED Display ----------
display.clearDisplay();
display.setCursor(0, 0);
display.printf("PPM: %.1f\nTemp: %.1fC\nHum: %.1f%%\n", ppm, t, h);
display.printf("LDR: %d\nStatus: %s\n", ldrValue, status.c_str());
display.display();
 // ---------- Blynk Updates ----------
Blynk.virtualWrite(V0, ppm);
Blynk.virtualWrite(V1, t);
Blynk.virtualWrite(V2, h);
 if (hazardCondition) Blynk.logEvent("poor_air", " Smoke/Fire risk detected!");
delay(2000);
}
// ---------------- MQ135 Functions ----------------
void performCalibration() {
unsigned long now = millis();
 if (now - startMillis < WARMUP_TIME_MS) {
 int remaining = (WARMUP_TIME_MS - (now - startMillis)) / 1000;
Serial.print("Warming up... "); Serial.print(remaining); Serial.println("s left");
display.clearDisplay();
display.setCursor(0, 0);
display.println("Warming up...");
display.setCursor(0, 20);
display.printf("%d s left", remaining);
display.display();
delay(1000);
return;
}
Serial.println("Warmup done. Calibrating...");
 display.clearDisplay();
 display.setCursor(0, 0);
 display.println("Calibrating...");
display.display();
unsigned long calibStart = millis();
long sum = 0; int count = 0;
while (millis() - calibStart < CALIBRATION_TIME_MS) {
 sum += analogRead(MQ135PIN);
count++;
delay(200);
 }
float avgAdc = (float)sum / count;
float vEsp = avgAdc * 3.3 / 4095.0;
 float vAout = vEsp / DIVIDER_FACTOR;
 float Rs = RL * (VccSensor - vAout) / vAout;
R0 = Rs / 10.0;
Serial.print("ADC avg: "); Serial.println(avgAdc, 1);
 Serial.print("R0: "); Serial.println(R0, 1);
 Serial.println("Calibration done.");
display.clearDisplay();
display.setCursor(0, 0);
display.println("Calibration Done!");
display.display();
calibrated = true;
}
float readMQ135ppm() {
long sum = 0;
for (int i = 0; i < SMOOTH_COUNT; i++) {
 sum += analogRead(MQ135PIN);
 delay(20); }
 float avgAdc = (float)sum / SMOOTH_COUNT;
 float vEsp = avgAdc * 3.3 / 4095.0;
float vAout = vEsp / DIVIDER_FACTOR;
 if (vAout <= 0) return NAN;
 float Rs = RL * (VccSensor - vAout) / vAout;
if (Rs <= 0) return NAN;
float ratio = Rs / R0;
float ppm_raw = 116.6020682 * pow(ratio, -2.769034857);
// Scale calibration so 0.0V → ~100ppm, 0.2V → ~300ppm
float ppm_calibrated = (400.0 * ppm_raw) + 80.0;
return ppm_calibrated;}
