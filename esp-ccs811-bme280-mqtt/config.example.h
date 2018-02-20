/* config file for esp ccs bme280 mqtt project */

/*
 * Main Settings
 * 
 * measureDelay: defines how often data will be pulled from the sensors (in ms, means 60000 = 60s)
 */
const unsigned long measureDelay = 60000;

/*
 * These are the current Wifi Settings
 * 
 * ssid: wifi name
 * password: wifi access key
 * 
 * hostname and OTA name are defined by mqttClientName below
 */
const char* ssid = "<your-wifi-ssid>";
const char* password = "<your-wifi-key>";

/*
 * Settings for the MQTT Broker, which shall be used 
 * 
 * mqttServer: IP or Hostname of mqttBroker
 * mqttUser: username on MQTT server
 * mqttPass: users password
 * mqttClientName: clientId on MQTT broker
 * mqttTopicPrefix: prefix for your topic (e.g. status will be shown in mqttTopicPrefix + "/status")
 */
const char* mqttServer = "<mqtt-broker-ip-or-host>";
const char* mqttUser = "<mqtt-user>";
const char* mqttPass = "<mqtt-password>";
const char* mqttClientName = "<mqtt-client-id>"; //will also be used hostname and OTA name
const char* mqttTopicPrefix = "<mqtt-topic-prefix>";

/*
 * GPIO Settings
 * 
 * LED_BUILTIN: Digial Pin Number of your boards built-in LED or 
 *               the LED you wish to show status signals
 */
#define LED_BUILTIN 2 //ESP-12F has the builtin LED on GPIO2, comment for other boards

/*
 * CCS811
 * 
 * WAKE_PIN: Digial Pin Number, where your BME280s wake signal is connected
 */
#define WAKE_PIN 15

