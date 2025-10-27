# Micro-Grow repo.

## PROTOTYPE (Current):
1. ESP32 Development Board
2. DHT11 Temperature/Humidity Module
3. EMQX as MQTT Broker
4. Flutter Application (using Dart)
5. Hive Database (Temporary)

## FEATURES:
1. Real-time environmental data
2. Sends data over WiFi hotspot
3. Utilizes database to store information
4. Displays data per habitat on Flutter

## BUGs:
1. If sensor is unplugged, it displays garbage values. We don't handle that issue, as it displays more than 7 digits when disconnected from ESP32
2. If there is a disconnect from MQTT, we don't test for reconnection
3. Currently hardcoded, so it is limited to 1 habitat at a time and will override if more are introduced

## Future Improvements:
1. Use the DHT22 instead of the DHT11 for more accurate temperature readings
2. More stable WiFi hotspot connection for ESP32
3. Use more reliable database to store multiple habitats without fear of losing data
4. Begin connecting more sensors to the ESP32 to test functionality of those as well
