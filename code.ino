

// Libraries
#include <WiFi.h>
#include "ThingSpeak.h"
#include "DHT.h"
#include <LiquidCrystal_I2C.h>

int buzzerPin = 27; ///< The pin (D14) connected to the positive pin of buzzer

const int lcdColumns = 16; ///< Number of columns in the I2C Display
const int lcdRows = 2; ///< Number of rows in the I2C Display
/**
 * An object 'lcd' of the 'LiquidCrystal_I2C' class is initialised with 0x27, 'lcdColumns' and 'lcdRows' as parameters
 */
LiquidCrystal_I2C lcd(0x27, lcdColumns, lcdRows);

const char* ssid = "Islington-College";       ///< SSID of wireless network that the ESP32 connects to
const char* password = "I$LiNGT0N2023"; ///< Password of wireless network that the ESP32 connects to
WiFiServer server(80);
WiFiClient client;

const char *apiKey = "hidden"; ///< Write API key of ThingSpeak channel
unsigned long channelID = 3;
const char* myWriteAPIKey = "hidden";

#define DHTPIN  4    ///< The pin (D15) connected to signal pin of DHT11 sensor
#define DHTTYPE DHT11 ///< The model of DHT sensor used is DHT11
/**
 * An object 'dht' of the 'DHT' class is initialised with 'DHTPIN' and 'DHTTYPE' as parameters
 */
DHT dht(DHTPIN, DHTTYPE);

const int trigPin = 14; ///< The pin (D5) connected to Trig pin of HC-SR04 sensor 
const int echoPin = 33; ///< The pin (D18) connected to Echo pin of HC-SR04 sensor
long duration; ///< Duration between transmitting and recieving the ultrasonic wave
float distanceCm; ///< Distance between object and HC-SR04 sensor in Centimetre
float distanceInch; ///< Distance between object and HC-SR04 sensor in Inch

#define SOUND_SPEED 0.034 ///< Speed of sound in air in metre per microsecond
#define CM_TO_INCH 0.393701 ///< Conversion factor from centimetre to inch
int soilMoisture; ///< Percentage of moisture in the soil
int sensorAnalog; ///< Analog value returned by the Hygrometer module
const int moistureAnalogPin = 35; ///< The pin (D34) connected to analog pin of Hygrometer module
/**
 * The standard setup function used for setup and configuration tasks
 */
void setup()
{
  Serial.begin(9600); // The serial monitor is set at 9600 baud rate
  Serial.println("Serial monitor initiated");
  dht.begin();
  lcd.init();
  lcd.backlight();
  pinMode(trigPin, OUTPUT);
  pinMode(echoPin, INPUT);

  pinMode(buzzerPin, OUTPUT);

  WiFi.begin(ssid, password);
  Serial.println("\nConnecting");
  while (WiFi.status() != WL_CONNECTED)
  {
    Serial.print(".");
    delay(1000);
  }
  Serial.println("\nConnected to the WiFi network");
  ThingSpeak.begin(client);
}
/**
 * The standard loop function used for repeating tasks *
 */
void loop()
{
  digitalWrite(buzzerPin, LOW);
  float humidity = dht.readHumidity();
  float temperatureCelsius = dht.readTemperature();
  float temperatureFahrenheit = dht.readTemperature(true); // the parameter passed is isFahrenheit=true
  if (isnan(humidity) || isnan(temperatureCelsius) || isnan(temperatureFahrenheit))
  {
    Serial.println(F("Failed to read from the DHT11 sensor")); // The F() macro helps save the SRAM memory by moving the constant string from SRAM to Flash Memory
    return;
  }
  float heatIndexFahrenheit = dht.computeHeatIndex(temperatureFahrenheit, humidity);
  float heatIndexCelsius = dht.computeHeatIndex(temperatureCelsius, humidity, false); // third parameter is isFahrenheit=false

  digitalWrite(trigPin, LOW);
  delayMicroseconds(2);
  digitalWrite(trigPin, HIGH);
  delayMicroseconds(10);
  digitalWrite(trigPin, LOW);

  duration = pulseIn(echoPin, HIGH);
  distanceCm = duration * SOUND_SPEED / 2;
  distanceInch = distanceCm * CM_TO_INCH;

  sensorAnalog = analogRead(moistureAnalogPin);
  soilMoisture = (100 - ((sensorAnalog / (float)4095) * 100));

  if (temperatureCelsius > 50 || temperatureCelsius < 0){
    digitalWrite(buzzerPin, HIGH);
  }
  if (distanceInch < 3){
    digitalWrite(buzzerPin, HIGH);
  }
  if (soilMoisture < 30 || soilMoisture > 80){
    digitalWrite(buzzerPin, HIGH);    
  }

  Serial.print(F("Humidity: "));
  Serial.print(humidity);
  Serial.print(F("%  Temperature: "));
  Serial.print(temperatureCelsius);
  Serial.print(F("°C "));
  Serial.print(temperatureFahrenheit);
  Serial.print(F("°F  Heat index: "));
  Serial.print(heatIndexCelsius);
  Serial.print(F("°C "));
  Serial.print(heatIndexFahrenheit);
  Serial.println(F("°F"));

  Serial.print("Distance (cm): ");
  Serial.println(distanceCm);
  Serial.print("Distance (inch): ");
  Serial.println(distanceInch);

  Serial.print("Moisture = ");
  Serial.print(soilMoisture);
  Serial.println("%");

  ThingSpeak.setField(1, temperatureCelsius);
  ThingSpeak.setField(2, humidity);
  ThingSpeak.setField(3, temperatureFahrenheit);
  ThingSpeak.setField(4, soilMoisture);
  int statusCode = ThingSpeak.writeFields(channelID, myWriteAPIKey);
  if (statusCode == 200)
  {
    Serial.println("Sucessfully wrote to channel");
  }
  else
  {
    Serial.println("Problem reading channel. HTTP error code " + String(statusCode));
  }
  lcd.clear();
  lcd.setCursor(0,0);
  lcd.print("Temp:");
  lcd.print(temperatureCelsius);
  lcd.print((char)223);
  lcd.print("C");
  lcd.setCursor(0,1);
  lcd.print("RH:");
  lcd.print((int)humidity);
  lcd.print("%");

  delay(15000);
}
