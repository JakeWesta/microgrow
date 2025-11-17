# Micro-Grow

## Prototype:
### Components
- ESP32/App two-way communication using MQTT protocol and EMQX broker
- Mobile application interface (using Flutter/Dart)
- Electrical components configured on breadboard
- Data storage managed with Hive database
- 3D-printed unit (CAD models)

## Implemented Features:
- **Real-Time Data Transmission** - ESP32 polls readings from temperature/humidity and water-level sensors and publishes the data every 5 seconds to the MQTT topics `microgrow/<habitatID>/temp, humidity, water, light`.
- **User Interface** - App dashboard displays all created habitats and has the option to add and remove them. Selecting a habitat will display the sensor data readings sent from the ESP32.
- **Schedule-Based Automation** - ESP32 pumps water from the water reservoir to the watering tray based on the watering schedule information sent during `microgrow/init`.

## Future Improvements:
- Include misting pump to regulate humidity within the habitat
- Create pixelated character sprites to display on the LCD screen
- Expand the mobile app to include a 'virtual world' for the users to interact with their pixel characters

## How to Use:

### Requirements:
- Flutter SDK
- Dart
- VS Code
- Arduino IDE
- Hive (for database)
- EMQX MQTT broker

### Setup:
1. Ensure the required software is installed and install the necessary dependencies
2. Launch the mobile application:
   - Open VS Code and clone the repository
   - Navigate to the app directory with `cd microgrow/app`
   - Fetch packages with `flutter pub get`
   - Connect to a 2.4 GHz WiFi network, such as a mobile hotspot
   - Start the application with `flutter run` and specify the launch platform
3. Flash the ESP32 firmware:
   - Open `main.ino` from the `ESP32 Firmware` directory in the Arduino IDE
   - Connect the ESP32 to a USB port on your computer
   - In Arduino IDE, select the correct configuration:
     - Board: ESP32 Dev Module
     - Port: auto-detected when plugging in
4. At the top of the code, assign the corresponding network information to the variables `ssid` and `password`
5. Press Upload and wait for flashing to complete

## Known Bugs:
- If sensor is unplugged, it displays garbage values. We don't handle that issue, as it displays more than 7 digits when disconnected from ESP32.
- If there is a disconnect from MQTT, we don't test for reconnection.
- Sometimes the ESP32 flashing will fail if its GPIO pins are connected to external peripherals/sensors. Remove the ESP32 from the breadboard before flashing to resolve this.
- Pump and fan will sometimes stop working during regular use.
