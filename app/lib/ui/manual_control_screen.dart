import 'package:flutter/material.dart';
import '../models/habitat_obj.dart';
import '../mqtt/mqtt_connect.dart';

class ManualControlScreen extends StatefulWidget {
  final Habitat habitat;

  const ManualControlScreen({super.key, required this.habitat});

  @override
  State<ManualControlScreen> createState() => _ManualControlScreenState();
}

class _ManualControlScreenState extends State<ManualControlScreen> {
  bool lightOn = false;
  bool fanOn = false;
  bool waterFlashing = false;

  Future<void> sendOverride(String actuator, int val) async {
    try {
      await MqttService.actuatorPublish(
        habitatId: widget.habitat.id,
        actuatorName: actuator,
        val: val,
      );
      if (actuator.toLowerCase() == 'water') {
        setState(() => waterFlashing = true);
        await Future.delayed(const Duration(milliseconds: 1000));
        if (mounted) setState(() => waterFlashing = false);
      }
    } catch (e) {
      ScaffoldMessenger.of(context).showSnackBar(
        SnackBar(content: Text('Failed to override $actuator: $e')),
      );
    }
  }

  Widget toggleCard(String label, bool value, void Function(bool) onChanged) {
    return Card(
      elevation: 4,
      shape: RoundedRectangleBorder(borderRadius: BorderRadius.circular(12)),
      margin: const EdgeInsets.symmetric(vertical: 8),
      child: Padding(
        padding: const EdgeInsets.symmetric(horizontal: 16, vertical: 12),
        child: Row(
          mainAxisAlignment: MainAxisAlignment.spaceBetween,
          children: [
            Text(label, style: const TextStyle(fontSize: 20, fontWeight: FontWeight.bold)),
            Switch(
              value: value,
              activeThumbColor : const Color.fromARGB(255, 81, 162, 85),
              onChanged: onChanged,
            ),
          ],
        ),
      ),
    );
  }

Widget waterCard() {
  return Card(
    elevation: 4,
    shape: RoundedRectangleBorder(borderRadius: BorderRadius.circular(12)),
    margin: const EdgeInsets.symmetric(vertical: 8),
    child: Padding(
      padding: const EdgeInsets.symmetric(horizontal: 16, vertical: 12),
      child: Row(
        mainAxisAlignment: MainAxisAlignment.spaceBetween,
        children: [
          const Text(
            'Water',
            style: TextStyle(fontSize: 20, fontWeight: FontWeight.bold),
          ),
          ElevatedButton(
            style: ElevatedButton.styleFrom(
              backgroundColor: waterFlashing ? const Color.fromARGB(255, 134, 233, 139) : const Color.fromARGB(255, 86, 171, 90),
            ),
            onPressed: () => sendOverride('water', 1),
            child: const Text(
              'Pulse',
              style: TextStyle(fontSize: 18, fontWeight: FontWeight.bold, color: Color.fromARGB(255, 255, 255, 255)),
            ),
          ),
        ],
      ),
    ),
  );
}


  @override
  Widget build(BuildContext context) {
    return Scaffold(
      appBar: AppBar(
        backgroundColor: Colors.green[700],
        title: Row(
          children: [
            Icon(Icons.eco, size: 32, color: const Color.fromARGB(255, 134, 245, 153)), // microgreen/leaf icon
            const SizedBox(width: 10),
            Text(
              ('${widget.habitat.name} Override'),
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
      body: Padding(
        padding: const EdgeInsets.all(16.0),
        child: Column(
        children: [
          const SizedBox(height: 16),
          waterCard(),
          toggleCard('Light', lightOn, (val) {
            setState(() => lightOn = val);
            sendOverride('light', val ? 1 : 0);
          }),
          toggleCard('Fan', fanOn, (val) {
            setState(() => fanOn = val);
            sendOverride('fan', val ? 1 : 0);
          }),
        ],
      ),
      ),
    );
  }
}
