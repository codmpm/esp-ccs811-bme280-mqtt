# ESP8266 MQTT CCS811/BME280 air quality sensor

This is a simple sketch to measure the air quality with the [CCS811 Air Quality Sensor](http://ams.com/eng/Products/Environmental-Sensors/Air-Quality-Sensors/CCS811). It get's compensated with an [Bosch BME280](https://www.bosch-sensortec.com/bst/products/all_products/bme280).

I'm using the Adafruit CCS811 library which worked after I used the correct I2C address of 0x5A for my sensor.

__Currently I'm testing if my setup works and the values are reliable - so please see this as a work in progress. Any Feedback and help is appreciated!__

---

![image of breadboard ](esp-ccs811-bme280-breadboard.jpg)
Used here are the [Sparkfun CCS811](https://www.sparkfun.com/products/14193) and the [Adaferuit BME280](https://www.adafruit.com/product/2652) breakout.

## Config
See the config section in the code, should be self-explaining :-)

The read data get's dropped to the following topics using `<mqtt-topic-prefix>`. All values are retained, the online state is set to `offline` using mqtt's last-will.

```
<mqttTopicPrefix>status          online/offline (last will)
<mqttTopicPrefix>ip              system ip
<mqttTopicPrefix>temperature     temperature in °C
<mqttTopicPrefix>humidity        relative humidity in %
<mqttTopicPrefix>pressure        pressure in hPa
<mqttTopicPrefix>altitude        altitude in m
<mqttTopicPrefix>co2             CO2 concentration in ppm
<mqttTopicPrefix>tvoc            total volatile compound in ppb
```

## Todo
- Check Sparkfuns CCS811 lib 
- better configuration

## Created with
- Arduino 1.8.1 (https://www.arduino.cc/)
- ESP8266 board definition 2.4.0 (https://github.com/esp8266/Arduino)
- Tasker (https://github.com/sticilface/Tasker)
- PubSubClient 2.6.0 by Nick O'Leary (https://github.com/knolleary/pubsubclient)
- Adafruit CCS811 Library (https://github.com/adafruit/Adafruit_CCS811)
- Sparkfun BME280 Library (https://github.com/sparkfun/SparkFun_CCS811_Arduino_Library)

### Credits
Patrik Mayer, 2018 

### License
MIT