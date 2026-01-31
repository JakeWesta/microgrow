import 'package:app/ui/sensor_data_screen.dart';
import 'package:flutter/material.dart';
import 'package:provider/provider.dart';
import '../models/app_state.dart';
import 'add_habitat_screen.dart';
import '../models/habitat_obj.dart';
import 'manual_control_screen.dart';
import '../mqtt/mqtt_connect.dart';


void showDeleteConfirm(BuildContext context, Habitat habitat) {
  showDialog(
    context: context,
    builder: (context) => AlertDialog(
      title: const Text("Delete Habitat"),
      content: Text("Are you sure you want to delete '${habitat.name}'?"),
      actions: [
        TextButton(
          onPressed: () => Navigator.pop(context),
          child: const Text("Cancel"),
        ),
        TextButton(
          onPressed: () {
            final appState = context.read<MyAppState>();
            MqttService.deleteHabitat(
              habitatId: habitat.id
            );
            appState.deleteHabitat(habitat);
            Navigator.pop(context); 
          },
          child: const Text(
            "Delete",
            style: TextStyle(color: Colors.red),
          ),
        ),
      ],
    ),
  );
}
class HomeScreen extends StatelessWidget {
  const HomeScreen({super.key});

  @override
  Widget build(BuildContext context) {
    final appState = context.watch<MyAppState>();
    final habitats = appState.getHabitats;
    final showDot = appState.showHarvestNotification;


    return Scaffold(
      appBar: AppBar(
        backgroundColor: const Color.from(alpha: 1, red: 0.22, green: 0.557, blue: 0.235),
        title: Row(
          children: [
            Icon(Icons.eco, size: 32, color: const Color.fromARGB(255, 134, 245, 153)), // microgreen/leaf icon
            const SizedBox(width: 10),
            const Text(
              'Habitat Home',
              style: TextStyle(
                fontSize: 28,
                fontWeight: FontWeight.bold,
                color: Colors.white,
              ),
            ),
          ],
        ),
        centerTitle: false,
         actions: [
          Stack(
            children: [
              IconButton(
                icon: const Icon(Icons.notifications, size: 32, color: Colors.white),
                onPressed: () {
                  final ready = appState.harvestReadyHabitats;

                  if (ready.isEmpty) return;

                  showDialog(
                    context: context,
                    builder: (_) => AlertDialog(
                      title: const Text("Ready to Harvest"),
                      content: SizedBox(
                        width: double.maxFinite,
                        child: ListView(
                          shrinkWrap: true,
                          children: ready
                              .map(
                                (h) => Card(
                                  child: ListTile(
                                    leading: const Icon(Icons.eco, color: Colors.green),
                                    title: Text(h.name),
                                    subtitle: Text(h.greenType),
                                  ),
                                ),
                              )
                              .toList(),
                        ),
                      ),
                      actions: [
                        TextButton(
                          onPressed: () {
                            Navigator.pop(context);
                            appState.acknowledgeHarvestNotification(); 
                          },
                          child: const Text("OK"),
                        ),
                      ],
                    ),
                  );
                },
              ),

              if (showDot)
                Positioned(
                  right: 8,
                  top: 8,
                  child: Container(
                    width: 10,
                    height: 10,
                    decoration: const BoxDecoration(
                      color: Color.fromARGB(255, 82, 175, 88),
                      shape: BoxShape.circle,
                    ),
                  ),
                ),
            ],
          ),
        ],
      ),

      body: SafeArea(
        child: Padding(
          padding: const EdgeInsets.all(16.0),
          child: Column(
            children: [

              if (habitats.isEmpty) 
                const Center(child: Text(
                  'No current habitats formed.\n Press "Add a Habitat" to begin!',
                  textAlign: TextAlign.center,
                  style: TextStyle(fontSize: 32, color: Color.fromARGB(255, 82, 175, 88), fontFamily: "Times", fontWeight: FontWeight.bold)
                ),
              ), 

              if (habitats.isEmpty) 
                const SizedBox(height:30),

              ElevatedButton(
                style: ElevatedButton.styleFrom(
                  minimumSize: const Size(220, 80), 
                  padding: const EdgeInsets.all(16),
                  backgroundColor: Color.fromARGB(255, 82, 175, 88)
                ),
                onPressed: () {
                  Navigator.push(
                    context,
                    MaterialPageRoute(
                      builder: (_) => const AddHabitatScreen(),
                    ),
                  );
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

              const SizedBox(height:15),

              if (habitats.isNotEmpty)
                Expanded(
                  child: ListView.builder(
                    itemCount: habitats.length,
                    itemBuilder: (context, index) {
                      final habitat = habitats[index];
                      return Card(
                          child: ListTile(
                            title: Text(habitat.name),
                            subtitle: Text('Type: ${habitat.greenType}'),
                            trailing: Row(
                              mainAxisSize: MainAxisSize.min,
                              children: [
                                // MANUAL BUTTON
                                IconButton(
                                  icon: const Icon(Icons.play_arrow_sharp),
                                  onPressed: () {
                                    Navigator.push(
                                      context,
                                      MaterialPageRoute(
                                        builder: (_) => ManualControlScreen(habitat: habitat),
                                      ),
                                    );
                                  },
                                ),

                                // SENSOR BUTTON
                                IconButton(
                                  icon: const Icon(Icons.water_damage_outlined),
                                  onPressed: () {
                                    Navigator.push(
                                      context,
                                      MaterialPageRoute(
                                        builder: (_) => SensorDataScreen(habitat: habitat),
                                      ),
                                    );
                                  },
                                ),

                                // DELETE BUTTON
                                IconButton(
                                  icon: const Icon(Icons.delete),
                                  onPressed: () {
                                    showDeleteConfirm(context, habitat);
                                  },
                                ),
                              ],
                            ), 
                          ),
                        );
                    },
                  ),
                ),
            ],
          ),
        ),
      ),
    );
  }
}

          