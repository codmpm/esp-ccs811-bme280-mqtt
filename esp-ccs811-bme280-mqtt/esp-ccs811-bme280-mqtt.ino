/****************************************************
  ESP8266 CSS811 compensated via BME280 with MQTT
  2018, Patrik Mayer - patrik.mayer@codm.de

  mqtt client from https://github.com/knolleary/pubsubclient/
  Arduino OTA from ESP8266 Arduino Core V2.4
  CCS811 from https://github.com/AKstudios/CCS811-library
  BME280 from https://github.com/sparkfun/SparkFun_CCS811_Arduino_Library
  Tasker from https://github.com/sticilface/Tasker

  Connect the sensors via I2C, (SDA -> GPIO4 / SCL -> GPIO5). Don't forget 
  the I2C Pull-Ups.
  Make sure you connect !WAKE of the CCS811 to GPIO12 on the ESP. Pin is
  configurable but needed.
  Serial is left open for debugging (115200).

  The following topics will be dropped to mqtt, all retained:

   <mqttTopicPrefix>status          online/offline (last will)
   <mqttTopicPrefix>ip              system ip
   <mqttTopicPrefix>temperature     temperature in °C
   <mqttTopicPrefix>humidity        relative humidity in %
   <mqttTopicPrefix>pressure        pressure in hPa
   <mqttTopicPrefix>altitude        altitude in m
   <mqttTopicPrefix>co2             CO2 concentration in ppm
   <mqttTopicPrefix>tvoc            total volatile compound in ppb

  If you want to read Farenheit/Feet change readTempC() to readTempF() and 
  readFloatAltitudeMeters() to readFloatAltitudeFeet()

  ArduinoOTA is enabled, the mqttClientName is used as OTA name, see config.

****************************************************/

#include <Tasker.h>
#include <ESP8266WiFi.h>
#include <ESP8266mDNS.h>
#include <WiFiUdp.h>
#include <ArduinoOTA.h>
#include <PubSubClient.h>

#include <Wire.h>

#include <CCS811.h>
#include <SparkFunBME280.h>


//--------- Configuration
// WiFi
const char* ssid = "<your-wifi-ssid>";
const char* password = "<your-wifi-key>";

// MQTT
const char* mqttServer = "<mqtt-broker-ip-or-host>";
const char* mqttUser = "<mqtt-user>";
const char* mqttPass = "<mqtt-password>";
const char* mqttClientName = "<mqtt-client-id>"; //will also be used hostname and OTA name
const char* mqttTopicPrefix = "<mqtt-topic-prefix>";

#define LED_BUILTIN 2 //ESP-12F has the builtin LED on GPIO2, comment for other boards

//CCS811
//#define ADDR      0x5A
#define ADDR      0x5B
#define WAKE_PIN  14

unsigned long measureDelay = 60000; //read every 60s

//------------------------

// internal vars
CCS811 ccs811;
BME280 myBME280;

long lastReconnectAttempt = 0; //For the non blocking mqtt reconnect (in millis)

WiFiClient espClient;
PubSubClient mqttClient(espClient);

Tasker tasker;

char mqttTopicStatus[64];
char mqttTopicIp[64];
char mqttTopicTemperatureC[64];
char mqttTopicHumidity[64];
char mqttTopicPressure[64];
char mqttTopicAltitude[64];
char mqttTopicCO2[64];
char mqttTopicTVOC[64];

float bme280TemperatureC;
float bme280Humidity;
float bme280Pressure;
float bme280Altitude;

int ccs811co2;
int ccs811tvoc;

void setup() {
  Serial.begin(115200);
  Serial.println("Starting up...");

  pinMode(LED_BUILTIN, OUTPUT);

  //put in mqtt prefix
  sprintf(mqttTopicStatus, "%sstatus", mqttTopicPrefix);
  sprintf(mqttTopicIp, "%sip", mqttTopicPrefix);
  sprintf(mqttTopicTemperatureC, "%stemperature", mqttTopicPrefix);
  sprintf(mqttTopicHumidity, "%shumidity", mqttTopicPrefix);
  sprintf(mqttTopicPressure, "%spressure", mqttTopicPrefix);
  sprintf(mqttTopicAltitude, "%saltitude", mqttTopicPrefix);
  sprintf(mqttTopicCO2, "%sco2", mqttTopicPrefix);
  sprintf(mqttTopicTVOC, "%stvoc", mqttTopicPrefix);

  
  if (!ccs811.begin(uint8_t(ADDR), uint8_t(WAKE_PIN))) {
    Serial.println("CCS811 Initialization failed.");
    while (1);
  }


  //Initialize BME280
  myBME280.settings.commInterface = I2C_MODE;
  myBME280.settings.I2CAddress = 0x77;
  myBME280.settings.runMode = 3; //Normal mode
  myBME280.settings.tStandby = 0;
  myBME280.settings.filter = 4;
  myBME280.settings.tempOverSample = 5;
  myBME280.settings.pressOverSample = 5;
  myBME280.settings.humidOverSample = 5;

  //Calling .begin() causes the settings to be loaded
  delay(10);  //Make sure sensor had enough time to turn on. BME280 requires 2ms to start up.

  if (!myBME280.begin()) {
    Serial.println("Could not find a valid BME280 sensor, check wiring!");
    while (1);
  }

  setup_wifi();
  mqttClient.setServer(mqttServer, 1883);

  //----------- OTA
  ArduinoOTA.setHostname(mqttClientName);

  ArduinoOTA.onStart([]() {
    String type;
    if (ArduinoOTA.getCommand() == U_FLASH) {
      type = "sketch";
    } else { // U_SPIFFS
      type = "filesystem";
    }
    // NOTE: if updating SPIFFS this would be the place to unmount SPIFFS using SPIFFS.end()
    Serial.println("Start updating " + type);
  });

  ArduinoOTA.onEnd([]() {
    Serial.println("\nEnd");
    delay(1000);
    ESP.restart();
  });

  ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
    Serial.printf("Progress: %u%%\r", (progress / (total / 100)));
  });

  ArduinoOTA.onError([](ota_error_t error) {
    Serial.printf("Error[%u]: ", error);
    if (error == OTA_AUTH_ERROR) Serial.println("Auth Failed");
    else if (error == OTA_BEGIN_ERROR) Serial.println("Begin Failed");
    else if (error == OTA_CONNECT_ERROR) Serial.println("Connect Failed");
    else if (error == OTA_RECEIVE_ERROR) Serial.println("Receive Failed");
    else if (error == OTA_END_ERROR) Serial.println("End Failed");
  });
  ArduinoOTA.begin();



  Serial.println("ready...");
  digitalWrite(LED_BUILTIN, LOW);
  delay(500);
  //ready, blink the led twice
  for (uint8_t i = 0; i < 5; i++ ) {
    digitalWrite(LED_BUILTIN, !digitalRead(LED_BUILTIN));
    delay(200);
  }

  //measure every <measureDelay>ms - GO!
  tasker.setInterval(meassureEnvironment, measureDelay);

}

void loop() {

  //handle mqtt connection, non-blocking
  if (!mqttClient.connected()) {
    long now = millis();
    if (now - lastReconnectAttempt > 5000) {
      lastReconnectAttempt = now;
      // Attempt to reconnect
      if (MqttReconnect()) {
        lastReconnectAttempt = 0;
      }
    }
  }

  mqttClient.loop();
  tasker.loop();

  //handle OTA
  ArduinoOTA.handle();


}

void meassureEnvironment(int) {
  bme280TemperatureC = myBME280.readTempC();;
  bme280Humidity = myBME280.readFloatHumidity();
  bme280Pressure = myBME280.readFloatPressure();
  bme280Altitude = myBME280.readFloatAltitudeMeters();

  Serial.print("Temperature = ");
  Serial.print(bme280TemperatureC);
  Serial.println(" *C");

  Serial.print("Pressure = ");
  Serial.print(bme280Pressure);
  Serial.println(" hPa");

  Serial.print("Approx. Altitude = ");
  Serial.print(bme280Altitude);
  Serial.println(" m");

  Serial.print("Humidity = ");
  Serial.print(bme280Humidity);
  Serial.println(" %");

  ccs811.compensate(bme280TemperatureC, bme280Humidity);  // replace with t and rh values from sensor
  ccs811.getData();

  ccs811co2 = ccs811.readCO2();
  ccs811tvoc = ccs811.readTVOC();

  Serial.print("CO2 concentration : ");
  Serial.print(ccs811co2);
  Serial.println(" ppm");

  Serial.print("TVOC concentration : ");
  Serial.print(ccs811tvoc);
  Serial.println(" ppb");


  char buf[8] = "";
  sprintf(buf, "%f", bme280TemperatureC);
  mqttClient.publish(mqttTopicTemperatureC, buf, true);

  sprintf(buf, "%f", bme280Humidity);
  mqttClient.publish(mqttTopicHumidity, buf, true);

  sprintf(buf, "%f", bme280Pressure);
  mqttClient.publish(mqttTopicPressure, buf, true);

  sprintf(buf, "%f", bme280Altitude);
  mqttClient.publish(mqttTopicAltitude, buf, true);

  sprintf(buf, "%d", ccs811co2);
  mqttClient.publish(mqttTopicCO2, buf, true);

  sprintf(buf, "%d", ccs811tvoc);
  mqttClient.publish(mqttTopicTVOC, buf, true);

  Serial.println();
}


void setup_wifi() {

  delay(10);

  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(ssid);

  WiFi.mode(WIFI_STA); //disable AP mode, only station
  WiFi.hostname(mqttClientName);
  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
}


bool MqttReconnect() {

  if (!mqttClient.connected()) {

    Serial.print("Attempting MQTT connection...");

    // Attempt to connect with last will retained
    if (mqttClient.connect(mqttClientName, mqttUser, mqttPass, mqttTopicStatus, 1, true, "offline")) {

      Serial.println("connected");

      // Once connected, publish an announcement...
      char curIp[16];
      sprintf(curIp, "%d.%d.%d.%d", WiFi.localIP()[0], WiFi.localIP()[1], WiFi.localIP()[2], WiFi.localIP()[3]);

      mqttClient.publish(mqttTopicStatus, "online", true);
      mqttClient.publish(mqttTopicIp, curIp, true);

    } else {
      Serial.print("failed, rc=");
      Serial.print(mqttClient.state());
      Serial.println(" try again in 5 seconds");
    }
  }
  return mqttClient.connected();
}


