# Micro-Grow

- Flutter app demo video: https://www.youtube.com/watch?v=zIg6AqKC-sg

## Implemented Features:
### Pre-Alpha Build:
- **MQTT Connection** - Set up two-way communication between the ESP32 and the mobile app by using the MQTT protocol and EMQX broker.
- **Data Management** - Handle the storing of data by using a Hive database, which stores the watering cycle for each microgreen type.
- **Real-Time Sensor Data** - Configured ESP32 to poll readings from DHT11 temperature sensor and publishes the data every 5 seconds to the MQTT topic `microgrow/<habitatID>/sensor`.
- **User Interface** - Developed a Flutter app using Dart to allow the user to initialize and manage their MicroGrow habitat. Clicking on the habitat will display the real-time sensor data from the DHT11 on the app interface.

### Design Prototype:
- **Expanded Sensor Data** - Updated ESP32 code to publish sensor data for temperature/humidity, water level, and light intensity to MQTT topics `microgrow/<habitatID>/temp, humidity, water, light`. Sensor data is visible on the LCD screen to allow the user to easily monitor environmental conditions without their mobile device.
- **Expanded User Interface** - Updated dashboard to displays multiple created habitats and implemented the option to add and remove them. Selecting a habitat will display the real-time sensor data readings sent from the ESP32 for temperature, humidity, water level, and light intensity. Implemented the 'fun area' for the user to interact with their plant character
- **Schedule-Based Automation** - ESP32 pumps water from the water reservoir to the watering tray based on the watering schedule information sent during `microgrow/init`.
- **Manual Override** - Added a method for the user to control the water pump, fans, and light intensity manually from within the app. This feature does not interfere with the schedule timing.
- **Hardware** - Determined pin assignments for each electrical component. Connected ESP32, sensors, pump, lights, and LCD screen on a breadboard to test the functionality of each system. Designed CAD models to 3D-print the chassis and parts for the MicroGrow unit.

## Future Improvements:
- Include misting pump to regulate humidity within the habitat
- Create pixelated character sprites to display on the LCD screen
- Expand the 'fun area' in the mobile app to include customization features and scripted interaction between characters

## How to Use:

### Requirements:
- Flutter SDK
- Dart
- VS Code
- Arduino IDE
- Hive (for database)
- EMQX MQTT broker

### To setup the mobile application:
1. Ensure the required software is installed and install the necessary dependencies
2. Launch the mobile application:
   - Open VS Code and clone the repository
   - If prompted on VS Code, install the Flutter SDK and add to path if missing.
   - Install Flutter and Dart VS Code exstentions
   - Navigate to the app directory with `cd microgrow/app`
   - Fetch packages with `flutter pub get`
   - Connect to a 2.4 GHz WiFi network, such as a mobile hotspot
   - Start the application with `flutter run -d windows`
3. Flash the ESP32 firmware:
   - Open `main.ino` from the `ESP32 Firmware` directory in the Arduino IDE
   - Connect the ESP32 to a USB port on your computer
   - Follow the tutorial for the IDE to recognize the board: https://randomnerdtutorials.com/installing-the-esp32-board-in-arduino-ide-windows-instructions/
   - In Arduino IDE, select the correct configuration:
     - Board: ESP32 Dev Module
     - Port: auto-detected when plugging in
4. You may need to install the following packages. Easiest way is to go to the library manager tab on the left and search:
   - **PubSubClient** by Nick O'Leary
   - **DHT20** by Rob Tilaart
   - **ArduinoJson** by Benoit Blanchon
   - **Adafruit GFX Library** by Adafruit
   - **Adafruit ST7735 and ST7789 Library** by Adafruit
   - **FastLED** by Daniel Garcia
6. At the top of the code, assign the corresponding network information to the variables `ssid` and `password`
7. Press Upload and wait for flashing to complete

## Known Bugs:
- If sensor is unplugged, it displays garbage values. We don't handle that issue, as it displays more than 7 digits when disconnected from ESP32.
- If there is a disconnect from MQTT, we don't test for reconnection.
- Sometimes the ESP32 flashing will fail if its GPIO pins are connected to external peripherals/sensors. Remove the ESP32 from the breadboard before flashing to resolve this.
- Pump and fan will sometimes stop working during regular use.
- The current setup for the init sequence sends a JSON that is too large for the ESP32 to parse. This means that light scheduling is currently not being sent, as it had to be trimmed for the JSON to fit.
