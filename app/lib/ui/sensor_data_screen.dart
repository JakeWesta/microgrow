import 'dart:convert';

import 'package:fl_chart/fl_chart.dart';
import 'package:flutter/material.dart';
import 'package:intl/intl.dart';

import '../models/habitat_obj.dart';
import '../models/sensor_history_obj.dart';
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

  bool isLoadingHistory = false;
  List<Map<String, dynamic>> historyData = [];
  String selectedGraph = "Temperature";

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
        final dynamic decoded = jsonDecode(payload);

        final List<dynamic> items = decoded is List ? decoded : [decoded];

        setState(() {
          for (var item in items) {
            final historyEntry = SensorHistory(
              temp: (item['temp'] as num).toDouble(),
              humidity: (item['humidity'] as num).toDouble(),
              timestamp: DateTime.parse(item['timestamp']),
            );

            if (!widget.habitat.history.any(
              (e) => e.timestamp == historyEntry.timestamp,
            )) {
              widget.habitat.history.insert(0, historyEntry);
            }
          }
          widget.habitat.save();
          isLoadingHistory = false;
        });
      } catch (e) {
        print('History parse error: $e');
        setState(() {
          isLoadingHistory = false;
        });
      }
    },
  );
}

  double get reservoirPercentage {
    final max = widget.habitat.maxReservoirVolume;
    final current = widget.habitat.currentReservoirVolume;

    if (max <= 0) return 0;
    final percent = (current / max) * 100;
    return percent.clamp(0, 100);
  }

  Widget buildGraph() {
    final history = widget.habitat.history.reversed.toList();

    if (history.isEmpty) {
      return const Padding(
        padding: EdgeInsets.all(20),
        child: Text("No data available"),
      );
    }

    return Column(
      crossAxisAlignment: CrossAxisAlignment.start,
      children: [
        Center(
          child: Text(
            "$selectedGraph vs Time",
            style: const TextStyle(
              fontSize: 20,
              fontWeight: FontWeight.bold,
            ),
          ),
        ),
        const SizedBox(height: 10),
        LayoutBuilder(
          builder: (context, constraints) {
            return SizedBox(
              height: 360,
              child: Padding(
                padding: const EdgeInsets.symmetric(horizontal: 16),
                child: LineChart(
                  LineChartData(
                    minY: 0,
                    maxY: 100,
                    clipData: FlClipData.none(),
                    gridData: FlGridData(
                      show: true,
                      horizontalInterval: 20,
                    ),
                    borderData: FlBorderData(
                      show: true,
                      border: const Border(
                        left: BorderSide(width: 2),
                        bottom: BorderSide(width: 2),
                      ),
                    ),
                    titlesData: FlTitlesData(
                      topTitles: const AxisTitles(
                        sideTitles: SideTitles(showTitles: false),
                      ),
                      rightTitles: const AxisTitles(
                        sideTitles: SideTitles(showTitles: false),
                      ),
                      leftTitles: AxisTitles(
                        axisNameWidget: Padding(
                          padding: const EdgeInsets.only(bottom: 2),
                          child: Text(
                            selectedGraph == "Temperature"
                                ? "Temperature (F)"
                                : "Humidity (%)",
                            style: const TextStyle(
                              fontWeight: FontWeight.bold,
                              fontSize: 16,
                            ),
                          ),
                        ),
                        axisNameSize: 40,
                        sideTitles: const SideTitles(
                          showTitles: true,
                          reservedSize: 40,
                          interval: 20,
                        ),
                      ),
                      bottomTitles: AxisTitles(
                        axisNameWidget: const Padding(
                          padding: EdgeInsets.only(top: 12),
                          child: Text(
                            "Time Since Plant (Hours)",
                            style: TextStyle(
                              fontWeight: FontWeight.bold,
                              fontSize: 16,
                            ),
                          ),
                        ),
                        axisNameSize: 40,
                        sideTitles: SideTitles(
                          showTitles: true,
                          reservedSize: 35,
                          interval: history.length > 5
                              ? (history.length / 5).floorToDouble()
                              : 1,
                        ),
                      ),
                    ),
                    lineBarsData: [
                      LineChartBarData(
                        isCurved: true,
                        barWidth: 3,
                        color: selectedGraph == "Temperature"
                            ? Colors.red
                            : Colors.blue,
                        dotData: const FlDotData(show: false),
                        spots: history.asMap().entries.map((entry) {
                          final index = entry.key;
                          final item = entry.value;

                          final value = selectedGraph == "Temperature"
                              ? item.temp
                              : item.humidity;

                          return FlSpot(index.toDouble(), value);
                        }).toList(),
                      ),
                    ],
                  ),
                ),
              ),
            );
          },
        ),
      ],
    );
  }

  Widget sensorCard(String label, String? value) {
    String displayValue;

    if (value == null || value.isEmpty) {
      displayValue = 'No data';
    } else {
      final num? numValue = num.tryParse(value.trim());

      if (label == 'Light' || label == 'Humidity') {
        displayValue = numValue != null
            ? "${numValue.toStringAsFixed(0)} %"
            : "$value %";
      } else if (label == 'Temperature') {
        displayValue = numValue != null
            ? "${numValue.toStringAsFixed(0)} F"
            : "$value F";
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
            Text(
              label,
              style: const TextStyle(fontSize: 20, fontWeight: FontWeight.bold),
            ),
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

  Widget reservoirCard() {
    final percent = reservoirPercentage;

    return Card(
      elevation: 4,
      shape: RoundedRectangleBorder(borderRadius: BorderRadius.circular(12)),
      margin: const EdgeInsets.symmetric(vertical: 8),
      child: Padding(
        padding: const EdgeInsets.symmetric(horizontal: 16, vertical: 12),
        child: Column(
          crossAxisAlignment: CrossAxisAlignment.start,
          children: [
            Row(
              mainAxisAlignment: MainAxisAlignment.spaceBetween,
              children: [
                const Text(
                  'Water Reservoir',
                  style: TextStyle(fontSize: 20, fontWeight: FontWeight.bold),
                ),
                Text(
                  "${percent.toStringAsFixed(0)} %",
                  style: const TextStyle(
                    fontSize: 20,
                    fontWeight: FontWeight.bold,
                    color: Colors.green,
                  ),
                ),
              ],
            ),
            const SizedBox(height: 12),
            ClipRRect(
              borderRadius: BorderRadius.circular(8),
              child: LinearProgressIndicator(
                value: percent / 100,
                minHeight: 12,
                backgroundColor: Colors.green.shade100,
                valueColor: AlwaysStoppedAnimation<Color>(Colors.green.shade600),
              ),
            ),
            const SizedBox(height: 8),
            Text(
              "${widget.habitat.currentReservoirVolume.toStringAsFixed(1)} / ${widget.habitat.maxReservoirVolume.toStringAsFixed(1)} mL",
              style: const TextStyle(
                fontSize: 14,
                color: Colors.black54,
                fontWeight: FontWeight.w600,
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
            const Icon(
              Icons.eco,
              size: 32,
              color: Color.fromARGB(255, 134, 245, 153),
            ),
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
            crossAxisAlignment: CrossAxisAlignment.stretch,
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
              reservoirCard(),
              const SizedBox(height: 30),
              const Text(
                "Past Sensor Readings",
                style: TextStyle(
                  fontSize: 24,
                  fontWeight: FontWeight.bold,
                ),
              ),
              const SizedBox(height: 30),
              Row(
                mainAxisAlignment: MainAxisAlignment.center,
                children: [
                  ToggleButtons(
                    isSelected: [
                      selectedGraph == "Temperature",
                      selectedGraph == "Humidity",
                    ],
                    onPressed: (index) {
                      setState(() {
                        selectedGraph =
                            index == 0 ? "Temperature" : "Humidity";
                      });
                    },
                    children: const [
                      Padding(
                        padding: EdgeInsets.symmetric(horizontal: 16),
                        child: Text("Temperature"),
                      ),
                      Padding(
                        padding: EdgeInsets.symmetric(horizontal: 16),
                        child: Text("Humidity"),
                      ),
                    ],
                  ),
                ],
              ),
              const SizedBox(height: 20),
              buildGraph(),
              const SizedBox(height: 30),
              Center(
                child: ElevatedButton(
                  onPressed: isLoadingHistory ? null : fetchHistory,
                  style: ElevatedButton.styleFrom(
                    backgroundColor: const Color.fromARGB(255, 82, 175, 88),
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
                            color: Colors.white,
                          ),
                        ),
                ),
              ),
              const SizedBox(height: 20),
              Column(
                children: widget.habitat.history.map((entry) {
                  final formattedDate =
                      DateFormat('MMMM d, h:mm a').format(entry.timestamp);
                  return Card(
                    elevation: 3,
                    margin: const EdgeInsets.symmetric(vertical: 6),
                    child: ListTile(
                      title: Text(
                        formattedDate,
                        style: const TextStyle(fontWeight: FontWeight.bold),
                      ),
                      subtitle: Text(
                        "Temp: ${entry.temp.toStringAsFixed(1)} F | "
                        "Humidity: ${entry.humidity.toStringAsFixed(1)} %",
                      ),
                    ),
                  );
                }).toList(),
              )
            ],
          ),
        ),
      ),
    );
  }
}