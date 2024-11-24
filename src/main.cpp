#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>
#include <Wire.h>
#include "Dps310.h"
#include <OneWire.h>
#include <DallasTemperature.h>
#include "SHT2x.h"

#define I2C_SDA 4
#define I2C_SCL 5

//dont forget to create wifi_credentials.h!
#include "wifi_credentials.h"

const static uint16_t ALTITUDE = 263;
const static uint16_t PORT = 1242;

char buffer[96];

float_t temp = 0;
float_t press = 0;
float_t relPress = 0;
float_t voltage = 0;
float_t humidity = 0;

Dps310 Dps310PressureSensor = Dps310();
SHT2x sht;

float_t calculatePressureSeaLevel(float_t temperature, float_t pressure, uint16_t altitude);

void generateJson(char *out, double temp, double press, double relPress);
void initDS18B20();
void measBatteryVoltage();
void initWiFi();
void sendData();

WiFiClient client;
HTTPClient http;

OneWire oneWire(2);
DallasTemperature sensors(&oneWire);

void setup()
{
  // enable power to sh21
  pinMode(13, OUTPUT);
  digitalWrite(13, HIGH);

  // Get Battery Voltage before load is being applied to the battery
  measBatteryVoltage();

  // Set pin D0 to WAKEUP_PULLUP so the esp8266 can be awaked from deepsleep
  pinMode(D0, WAKEUP_PULLUP);

  // Initialize I2C Bus and DPS310
  Wire.begin(I2C_SDA, I2C_SCL);
  Dps310PressureSensor.begin(Wire);
  sht.begin();
  sht.read();

  initWiFi();
  initDS18B20();

  // Perform Measurements
  temp = sensors.getTempCByIndex(0);
  Dps310PressureSensor.measurePressureOnce(press, DPS__OVERSAMPLING_RATE_8);
  press /= 100;
  humidity = sht.getHumidity();

  // calculate relative Pressure based on Temperature and Altitude
  relPress = calculatePressureSeaLevel(temp, press, ALTITUDE);

  // send Data
  sendData();

  // disable power to sh21
  digitalWrite(13, LOW);
  pinMode(13, INPUT);

  // Go To DeepSleep for 60 Seconds, then reawake
  ESP.deepSleep(300e06, RF_NO_CAL);
}

/**
 * @brief Initialize the DS18B20 Temperature Sensor and request a measurement
 *
 */
void initDS18B20()
{
  sensors.begin();
  sensors.setResolution(11);
  sensors.requestTemperaturesByIndex(0);
}

void loop()
{
}

/**
 * @brief Generate the Json-String containing the measurements and the battery voltage
 *
 * @param out Char-Array in which Json-String is stored
 */
void generateJson(char *out)
{
  sprintf(out, "{\"temperature\":%.4f,\"pressure\":%.4f,\"relPressure\":%.4f,\"voltage\":%.4f,\"humidity\":%.4f}", temp, press, relPress, voltage, humidity);
}

/**
 * @brief Initialize the WiFi-Connection, go back to deepsleep if no connection possible
 *
 */
void initWiFi()
{
  WiFi.mode(WIFI_STA);
  WiFi.persistent(true);
  WiFi.setAutoConnect(false);
  WiFi.setAutoReconnect(true);
  WiFi.hostname("Wetterstation");
  if (WiFi.SSID() != WIFI_SSID || WiFi.psk() != WIFI_PASS)
  {
    WiFi.begin(WIFI_SSID, WIFI_PASS);
  }
  else
  {
    WiFi.begin();
  }
  uint8_t counter = 0;
  while (WiFi.status() != WL_CONNECTED)
  {
    delay(500);
    counter++;
    if (counter > 40)
    {
      ESP.deepSleep(60e06, RF_CAL);
    }
  }
}

/**
 * @brief Send data to Server
 *
 */
void sendData()
{
  client.setTimeout(5000);
  if (http.begin(client, "http://wetterserver.homeserver.lan/weather_data"))
  {
    memset(buffer, 0, sizeof(buffer));
    generateJson(buffer);
    http.POST(buffer);
  }
  http.end();
  client.flush();
  delay(10);
}

/**
 * @brief Calculates the relative air pressure
 *
 * @param temperature current temperature in degrees celsius
 * @param pressure current absolute pressure in hPa
 * @param altitude current altituted in m above sea level
 * @return float_t relative air pressure for the given params in hPa
 */
float_t calculatePressureSeaLevel(float_t temperature, float_t pressure, uint16_t altitude)
{
  float_t temp_kelvin = temperature + 273.15;
  return pressure * pow(temp_kelvin / (temp_kelvin + 0.0065 * altitude), -5.225);
}

/**
 * @brief measures the Battery voltage.
 * make shure that no expensive operations are running before or during the measurement because it could
 * falsefy the measurement
 *
 */
void measBatteryVoltage()
{
  voltage = analogRead(A0) * (5.25 / 1023.0);
}