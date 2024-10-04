// The libary setup, I replace <WiFi.h> with <Wifi101.h> since the ESP 32 is not invovle.
// I remove all BLE-related code
// I will remove EEPROM code if not using for saving WiFi credentials
#include <Arduino.h>
#include <OneWire.h>
#include <DallasTemperature.h>
#include <WiFi101.h>
#include <ThingSpeak.h>

// Pin Definitions
#define ONE_WIRE_BUS 2  // Data pin for DS18B20
#define RED_LED 8       // LED indicators
#define ORANGE_LED 9
#define BLUE_LED 10
#define BUTTON_PIN 7    // Button pin

// Temperature Sensor Setup
OneWire oneWire(ONE_WIRE_BUS);
DallasTemperature DS18B20(&oneWire);

// WiFi Credentials, In which Im still trying to figure out
const char* ssid = "Placeholder-SSID";
const char* password = "Placeholder-PASSWORD";

// ThingSpeak Channel Info, As same as above Im still trying to understand how I would achieve the API key acess, maybe im dumb
const char* apiKey_write = "THE_WRITE_API_KEY";
const char* server = "api.thingspeak.com";
unsigned long channelID_write = My_CHANNEL_ID;
int field = 1;  // Field in ThingSpeak to write the temperature data

// WiFi client
WiFiClient client;

// Temperature Adjustment Constants
const float refHigh = 200.3; // Reference temperature of boiling at Durango, CO
const float refLow = 32.0;   // Reference temperature of freezing
const float rawHigh = 199.96; // Measured raw sensor value at boiling point
const float rawLow = 32.34;   // Measured raw sensor value at freezing point

// Timers
const long intervalUpload = 900000; // 15 minutes
unsigned long previousMillisUpload = 0;

// LED Functions
void red() {
  digitalWrite(RED_LED, HIGH); digitalWrite(ORANGE_LED, LOW); digitalWrite(BLUE_LED, LOW);
}
void orange() {
  digitalWrite(RED_LED, LOW); digitalWrite(ORANGE_LED, HIGH); digitalWrite(BLUE_LED, LOW);
}
void blue() {
  digitalWrite(RED_LED, LOW); digitalWrite(ORANGE_LED, LOW); digitalWrite(BLUE_LED, HIGH);
}
void success() {
  digitalWrite(RED_LED, LOW); digitalWrite(ORANGE_LED, HIGH); digitalWrite(BLUE_LED, HIGH);
}
void fail() {
  digitalWrite(RED_LED, HIGH); digitalWrite(ORANGE_LED, LOW); digitalWrite(BLUE_LED, HIGH);
}
void off() {
  digitalWrite(RED_LED, LOW); digitalWrite(ORANGE_LED, LOW); digitalWrite(BLUE_LED, LOW);
}

// Function to Adjust Raw Temperature Readings
float TempAdjust(float rawTemp){
  float RawRange = rawHigh - rawLow;
  float ReferenceRange = refHigh - refLow;
  float CorrectedValue = (((rawTemp - rawLow) * ReferenceRange) / RawRange) + refLow;
  return CorrectedValue;
}

// WiFi Connection Function
void connectToWiFi() {
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("WiFi connected");
}

// Function to Send Data to ThingSpeak
void sendTemperatureToThingSpeak(float temperature) {
  if (WiFi.status() == WL_CONNECTED) {
    if (client.connect(server, 80)) {
      String postStr = "api_key=" + String(apiKey_write);
      postStr += "&field" + String(field) + "=" + String(temperature);
      postStr += "\r\n\r\n";

      client.print("POST /update HTTP/1.1\n");
      client.print("Host: api.thingspeak.com\n");
      client.print("Connection: close\n");
      client.print("Content-Type: application/x-www-form-urlencoded\n");
      client.print("Content-Length: ");
      client.print(postStr.length());
      client.print("\n\n");
      client.print(postStr);

      Serial.println("Data sent to ThingSpeak");
    }
    client.stop();
  } else {
    Serial.println("WiFi not connected");
  }
}

void setup() {
  // Initialize Serial communication
  Serial.begin(9600);
  
  // Initialize the sensor
  DS18B20.begin();
  
  // Initialize the LED pins
  pinMode(RED_LED, OUTPUT);
  pinMode(ORANGE_LED, OUTPUT);
  pinMode(BLUE_LED, OUTPUT);
  
  // Turn off LEDs initially
  off();

  // Connect to WiFi
  connectToWiFi();
  
  // Initialize ThingSpeak
  ThingSpeak.begin(client);
}

void loop() {
  unsigned long currentMillis = millis();
  
  // Read and upload temperature every 15 minutes
  if (currentMillis - previousMillisUpload >= intervalUpload) {
    // Request temperature readings
    DS18B20.requestTemperatures();
    
    // Get the temperature in Celsius and convert to Fahrenheit
    float rawTempC = DS18B20.getTempCByIndex(0);
    float rawTempF = (rawTempC * 1.8) + 32;
    
    // Adjust the temperature using calibration values
    float correctedTempF = TempAdjust(rawTempF);
    
    // Send the adjusted temperature to ThingSpeak
    sendTemperatureToThingSpeak(correctedTempF);

    // Success indicator
    success();
    
    // Update the timer
    previousMillisUpload = currentMillis;
  } else {
    off(); // LEDs off between uploads
  }
  
  delay(2500); // Delay to avoid overwhelming the loop
}
