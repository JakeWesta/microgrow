import 'package:flutter/material.dart';
import '../models/habitat_obj.dart';
import '../mqtt/mqtt_connect.dart';

class SensorDataScreen extends StatefulWidget {
  final Habitat habitat;

  const SensorDataScreen({super.key, required this.habitat});

  @override
  State<SensorDataScreen> createState() => _SensorDataScreenState();
}

class _SensorDataScreenState extends State<SensorDataScreen> {
  String? light;
  String? humidity;
  String? temp;
  String? water;

  @override
  void initState() {
    super.initState();

    MqttService.sensorSubscribe(
      habitatId: widget.habitat.id,
      onMessage: (topic, payload) {
        if (!mounted) return;

        setState(() {
          if (topic.endsWith('/light')) {
            light = payload;
          } else if (topic.endsWith('/humidity')) {
            humidity = payload;
          } else if (topic.endsWith('/temp')) {
            temp = payload;
          } else if (topic.endsWith('/water')) {
            water = payload;
          }
        });
      },
    );
  }

  Widget sensorCard(String label, String? value) {
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
            Text(
              value ?? 'No data',
              style: const TextStyle(
                fontSize: 20,
                fontWeight: FontWeight.bold,
                color: Colors.green,
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
            Icon(Icons.eco, size: 32, color: const Color.fromARGB(255, 134, 245, 153)),
            const SizedBox(width: 10),
            Text(
              '${widget.habitat.name} Sensors',
              style: const TextStyle(
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
            sensorCard('Light', light),
            sensorCard('Humidity', humidity),
            sensorCard('Temperature', temp),
            sensorCard('Water Level', water),
          ],
        ),
      ),
    );
  }
}
