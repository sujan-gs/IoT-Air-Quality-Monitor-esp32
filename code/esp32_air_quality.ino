#include <WiFi.h>
#include <HTTPClient.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <DHT.h>

// 1. USER CONFIGURATION

const char* WIFI_SSID     = "ssid";
const char* WIFI_PASS     = "password";

const char* AIO_USERNAME  = "username";
const char* AIO_KEY       = "aio_key-------";

// Adafruit Feed Names
const char* FEED_MQ135    = "mq135";
const char* FEED_MQ07     = "mq07";
const char* FEED_PM25     = "dust-density";
const char* FEED_TEMP     = "temperature";
const char* FEED_HUMID    = "humidity";

// 2. PIN DEFINITIONS

const int PIN_MQ135    = 34;
const int PIN_MQ07     = 35;
const int PIN_DUST_VO  = 32; // Direct Wire (No Voltage Divider)
const int PIN_DUST_LED = 25;
const int PIN_DHT      = 4;

// 3. OBJECTS & VARIABLES

LiquidCrystal_I2C lcd(0x27, 16, 2);
#define DHTTYPE DHT22
DHT dht(PIN_DHT, DHTTYPE);

unsigned long lastSendTime = 0;
const unsigned long SEND_INTERVAL_MS = 20000; // 10 Seconds
const unsigned long LCD_UPDATE_MS = 2000;
unsigned long lastLCDUpdate = 0;

// Variables to store sensor data
float smoothMQ135 = 0;
float smoothMQ07  = 0;
float smoothPM25  = 0;
float temp = 0;
float hum = 0;
const float ALPHA = 0.18; // Smoothing factor (0.1 to 1.0)

// Function Declarations
float measureDust();
float readMQRaw(int pin);
bool postToAdafruit(const char* feed_key, float value);
void printLCDLine(int row, const String &s);

void setup() {
  // Start Serial Monitor
  Serial.begin(115200);
  delay(1000);
  
  Serial.println("\n\n--- SYSTEM STARTING ---");

  // Init Pins
  pinMode(PIN_DUST_LED, OUTPUT);
  pinMode(PIN_DUST_VO, INPUT);
  digitalWrite(PIN_DUST_LED, HIGH); // Turn LED Off initially

  // Init LCD
  lcd.init();
  lcd.backlight();
  printLCDLine(0, "System Start...");
  
  // Init DHT
  dht.begin();
  
  // Connect WiFi
  Serial.print("Connecting to WiFi: ");
  Serial.println(WIFI_SSID);
  
  WiFi.begin(WIFI_SSID, WIFI_PASS);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nWiFi Connected!");
  Serial.print("IP Address: ");
  Serial.println(WiFi.localIP());
  
  printLCDLine(0, "WiFi Connected!");
  delay(2000);
  lcd.clear();
}

void loop() {
  unsigned long now = millis();

  // READ ALL SENSORS

  // A. DHT22
  float t_read = dht.readTemperature();
  float h_read = dht.readHumidity();
  if (!isnan(t_read)) temp = t_read;
  if (!isnan(h_read)) hum = h_read;

  // B. MQ Sensors (Raw Analog)
  float rawMQ135 = readMQRaw(PIN_MQ135);
  float rawMQ07  = readMQRaw(PIN_MQ07);
  
  // Smoothing Math (Prevents numbers from jumping too fast)
  if (smoothMQ135 == 0) smoothMQ135 = rawMQ135; 
  else smoothMQ135 = ALPHA * rawMQ135 + (1.0 - ALPHA) * smoothMQ135;
  
  if (smoothMQ07 == 0) smoothMQ07 = rawMQ07; 
  else smoothMQ07 = ALPHA * rawMQ07 + (1.0 - ALPHA) * smoothMQ07;

  // C. Dust Sensor
  float dust_read = measureDust();
  if (smoothPM25 == 0) smoothPM25 = dust_read;
  else smoothPM25 = ALPHA * dust_read + (1.0 - ALPHA) * smoothPM25;

  
  // UPDATE LCD (Fast Update)

  if (now - lastLCDUpdate >= LCD_UPDATE_MS) {
    lastLCDUpdate = now;
    
    char line1[17];
    char line2[17];
    
    // Format: T:25.5 H:60%
    snprintf(line1, sizeof(line1), "T:%.1f H:%.0f%%", temp, hum);
    // Format: AQ:1200 D:15
    snprintf(line2, sizeof(line2), "AQ:%d D:%.0f", (int)smoothMQ135, smoothPM25);
    
    printLCDLine(0, String(line1));
    printLCDLine(1, String(line2));
  }

  // SERIAL MONITOR & UPLOAD (Every 10s)
  
  if (now - lastSendTime >= SEND_INTERVAL_MS) {
    lastSendTime = now;
    
    // PRINT TO SERIAL MONITOR
    Serial.println("\n========================================");
    Serial.println("         SENSOR READINGS REPORT         ");
    Serial.println("========================================");
    
    Serial.print("1. Temperature:   "); Serial.print(temp); Serial.println(" C");
    Serial.print("2. Humidity:      "); Serial.print(hum); Serial.println(" %");
    Serial.print("3. MQ-135 (Air):  "); Serial.print(smoothMQ135); Serial.println(" (Raw 0-4095)");
    Serial.print("4. MQ-07  (CO):   "); Serial.print(smoothMQ07); Serial.println(" (Raw 0-4095)");
    Serial.print("5. Dust Density:  "); Serial.print(smoothPM25); Serial.println(" ug/m3");
    
    Serial.println("----------------------------------------");
    Serial.println("Uploading to Adafruit IO...");

    // Upload Data
    postToAdafruit(FEED_MQ135, smoothMQ135);
    postToAdafruit(FEED_MQ07, smoothMQ07);
    postToAdafruit(FEED_PM25, smoothPM25);
    postToAdafruit(FEED_TEMP, temp);
    postToAdafruit(FEED_HUMID, hum);
    
    Serial.println("Upload Complete!");
    Serial.println("========================================\n");
  }
  
  delay(10); // Small stability delay
}

// FUNCTIONS

// Dust Sensor Measurement (Direct Connection Logic)
float measureDust() {
  digitalWrite(PIN_DUST_LED, HIGH); // Turn LED ON
  delayMicroseconds(280);
  
  int adc = analogRead(PIN_DUST_VO);
  
  delayMicroseconds(40);
  digitalWrite(PIN_DUST_LED, LOW); // Turn LED OFF
  delayMicroseconds(9680);

  // Convert to Voltage
  float voltage = adc * (3.3 / 4095.0);
  // Formula: 0.17 * Voltage - 0.1 (Result in mg/m3)
  // Multiply by 1000 to get ug/m3
  float dust = (0.17 * voltage) * 1000;
  
  if (dust < 0) dust = 0;
  return dust;
}

// Read raw analog value
float readMQRaw(int pin) {
  return (float)analogRead(pin); 
}

// Upload to Adafruit
bool postToAdafruit(const char* feed_key, float value) {
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("Error: WiFi not connected.");
    return false;
  }
  
  HTTPClient http;
  String url = String("https://io.adafruit.com/api/v2/") + AIO_USERNAME + "/feeds/" + feed_key + "/data";
  
  http.begin(url);
  http.addHeader("Content-Type", "application/json");
  http.addHeader("X-AIO-Key", AIO_KEY);
  
  String payload = String("{\"value\":") + String(value, 3) + String("}");
  
  int code = http.POST(payload);
  http.end();
  
  if (code == 200 || code == 201) return true;
  else {
    Serial.print("Error uploading to "); 
    Serial.print(feed_key);
    Serial.print(" - Code: ");
    Serial.println(code);
    return false;
  }
}

// Helper to clear LCD line and print
void printLCDLine(int row, const String &s) {
  lcd.setCursor(0, row);
  lcd.print("                "); // Clear line with spaces
  lcd.setCursor(0, row);
  lcd.print(s);
}
