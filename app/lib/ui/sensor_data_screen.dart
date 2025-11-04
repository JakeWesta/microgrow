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
  String? sensorVal;

  @override
  void initState() {
    super.initState();

    MqttService.sensorSubscribe(
      habitatId: widget.habitat.id,
      onMessage: (payload) {
        final String? sensorValue = payload;
        if (sensorValue != null && mounted) {
          setState(() {
            sensorVal = sensorValue;
          });
        }
      },
    );
  }

  @override
  void dispose() {
    super.dispose();
  }

  @override
  Widget build(BuildContext context) {
    String displayText;

    if (sensorVal != null) {
      displayText = sensorVal.toString();
    } else {
      displayText = 'No data yet';
    }
    return Scaffold(
      appBar: AppBar(
        backgroundColor: Colors.green[700],
        title: Text('${widget.habitat.name} Sensor'),
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
                    'Current Sensor Value:',
                    style: TextStyle(fontSize: 20, fontWeight: FontWeight.bold),
                  ),
                  const SizedBox(height: 16),
                  Text(
                    displayText,
                    style: const TextStyle(fontSize: 36, color: Colors.green),
                  ),
                ],
              ),
            ),
          ),
        ),
      ),
    );
  }
}
