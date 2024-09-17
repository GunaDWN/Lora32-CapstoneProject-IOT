
#include <Wire.h>
#include <Adafruit_Sensor.h>
#include <DHT.h>
#include <DHT_U.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <LoRa.h>

// Define OLED display width and height, based on your OLED
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64

// Define pin numbers
#define DHTPIN 14        // Pin DHT11
#define DHTTYPE DHT11    // Tipe sensor DHT11
#define SOIL_PIN 34      // Pin Soil Moisture (analog)
#define RAIN_PIN 35      // Pin Raindrop Sensor (analog)

// LoRa pin configuration (specific to TTGO LoRa32)
#define SCK     5
#define MISO    19
#define MOSI    27
#define SS      18
#define RST     14
#define DI0     26
#define BAND    433E6  // Frequency band for LoRa (adjust for your region)

// Create an instance of DHT sensor
DHT dht(DHTPIN, DHTTYPE);

// Create an instance for the OLED display
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);

void setup() {
  // Start the serial communication
  Serial.begin(115200);
  
  // Start the DHT sensor
  dht.begin();
  
  // Initialize OLED display with I2C address 0x3C
  if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
    Serial.println(F("SSD1306 allocation failed"));
    for (;;);
  }
  
  // Initialize LoRa communication
  SPI.begin(SCK, MISO, MOSI, SS);
  LoRa.setPins(SS, RST, DI0);
  if (!LoRa.begin(BAND)) {
    Serial.println("Starting LoRa failed!");
    while (1);
  }

  // Set LoRa sync word for private network (0xF3 is just an example)
  LoRa.setSyncWord(0xF3);
  
  // Clear the display buffer
  display.clearDisplay();
  display.setTextSize(1);      
  display.setTextColor(SSD1306_WHITE); 
  display.setCursor(0, 0);     
  display.print("Sensor Data Logging Started");
  display.display();
  delay(2000); // Pause for 2 seconds
}

void loop() {
  // Read temperature and humidity from DHT11
  float temperature = dht.readTemperature();
  float humidity = dht.readHumidity();
  
  // Check if any reads failed
  if (isnan(temperature) || isnan(humidity)) {
    Serial.println("Failed to read from DHT sensor!");
    return;
  }
  
  // Read soil moisture and convert to percentage (0-100%)
  int soilMoistureValue = analogRead(SOIL_PIN);
  float soilMoisture = (soilMoistureValue / 4095.0) * 100;

  // Read raindrop sensor and convert to percentage (0-100%)
  int raindropValue = analogRead(RAIN_PIN);
  float raindrop = (raindropValue / 4095.0) * 100;
  
  // Print sensor readings to Serial Monitor
  Serial.print("Temperature: ");
  Serial.print(temperature);
  Serial.print(" C, Humidity: ");
  Serial.print(humidity);
  Serial.print(" %, Soil Moisture: ");
  Serial.print(soilMoisture);
  Serial.print(" %, Raindrop: ");
  Serial.println(raindrop);

  // Clear OLED display
  display.clearDisplay();
  
  // Set cursor position
  display.setCursor(0, 0);
  
  // Display temperature and humidity
  display.print("Temp: ");
  display.print(temperature);
  display.println(" C");
  
  display.print("Humidity: ");
  display.print(humidity);
  display.println(" %");
  
  // Display soil moisture
  display.print("Soil Moist: ");
  display.print(soilMoisture);
  display.println(" %");
  
  // Display raindrop sensor value
  display.print("Raindrop: ");
  display.print(raindrop);
  display.println(" %");
  
  // Update the display
  display.display();

  // Prepare data to send via LoRa in JSON format
  String dataToSend = "{\"temperature\":" + String(temperature, 2) + 
                      ",\"humidity\":" + String(humidity, 2) + 
                      ",\"soil_moisture\":" + String(soilMoisture, 2) + 
                      ",\"raindrop\":" + String(raindrop, 2) + "}";

  // Send data via LoRa
  LoRa.beginPacket();
  LoRa.print(dataToSend);
  LoRa.endPacket();
  
  // Wait for a second before next reading
  delay(1000);
}
