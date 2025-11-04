import 'package:app/ui/sensor_data_screen.dart';
import 'package:flutter/material.dart';
import 'package:provider/provider.dart';
import '../models/app_state.dart';
import 'add_habitat_screen.dart';
import 'package:hive/hive.dart';


class HomeScreen extends StatelessWidget {
  const HomeScreen({super.key});

  @override
  Widget build(BuildContext context) {
    final appState = context.watch<MyAppState>();
    final habitats = appState.getHabitats;

    return Scaffold(
      appBar: AppBar(
        backgroundColor: Colors.green[700],
        title: Row(
          children: [
            Icon(Icons.eco, size: 32, color: const Color.fromARGB(255, 134, 245, 153)), // microgreen/leaf icon
            const SizedBox(width: 10),
            const Text(
              'Micro-Grow',
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
          padding: const EdgeInsets.all(16.0),
          child: Column(
            children: [
              ElevatedButton(
                onPressed: () {
                  Navigator.push(
                    context,
                    MaterialPageRoute(
                      builder: (_) => const AddHabitatScreen(),
                    ),
                  );
                },
                child: const Text('Add a Habitat'),
              ),
              const SizedBox(height:15),

              if (habitats.isEmpty) 
                const Center(child: Text(
                  'No current habitats formed.\n Press "Add a Habitat" to begin!',
                  textAlign: TextAlign.center,
                  style: TextStyle(fontSize: 16, color: Color.fromARGB(255, 134, 245, 153))
                ),
              ),

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
                            trailing: IconButton(
                              icon: const Icon(Icons.water_damage_outlined),
                              onPressed: () {
                                Navigator.push(
                                  context,
                                  MaterialPageRoute(
                                    builder: (_) => SensorDataScreen(habitat: habitat)
                                  ),
                                );
                              },
                            ),
                          ),
                        );
                    },
                  ),
                ),

            ElevatedButton(
                onPressed: () async {
                  await Hive.deleteFromDisk();
                },
                child: const Text("DEBUG CLEAR"), 
              ),
            ],
          ),
        ),
      ),
    );
  }
}

          