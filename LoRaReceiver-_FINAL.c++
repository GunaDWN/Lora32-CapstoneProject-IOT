#include <SPI.h>
#include <LoRa.h>
#include <WiFi.h>
#include <PubSubClient.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <ArduinoJson.h>  // Tambahkan library ArduinoJson untuk parsing JSON

// WiFi credentials
const char* ssid = "JTI-POLINEMA";
const char* password = "jtifast!";

// MQTT broker details
const char* mqtt_server = "192.168.75.197";
const int mqtt_port = 1883;
const char* mqtt_topic = "iot/sensors";

// MQTT authentication
const char* mqtt_username = "capstone";
const char* mqtt_password = "bismillahpulang";

// LoRa pin configuration (specific to TTGO LoRa32)
#define SCK     5
#define MISO    19
#define MOSI    27
#define SS      18
#define RST     14
#define DI0     26
#define BAND    433E6

// OLED display configuration
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);

// MQTT client
WiFiClient espClient;
PubSubClient client(espClient);

void setup_wifi() {
  delay(10);
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
  }
}

void reconnect() {
  while (!client.connected()) {
    if (client.connect("LoRaReceiverClient", mqtt_username, mqtt_password)) {
    } else {
      delay(5000);
    }
  }
}

void setup() {
  // Start serial communication
  Serial.begin(115200);
  
  // Start WiFi connection
  setup_wifi();
  
  // Set the MQTT server to connect to
  client.setServer(mqtt_server, mqtt_port);

  // Initialize LoRa communication
  SPI.begin(SCK, MISO, MOSI, SS);
  LoRa.setPins(SS, RST, DI0);
  if (!LoRa.begin(BAND)) {
    Serial.println("Starting LoRa failed!");
    while (1);
  }
  
  // Initialize OLED display
  if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
    Serial.println(F("SSD1306 allocation failed"));
    for (;;);
  }
  
  // Clear the display
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(0, 0);
  display.print("Waiting for data...");
  display.display();
}

void loop() {
  if (!client.connected()) {
    reconnect();
  }
  client.loop();

  int packetSize = LoRa.parsePacket();
  if (packetSize) {
    String receivedData = "";
    
    while (LoRa.available()) {
      receivedData += (char)LoRa.read();
    }

    Serial.print("Received packet: ");
    Serial.println(receivedData);

    // Clear display before showing actual data
    display.clearDisplay();
    display.setCursor(0, 0);

    // Tampilkan "Data received" di atas data
    display.print("Data received");  // Menampilkan pesan "Data received" di bagian atas layar OLED
    display.setCursor(0, 10);  // Pindahkan kursor ke bawah untuk menampilkan data selanjutnya
    
    // Parse JSON data
    StaticJsonDocument<256> doc;
    DeserializationError error = deserializeJson(doc, receivedData);
    
    if (!error) {
      // Retrieve values from JSON
      float temperature = doc["temperature"];
      float humidity = doc["humidity"];
      float soilMoisture = doc["soil_moisture"];
      float raindrop = doc["raindrop"];
      
      // Display the data on OLED
      display.print("Temp: ");  // Menampilkan nilai suhu di OLED
      display.print(temperature);
      display.println(" C");
      
      display.print("Humidity: ");  // Menampilkan nilai kelembaban di OLED
      display.print(humidity);
      display.println(" %");
      
      display.print("Soil Moist: ");  // Menampilkan nilai kelembaban tanah di OLED
      display.print(soilMoisture);
      display.println(" %");
      
      display.print("Raindrop: ");  // Menampilkan nilai curah hujan di OLED
      display.print(raindrop);
      display.println(" %");
    } else {
      Serial.println("Failed to parse JSON");
      display.println("Invalid JSON");
    }
    
    display.display();  // Update OLED display with the new data

    // Verifikasi apakah data dalam format JSON
    if (receivedData[0] == '{' && receivedData[receivedData.length() - 1] == '}') {
      if (client.publish(mqtt_topic, receivedData.c_str())) {
        Serial.println("Data sent to MQTT broker");
      } else {
        Serial.println("Failed to send data to MQTT broker");
      }
    } else {
      Serial.println("Invalid JSON format");
    }
  }
}
