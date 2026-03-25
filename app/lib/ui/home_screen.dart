import 'package:app/ui/sensor_data_screen.dart';
import 'package:flutter/material.dart';
import 'package:provider/provider.dart';
import '../models/app_state.dart';
import 'add_habitat_screen.dart';
import '../models/habitat_obj.dart';
import 'manual_control_screen.dart';
import '../mqtt/mqtt_connect.dart';
import '../models/growth_config.dart';


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
    final showReservoirDot = appState.showReservoirNotification;
    final showBlackoutDot = appState.showBlackoutNotification;



    return Scaffold(
      appBar: AppBar(
        backgroundColor: Colors.green[700],
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
                  final readyHarvest = appState.harvestReadyHabitats;
                  final lowWaterHabitats = appState.habitats
                      .where((h) => appState.reservoirNotified[h.id] == true)
                      .toList();
                  
                  final blackoutHabitats = appState.habitats
                      .where((h) => appState.blackoutNotified[h.id] == true && 
                                    appState.blackoutAcknowledged[h.id] != true);

                  showDialog(
                    context: context,
                    builder: (_) => AlertDialog(
                      title: const Text("Notifications"),
                      content: SizedBox(
                        width: double.maxFinite,
                        child: ListView(
                          shrinkWrap: true,
                          children: [
                            ...readyHarvest.map((h) => ListTile(
                                  leading: const Icon(Icons.eco, color: Colors.green),
                                  title: Text("${h.name} is ready to harvest!"),
                                  trailing: const Icon(Icons.check_circle_outline),
                                  onTap: () {
                                    appState.harvestHabitat(h);
                                    Navigator.pop(context);
                                  },
                                )),
                            ...lowWaterHabitats.map((h) => ListTile(
                                  leading: const Icon(Icons.water_drop, color: Colors.blue),
                                  title: Text("${h.name} reservoir is low!"),
                                  trailing: const Icon(Icons.warning_amber),
                                  onTap: () {
                                    appState.acknowledgeReservoirNotification();
                                    Navigator.pop(context);
                                  },
                                )),
                            ...blackoutHabitats.map((h) => ListTile(
                                  leading: const Icon(Icons.wb_sunny, color: Colors.orange),
                                  title: Text("${h.name} blackout period is complete!"),
                                  subtitle: const Text("Ready to move to light exposure"),
                                  trailing: const Icon(Icons.check_circle_outline),
                                  onTap: () {
                                    appState.acknowledgeBlackoutNotification(h.id);
                                    Navigator.pop(context);
                                  },
                                )),
                          ],
                        ),
                      ),
                      actions: [
                        TextButton(
                          onPressed: () {
                            appState.acknowledgeReservoirNotification();
                            appState.acknowledgeHarvestNotification();
                            appState.acknowledgeBlackoutNotification();
                            Navigator.pop(context);
                          },
                          child: const Text("OK"),
                        ),
                      ],
                    ),
                  );
                },
              ),
              if (showDot || showReservoirDot || showBlackoutDot)
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
                  backgroundColor: Colors.green[700]
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

                      final progress = getHabitatProgress(habitat); 
                      final stage = getGrowthStage(habitat);

                      Color progressColor;
                      switch (stage) {
                        case GrowthStage.seed:
                          progressColor = Colors.green[200]!;
                          break;
                        case GrowthStage.sapling:
                          progressColor = Colors.green[400]!;
                          break;
                        case GrowthStage.mature:
                          progressColor = Colors.green[600]!;
                          break;
                        case GrowthStage.ready:
                          progressColor = Colors.green[800]!;
                          break;
                      }

                      final inBlackout = habitat.blackoutDuration > 0 &&
                      DateTime.now().difference(habitat.createdAt).inSeconds < habitat.blackoutDuration;

                      return Padding(
                        padding: const EdgeInsets.symmetric(vertical: 8.0),
                        child: TweenAnimationBuilder<double>(
                          tween: Tween<double>(begin: 0, end: progress),
                          duration: const Duration(milliseconds: 800),
                          curve: Curves.easeInOut,
                          builder: (context, animatedProgress, child) {
                            return Card(
                              elevation: 4,
                              shape: RoundedRectangleBorder(
                                  borderRadius: BorderRadius.circular(12)),
                              child: Stack(
                                children: [
                                  Positioned.fill(
                                    child: ClipRRect(
                                      borderRadius: BorderRadius.circular(12),
                                      child: inBlackout
                                      ? LinearProgressIndicator(
                                          value: animatedProgress,
                                          backgroundColor: Colors.grey[900],
                                          valueColor: AlwaysStoppedAnimation<Color>(Colors.grey[800]!),
                                          minHeight: double.infinity,
                                        )
                                      : LinearProgressIndicator(
                                          value: animatedProgress,
                                          backgroundColor: const Color.fromARGB(255, 186, 222, 174),
                                          valueColor: AlwaysStoppedAnimation<Color>(progressColor),
                                          minHeight: double.infinity,
                                        ),
                                    ),
                                  ),
                                  ListTile(
                                    title: Text(
                                      habitat.name,
                                      style: const TextStyle(
                                        fontWeight: FontWeight.bold,
                                        color:  Color.fromRGBO(255, 255, 255, 1),
                                      ),
                                    ),
                                    subtitle: Text(
                                      'Type: ${habitat.greenType}',
                                      style: const TextStyle(color:  Color.fromARGB(255, 255, 255, 255)),
                                    ),
                                    trailing: Row(
                                      mainAxisSize: MainAxisSize.min,
                                      children: [
                                        // MANUAL BUTTON
                                        IconButton(
                                          icon: const Icon(Icons.play_arrow_sharp,
                                              color:   Color.fromARGB(255, 85, 192, 102)) ,
                                          onPressed: () {
                                            Navigator.push(
                                              context,
                                              MaterialPageRoute(
                                                builder: (_) =>
                                                    ManualControlScreen(habitat: habitat),
                                              ),
                                            );
                                          },
                                        ),
                                        // SENSOR BUTTON
                                        IconButton(
                                          icon: const Icon(Icons.water_damage_outlined,
                                              color: Color.fromARGB(255, 85, 192, 102)),
                                          onPressed: () {
                                            Navigator.push(
                                              context,
                                              MaterialPageRoute(
                                                builder: (_) =>
                                                    SensorDataScreen(habitat: habitat),
                                              ),
                                            );
                                          },
                                        ),
                                        // DELETE BUTTON
                                        IconButton(
                                          icon: const Icon(Icons.delete, color:  Color.fromARGB(255, 85, 192, 102)),
                                          onPressed: () {
                                            showDeleteConfirm(context, habitat);
                                          },
                                        ),
                                      ],
                                    ),
                                  ),
                                ],
                              ),
                            );
                          },
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

          