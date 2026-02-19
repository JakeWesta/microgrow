import 'package:flutter/material.dart';
import '../models/habitat_obj.dart';
import '../mqtt/mqtt_connect.dart';
import 'dart:convert';


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
  bool isLoadingHistory = false;
  List<Map<String, dynamic>> historyData = [];

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

  Future<void> fetchHistory() async {
  setState(() {
    isLoadingHistory = true;
  });

  await MqttService.requestHistory(
    habitatId: widget.habitat.id,
    onMessage: (payload) {
      try {
        final List<dynamic> decoded = jsonDecode(payload);

        setState(() {
          for (var item in decoded) {
            historyData.insert(0, item); 
          }
          isLoadingHistory = false;
        });
      } catch (e) {
        setState(() {
          isLoadingHistory = false;
        });
      }
    },
  );
}


Widget sensorCard(String label, String? value) {
  String displayValue;

  if (value == null || value.isEmpty) {
    displayValue = 'No data';
  } else {
    final num? numValue = num.tryParse(value.trim());

    if (label == 'Light' || label == 'Humidity') {
      displayValue = numValue != null ? "${numValue.toStringAsFixed(0)} %" : "$value %";
    } else if (label == 'Temperature') {
      displayValue = numValue != null ? "${numValue.toStringAsFixed(0)} F" : "$value F";
    } else if (label == 'Water Level') {
      double? waterDouble = double.tryParse(value);
      int waterInt = waterDouble?.round() ?? 2;
      switch (waterInt) {
        case 0:
          displayValue = "All Good!";
          break;
        case 1:
          displayValue = "Too Low!";
          break;
        default:
          displayValue = "No data";
      }
    } else {
      displayValue = value;
    }
  }

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
            displayValue,
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
        child: SingleChildScrollView(
          child: Column(
            crossAxisAlignment: CrossAxisAlignment.start,
            children: [

              const Text(
                "Current Sensor Readings",
                style: TextStyle(
                  fontSize: 24,
                  fontWeight: FontWeight.bold,
                ),
              ),

              const SizedBox(height: 10),

              sensorCard('Light', light),
              sensorCard('Humidity', humidity),
              sensorCard('Temperature', temp),
              sensorCard('Water Level', water),

              const SizedBox(height: 30),

              const Text(
                "Past Sensor Readings",
                style: TextStyle(
                  fontSize: 24,
                  fontWeight: FontWeight.bold,
                ),
              ),

              const SizedBox(height: 30),

              Center(
                child: ElevatedButton(
                  onPressed: isLoadingHistory ? null : fetchHistory,
                  style: ElevatedButton.styleFrom(
                    backgroundColor: Color.fromARGB(255, 82, 175, 88),
                  ),
                  child: isLoadingHistory
                      ? const SizedBox(
                          height: 20,
                          width: 20,
                          child: CircularProgressIndicator(
                            color: Colors.white,
                            strokeWidth: 2,
                          ),
                        )
                      : const Text(
                  "Refresh",
                  style: TextStyle(
                    fontSize: 24,
                    fontWeight: FontWeight.bold,
                    color: Colors.white
                  ),
                ),
                ),
              ),

              const SizedBox(height: 20),

              Column(
                children: historyData.map((item) {
                  return Card(
                    elevation: 3,
                    margin: const EdgeInsets.symmetric(vertical: 6),
                    child: ListTile(
                      title: Text(
                        "${item['timestamp']}",
                        style: const TextStyle(fontWeight: FontWeight.bold),
                      ),
                      subtitle: Text(
                        "Temp: ${item['temp']} F | Humidity: ${item['humidity']} %",
                      ),
                    ),
                  );
                }).toList(),
              ),
            ],
          ),
        ),
      ),
    );
  }
}
