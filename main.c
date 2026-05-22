#define BLYNK_TEMPLATE_ID "TMPL61az8VbNP"
#define BLYNK_TEMPLATE_NAME "BPM SpO2 machine"
#define BLYNK_AUTH_TOKEN "sh7eSZbwpXrlJGwq_2tU68fdkG10lZ_T"
#include <WiFi.h>
#include <WiFiClient.h>
#include <BlynkSimpleEsp32.h>
// SENSOR LIBRARIES 
#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include "MAX30105.h"
MAX30105 particleSensor;
LiquidCrystal_I2C lcd(0x27, 16, 2);  // 16x2 LCD with I2C address 0x27 (try 0x3F if not working)
// ESP32 WROOM PIN DEFINITIONS 
#define I2C_SDA 21
#define I2C_SCL 22
#define MAX30102_INT 4
// WIFI CREDENTIALS 
char ssid[] = "Ali";
char pass[] = "12345678";
// Smoothing buffers 
const int AVG_SIZE = 10;
float spo2Buffer[AVG_SIZE];
int indexAvg = 0;
float smooth(float *buffer, float value) {
  buffer[indexAvg] = value;
  float sum = 0;
  for (int i = 0; i < AVG_SIZE; i++) sum += buffer[i];
  return sum / AVG_SIZE;
}
// BP estimation (not real medical!) 
int estimateBP(int spo2) {
  int sys = map(spo2, 80, 100, 110, 135);
  if (sys < 90) sys = 90;
  if (sys > 150) sys = 150;
  return sys;
}
void setup() {
  Serial.begin(115200);
  Serial.println("\n=== Heart Rate Monitor ===");
  Serial.println("ESP32 WROOM-32");
  Serial.println("MAX30102 Sensor");
  Serial.println("16x2 LCD Display");
  // Initialize I2C with ESP32 WROOM pins
  Wire.begin(I2C_SDA, I2C_SCL);
  Serial.print("I2C initialized: SDA=GPIO");
  Serial.print(I2C_SDA);
  Serial.print(", SCL=GPIO");
  Serial.println(I2C_SCL);
  // LCD 16x2 setup 
  lcd.init();
  lcd.backlight();
  lcd.setCursor(0, 0);
  lcd.print("Initializing...");
  lcd.setCursor(0, 1);
  lcd.print("Please wait");
  Serial.println("LCD initialized");
  // MAX30102 setup 
  if (!particleSensor.begin(Wire, I2C_SPEED_FAST)) {
    Serial.println("MAX30102 NOT FOUND");
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("MAX30102 ERROR!");
    lcd.setCursor(0, 1);
    lcd.print("Check wiring");
    while (1) {
      delay(1000);
    }
  }
  Serial.println("MAX30102 detected!");
  particleSensor.setup();
  particleSensor.setPulseAmplitudeRed(0x1F);
  particleSensor.setPulseAmplitudeIR(0x1F);
  // Initialize smoothing buffer
  for (int i = 0; i < AVG_SIZE; i++) spo2Buffer[i] = 0;
  // BLYNK + WIFI 
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Connecting WiFi");
  Serial.println("Connecting to WiFi...");
  Blynk.begin(BLYNK_AUTH_TOKEN, ssid, pass);
  Serial.println("WiFi Connected!");
  Serial.println("Blynk Connected!");
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("WiFi Connected!");
  delay(2000);
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Salam Umar Aslam, ");
  lcd.setCursor(0, 1);
  lcd.print("place your finger");
  Serial.println("\n=== Ready! Place finger on sensor ===\n");
}
void loop() {
  Blynk.run();   // Keep Blynk connected
  long irValue = particleSensor.getIR();
  long redValue = particleSensor.getRed();
  // Debug output every 2 seconds
  static unsigned long lastDebug = 0;
  if (millis() - lastDebug > 2000) {
    Serial.print("IR: ");
    Serial.print(irValue);
    Serial.print(" | Red: ");
    Serial.print(redValue);
    lastDebug = millis();
  }
  if (irValue < 5000) {
    lcd.setCursor(0, 0);
    lcd.print("Salam Umar Aslam, ");
    lcd.setCursor(0, 1);
    lcd.print("place your finger      ");
    // Send zeros to Blynk
    Blynk.virtualWrite(V1, 0);
    Blynk.virtualWrite(V0, 0);
    Serial.println(" | Status: NO FINGER");
    delay(200);
    return;
  }
  // SpO2 calculation (SAME AS WORKING CODE)
  double ratio = (double)redValue / (double)irValue;
  int spo2 = -45.06 * ratio * ratio + 30.354 * ratio + 94.845;
  spo2 = constrain(spo2, 70, 100);
  // Smoothing
  indexAvg = (indexAvg + 1) % AVG_SIZE;
  int smoothSp = smooth(spo2Buffer, spo2);
  // Estimate BP
  int bp = estimateBP(smoothSp);
  // LCD 16x2 display 
  lcd.setCursor(0, 0);
  lcd.print("SpO2:");
  lcd.print(smoothSp);
  lcd.print("%");
  lcd.print("  ");  // Clear extra characters
  lcd.setCursor(0, 1);
  lcd.print("BP:");
  lcd.print(bp);
  lcd.print(" mmHg");
  lcd.print("  ");  // Clear extra characters
  //SEND VALUES TO BLYNK
  Blynk.virtualWrite(V0, smoothSp);  // SpO2
  Blynk.virtualWrite(V1, bp);        // BP
  // Debug output
  Serial.print(" | SpO2: ");
  Serial.print(smoothSp);
  Serial.print("% | BP: ");
  Serial.print(bp);
  Serial.println(" mmHg");
  delay(100);
}
