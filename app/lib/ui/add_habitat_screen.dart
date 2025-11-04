import 'package:flutter/material.dart';
import 'package:provider/provider.dart';
import '../models/app_state.dart';
import '../models/habitat_obj.dart';
import 'package:uuid/uuid.dart';
import '../mqtt/mqtt_connect.dart';


class AddHabitatScreen extends StatefulWidget {
  const AddHabitatScreen({super.key});

  @override
  State<AddHabitatScreen> createState() => _AddHabitatScreenState();
}

class _AddHabitatScreenState extends State<AddHabitatScreen> {
  final formKey = GlobalKey<FormState>();
  final nameController = TextEditingController();
  String? selectedGreen;

  final Map<String, int> greenOptions = {'Basil': 8, 'Broccoli': 10};

  @override
  Widget build(BuildContext context) {
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
                        final newHabitat = Habitat(
                          id: id,
                          name: nameController.text.trim(),
                          greenType: selectedGreen!,
                          waterSchedule: greenOptions[selectedGreen]
                        );

                        context.read<MyAppState>().addHabitat(newHabitat);
                        
                        try {
                          await MqttService.setupHabitat(
                            habitatId: id,
                            greenType: newHabitat.greenType,
                            schedule: greenOptions[selectedGreen]!, 
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
