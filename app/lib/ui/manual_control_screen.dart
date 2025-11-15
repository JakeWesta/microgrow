import 'package:flutter/material.dart';
import '../models/habitat_obj.dart';
import '../mqtt/mqtt_connect.dart';

class ManualControlScreen extends StatefulWidget {
  final Habitat habitat;

  const ManualControlScreen({super.key, required this.habitat});

  @override
  State<ManualControlScreen> createState() => _ManualControlScreen();
}

class _ManualControlScreen extends State<ManualControlScreen> {
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

  @override
  void dispose() {
    super.dispose();
  }

  @override
  Widget build(BuildContext context) {
    return Scaffold(
      appBar: AppBar(
        backgroundColor: Colors.green[700],
        title: Text('${widget.habitat.name} Sensors'),
      ),
      body: SafeArea(
        child: Center(
          child: Card(
            child: Padding(
              padding: const EdgeInsets.all(24.0),
              child: Column(
                mainAxisSize: MainAxisSize.min,
                children: [
                  const Text(
                    'Sensor Readings',
                    style: TextStyle(fontSize: 24, fontWeight: FontWeight.bold),
                  ),
                  const SizedBox(height: 24),

                  sensorTile('Light', light),
                  sensorTile('Humidity', humidity),
                  sensorTile('Temperature', temp),
                  sensorTile('Water Level', water),
                ],
              ),
            ),
          ),
        ),
      ),
    );
  }

  Widget sensorTile(String label, String? value) {
    return Padding(
      padding: const EdgeInsets.symmetric(vertical: 8.0),
      child: Row(
        mainAxisAlignment: MainAxisAlignment.spaceBetween,
        children: [
          Text(label, style: const TextStyle(fontSize: 18)),
          Text(
            value ?? 'No data',
            style: const TextStyle(
              fontSize: 22,
              fontWeight: FontWeight.bold,
              color: Colors.green,
            ),
          ),
        ],
      ),
    );
  }
}
