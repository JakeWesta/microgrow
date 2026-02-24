#pragma once

#include <Arduino.h>

// WiFi & Network
#define WIFI_TIMEOUT_MS 10000
#define WIFI_PORTAL_TIMEOUT_S 180
#define WIFI_CONNECT_RETRIES 3

// MQTT
#define MQTT_BROKER "broker.emqx.io"
#define MQTT_PORT 1883
#define MQTT_TIMEOUT_MS 10000
#define MQTT_RECONNECT_DELAY_MS 5000
#define MQTT_KEEPALIVE_S 15
#define MQTT_SOCKET_TIMEOUT_S 5
#define MQTT_PUBLISH_INTERVAL_MS 3000

// Task Configuration
#define SENSOR_READ_INTERVAL_MS 2000
#define ACTUATOR_UPDATE_INTERVAL_MS 200
#define SCHEDULE_CHECK_INTERVAL_MS 200

// Timezone (EST)
#define TIMEZONE_OFFSET_S (-5 * 3600)
#define DST_OFFSET_S 0

// NTP Servers
#define NTP_SERVER_1 "pool.ntp.org"
#define NTP_SERVER_2 "time.nist.gov"

// Display
#define TFT_WIDTH 320
#define TFT_HEIGHT 240
#define TFT_ROTATION 1
#define LED_STRIP_BRIGHTNESS 50

// Manual Override
#define MANUAL_OVERRIDE_TIMEOUT_MS 300000 // 5 minutes, 0 = no timeout

// Environmental Control Thresholds
#define HUMIDITY_HIGH_THRESHOLD 70.0f
#define HUMIDITY_MED_THRESHOLD 60.0f
#define TEMP_HIGH_THRESHOLD 85.0f

// Fan PWM levels
#define FAN_PWM_OFF 0
#define FAN_PWM_LOW 128
#define FAN_PWM_HIGH 255

// Debug
#define SERIAL_BAUD 115200

// Historical Data
#define READING_SAVE_INTERVAL_S 3