import 'package:flutter/material.dart';
import '../models/app_state.dart';
import '../models/habitat_obj.dart';
import 'tomagachi_screen.dart';
import 'package:provider/provider.dart';

class GreenhouseScreen extends StatelessWidget {
  const GreenhouseScreen({super.key});

  @override
  Widget build(BuildContext context) {
    final habitats = context.watch<MyAppState>().getHabitats;

    return Scaffold(
      appBar: AppBar(
        backgroundColor: Colors.green[700],
        title: Row(
          children: [
            Icon(Icons.eco, size: 32, color: const Color.fromARGB(255, 134, 245, 153)), // microgreen/leaf icon
            const SizedBox(width: 10),
            Text(
              ('The Greenhouse'),
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
      body: Stack(
        children: [
          Container(
              color: Colors.lightBlue[300]
          ),

          Positioned(
            bottom: 0,
            left: 0,
            right: 0,
            height: 120,
            child: Container(color: Colors.brown[400]),
          ),

          Center(
            child: Container(
              width: 350,
              height: 250,
              decoration: BoxDecoration(
                color: Colors.green[100]?.withOpacity(0.5),
                border: Border.all(color: Colors.green[700]!, width: 4),
                borderRadius: BorderRadius.circular(12),
              ),
              child: Padding(
                padding: const EdgeInsets.all(12.0),
                child: GridView.builder(
                  physics: const NeverScrollableScrollPhysics(),
                  gridDelegate: const SliverGridDelegateWithFixedCrossAxisCount(
                    crossAxisCount: 3,
                    crossAxisSpacing: 8,
                    mainAxisSpacing: 8,
                  ),
                  itemCount: 6,
                  itemBuilder: (context, index) {
                    Habitat? habitat;
                    if (index < habitats.length) {
                      habitat = habitats[index];
                    }

                    return GestureDetector(
                      onTap: habitat != null
                          ? () {
                              Navigator.push(
                                context,
                                MaterialPageRoute(
                                  builder: (_) => TomagachiScreen(habitat: habitat!),
                                ),
                              );
                            }
                          : null,
                      child: Container(
                        decoration: BoxDecoration(
                          color: habitat != null ? Colors.green[400] : Colors.green[100],
                          border: Border.all(color: Colors.green[700]!, width: 2),
                          borderRadius: BorderRadius.circular(8),
                        ),
                        child: Center(
                          child: habitat != null
                              ? Column(
                                  mainAxisSize: MainAxisSize.min,
                                  children: [
                                    Icon(Icons.eco, color: Colors.white, size: 32),
                                    const SizedBox(height: 4),
                                    Text(
                                      habitat.name,
                                      style: const TextStyle(
                                          color: Colors.white, fontWeight: FontWeight.bold),
                                    ),
                                  ],
                                )
                              : const Icon(Icons.add, color: Colors.white54),
                        ),
                      ),
                    );
                  },
                ),
              ),
            ),
          ),
        ],
      ),
    );
  }
}