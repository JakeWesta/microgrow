import 'package:flutter/material.dart';
import 'package:provider/provider.dart';
import '../models/app_state.dart';
import '../models/habitat_obj.dart';
import 'package:uuid/uuid.dart';
import '../mqtt/mqtt_connect.dart';
import 'package:intl/intl.dart';

class HabitatConfig {
  final String greenType;

  final int tempTarget;
  final int humidityTarget;

  final int lightStartMs;
  final int lightDurationMs;
  final int lightIntervalMs;

  final int waterStartMs;
  final int waterDurationMs;
  final int waterIntervalMs;

  const HabitatConfig({
    required this.greenType,
    required this.tempTarget,
    required this.humidityTarget,
    required this.lightStartMs,
    required this.lightDurationMs,
    required this.lightIntervalMs,
    required this.waterStartMs,
    required this.waterDurationMs,
    required this.waterIntervalMs,
  });

  Map<String, dynamic> toJson() => {
    "greenType": greenType,
    "target": {
      "temp": tempTarget,
      "humidity": humidityTarget,
    },
    "light": {
      "startTimeMs": lightStartMs,
      "durationMs": lightDurationMs,
      "intervalMs": lightIntervalMs,
    },
    "water": {
      "startTimeMs": waterStartMs,
      "durationMs": waterDurationMs,
      "intervalMs": waterIntervalMs,
    }
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
  String? selectedGreen;

  final now = DateTime.now();

  @override
  Widget build(BuildContext context) {

    final now = DateTime.now();
    final msSinceMidnight =
        now.hour * 3600000 +
        now.minute * 60000 +
        now.second * 1000 +
        now.millisecond;

    final Map<String, HabitatConfig> greenOptions = {
      'Basil': HabitatConfig(
        greenType: 'Basil',
        tempTarget: 24,
        humidityTarget: 60,
        lightStartMs: msSinceMidnight,
        lightDurationMs: 3600000,  
        lightIntervalMs: 86400000, 
        waterStartMs: msSinceMidnight,
        waterDurationMs: 30000,    
        waterIntervalMs: 28800000, 
      ),

      'Broccoli': HabitatConfig(
        greenType: 'Broccoli',
        tempTarget: 22,
        humidityTarget: 55,
        lightStartMs: msSinceMidnight,
        lightDurationMs: 5400000, 
        lightIntervalMs: 86400000,
        waterStartMs: msSinceMidnight,
        waterDurationMs: 45000, 
        waterIntervalMs: 21600000,
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
                Center(
                  child: ElevatedButton(
                    style: ElevatedButton.styleFrom(
                      backgroundColor: Colors.green[700],
                      padding: const EdgeInsets.all(20)
                    ),
                    onPressed: () async {
                      if (formKey.currentState!.validate()) {
                        final id = const Uuid().v4();
                        final config = greenOptions[selectedGreen]!;

                        final newHabitat = Habitat(
                          id: id,
                          name: nameController.text.trim(),
                          greenType: config.greenType,
                          tempTarget: config.tempTarget,
                          humidityTarget: config.humidityTarget,
                          lightStartMs: config.lightStartMs,
                          lightDurationMs: config.lightDurationMs,
                          lightIntervalMs: config.lightIntervalMs,
                          waterStartMs: config.waterStartMs,
                          waterDurationMs: config.waterDurationMs,
                          waterIntervalMs: config.waterIntervalMs,
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
                      "Save Habitat",
                      style: TextStyle(fontSize: 16, color: Colors.white),
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
