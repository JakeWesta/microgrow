import 'package:flutter/material.dart';
import 'package:provider/provider.dart';
import '../models/app_state.dart';
import '../models/habitat_obj.dart';
import '../mqtt/mqtt_connect.dart';

class HabitatConfig {
  final String greenType;

  final int tempTarget;
  final int humidityTarget;

  final int lightStartSec;
  final int lightDurationSec;
  final int lightIntervalSec;

  final int waterStartSec;
  final int waterDurationSec;
  final int waterIntervalSec;

  final int blackoutDuration;

  const HabitatConfig({
    required this.greenType,
    required this.tempTarget,
    required this.humidityTarget,
    required this.lightStartSec,
    required this.lightDurationSec,
    required this.lightIntervalSec,
    required this.waterStartSec,
    required this.waterDurationSec,
    required this.waterIntervalSec,
    required this.blackoutDuration,
  });

  Map<String, dynamic> toJson() => {
    "greenType": greenType,
    "target": {
      "temp": tempTarget,
      "humidity": humidityTarget,
    },
    "light": {
      "startTimeSec": lightStartSec,
      "durationSec": lightDurationSec,
      "intervalSec": lightIntervalSec,
    },
    "water": {
      "startTimeSec": waterStartSec,
      "durationSec": waterDurationSec,
      "intervalSec": waterIntervalSec,
    },
    "blackout": blackoutDuration,
  };
}


class AddHabitatScreen extends StatefulWidget {
  const AddHabitatScreen({super.key});

  @override
  State<AddHabitatScreen> createState() => _AddHabitatScreenState();
}

class _AddHabitatScreenState extends State<AddHabitatScreen> {
  final formKey = GlobalKey<FormState>();
  final nameController = TextEditingController();
  final idController = TextEditingController();
  String? selectedGreen;

  final now = DateTime.now();

  @override
  Widget build(BuildContext context) {

    final now = DateTime.now();
    final secSinceMidnight = now.hour * 3600 + now.minute * 60 + now.second;

    final Map<String, HabitatConfig> greenOptions = {
      'Basil': HabitatConfig(
        greenType: 'Basil',
        tempTarget: 80,
        humidityTarget: 78,
        lightStartSec: secSinceMidnight,
        lightDurationSec: 1, 
        lightIntervalSec: 300, 
        waterStartSec: secSinceMidnight,
        waterDurationSec: 5, 
        waterIntervalSec: 15, 
        blackoutDuration: 0,
      ),
      'Broccoli': HabitatConfig(
        greenType: 'Broccoli',
        tempTarget: 80,
        humidityTarget: 28,
        lightStartSec: secSinceMidnight,
        lightDurationSec: 10, 
        lightIntervalSec: 15,
        waterStartSec: secSinceMidnight,
        waterDurationSec: 5, 
        waterIntervalSec: 15, 
        blackoutDuration: 10,
      ),
      'Kale': HabitatConfig(
        greenType: 'Kale',
        tempTarget: 80,
        humidityTarget: 28,
        lightStartSec: secSinceMidnight,
        lightDurationSec: 10, 
        lightIntervalSec: 15,
        waterStartSec: secSinceMidnight,
        waterDurationSec: 5, 
        waterIntervalSec: 15, 
        blackoutDuration: 10,
      ),
      'Sunflower': HabitatConfig(
        greenType: 'Sunflower',
        tempTarget: 80,
        humidityTarget: 28,
        lightStartSec: secSinceMidnight,
        lightDurationSec: 10, 
        lightIntervalSec: 15,
        waterStartSec: secSinceMidnight,
        waterDurationSec: 5, 
        waterIntervalSec: 15, 
        blackoutDuration: 10,
      ),
      'Arugula': HabitatConfig(
        greenType: 'Arugula',
        tempTarget: 80,
        humidityTarget: 28,
        lightStartSec: secSinceMidnight,
        lightDurationSec: 10, 
        lightIntervalSec: 15,
        waterStartSec: secSinceMidnight,
        waterDurationSec: 5, 
        waterIntervalSec: 15, 
        blackoutDuration: 10,
      ),
      'Radish': HabitatConfig(
        greenType: 'Radish',
        tempTarget: 80,
        humidityTarget: 28,
        lightStartSec: secSinceMidnight,
        lightDurationSec: 10, 
        lightIntervalSec: 15,
        waterStartSec: secSinceMidnight,
        waterDurationSec: 5, 
        waterIntervalSec: 15, 
        blackoutDuration: 10,
      ),
      'Pea': HabitatConfig(
        greenType: 'Pea',
        tempTarget: 80,
        humidityTarget: 28,
        lightStartSec: secSinceMidnight,
        lightDurationSec: 10, 
        lightIntervalSec: 15,
        waterStartSec: secSinceMidnight,
        waterDurationSec: 5, 
        waterIntervalSec: 15, 
        blackoutDuration: 10,
      ),
    };


    return Scaffold(
      appBar: AppBar(
        backgroundColor: Colors.green[700],
        title: Row(
          children: [
            Icon(Icons.eco, size: 32, color: const Color.fromARGB(255, 134, 245, 153)), // microgreen/leaf icon
            const SizedBox(width: 10),
            const Text(
              'Add a Habitat',
              style: TextStyle(
                fontSize: 28,
                fontWeight: FontWeight.bold,
                color: Colors.white,
              ),
            ),
          ],
        ),
        centerTitle: false,
      ),
      body: SafeArea(
        child: Padding(
          padding: const EdgeInsets.all(16),
          child: Form(
            key: formKey,
            child: Column(
              crossAxisAlignment: CrossAxisAlignment.start,
              children: [
                TextFormField(
                  controller: nameController,
                  decoration: const InputDecoration(
                    labelText: "Habitat Name",
                    border: OutlineInputBorder(),
                  ),
                  validator: (value) {
                    if (value == null || value.isEmpty) {
                      return "Please enter a name";
                    }
                    return null;
                  },
                ),
                const SizedBox(height: 20),
                DropdownButtonFormField<String>(
                  initialValue: selectedGreen,
                  items: greenOptions.keys.map((type) => DropdownMenuItem(
                            value: type,
                            child: Text(type),
                          )).toList(),
                  decoration: const InputDecoration(
                    labelText: "Select Microgreen Type",
                    border: OutlineInputBorder(),
                  ),
                  onChanged: (value) {
                    setState(() {
                      selectedGreen = value;
                    });
                  },
                  validator: (value) {
                    if (value == null) {
                      return "Please select a microgreen type";
                    }
                    return null;
                  },
                ),
                const SizedBox(height: 30),
                TextFormField(
                  controller: idController,
                  decoration: const InputDecoration(
                    labelText: "Habitat Unique ID",
                    border: OutlineInputBorder(),
                  ),
                  validator: (value) {
                    if (value == null || value.isEmpty) {
                      return "Please enter ID shown on the Habitat's screen";
                    }
                    return null;
                  },
                ),
                const SizedBox(height: 30),
                Center(
                  child: ElevatedButton(
                    style: ElevatedButton.styleFrom(
                      minimumSize: const Size(220, 80), 
                      padding: const EdgeInsets.all(16),
                      backgroundColor: Color.fromARGB(255, 82, 175, 88)
                    ),
                    onPressed: () async {
                      if (formKey.currentState!.validate()) {
                        final id = idController.text.trim();
                        final config = greenOptions[selectedGreen]!;

                        final newHabitat = Habitat(
                          id: idController.text.trim(),
                          name: nameController.text.trim(),
                          greenType: config.greenType,
                          tempTarget: config.tempTarget,
                          humidityTarget: config.humidityTarget,
                          lightStartSec: config.lightStartSec,
                          lightDurationSec: config.lightDurationSec,
                          lightIntervalSec: config.lightIntervalSec,
                          waterStartSec: config.waterStartSec,
                          waterDurationSec: config.waterDurationSec,
                          waterIntervalSec: config.waterIntervalSec,
                          blackoutDuration: config.blackoutDuration,
                        );

                        context.read<MyAppState>().addHabitat(newHabitat);
                        
                        try {
                          final config = greenOptions[selectedGreen]!;
                          await MqttService.setupHabitat(
                            habitatId: id,
                            config: config,
                          );
                          print("MQTT setupHabitat sent successfully");
                        } catch (e) {
                          print("MQTT setupHabitat failed: $e");
                        }

                        Navigator.pop(context);
                      }
                    },
                    child: const Text(
                      'Add a Habitat',
                      style: TextStyle(
                        fontSize: 22,
                        fontWeight: FontWeight.bold,
                        color:  Color.fromARGB(255, 255, 255, 255),
                        fontFamily: "Times"
                      ),
                    ),
                  ),
                ),
              ],
            ),
          ),
        ),
      ),
    );
  }
}
