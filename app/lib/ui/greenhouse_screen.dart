import 'package:flutter/material.dart';
import '../models/app_state.dart';
import '../models/habitat_obj.dart';
import 'tomagachi_screen.dart';
import 'package:provider/provider.dart';

class GreenhouseScreen extends StatefulWidget {
  const GreenhouseScreen({super.key});

  @override
  State<GreenhouseScreen> createState() => _GreenhouseScreenState();
}

class _GreenhouseScreenState extends State<GreenhouseScreen> {
  bool editMode = false;

  Habitat? habitatForSlot(List<Habitat> habitats, int slot) {
    try {
      return habitats.firstWhere((h) => h.slotIndex == slot);
    } catch (_) {
      return null;
    }
  }

  Habitat? draggingHabitat;

  int displayToSlot(int displayIndex) {
    const map = [3, 4, 5, 0, 1, 2];
    return map[displayIndex];
  }

  @override
  Widget build(BuildContext context) {
    final appState = context.watch<MyAppState>();
    final habitats = appState.getHabitats;

    return Scaffold(
      appBar: AppBar(
        backgroundColor: Colors.green[700],
        title: Row(
          children: [
            const Icon(Icons.eco, size: 32, color: Color.fromARGB(255, 134, 245, 153)),
            const SizedBox(width: 10),
            const Text(
              'The Greenhouse',
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
          Padding(
            padding: const EdgeInsets.only(right: 12),
            child: editMode
                ? GestureDetector(
                    onTap: () => setState(() => editMode = false),
                    child: Container(
                      padding: const EdgeInsets.symmetric(horizontal: 14, vertical: 6),
                      decoration: BoxDecoration(
                        color: Colors.green[600],
                        borderRadius: BorderRadius.circular(20),
                      ),
                      child: const Row(
                        children: [
                          Icon(Icons.check, color: Colors.white, size: 18),
                          SizedBox(width: 4),
                          Text('Done', style: TextStyle(color: Colors.white, fontWeight: FontWeight.bold)),
                        ],
                      ),
                    ),
                  )
                : GestureDetector(
                    onTap: () => setState(() => editMode = true),
                    child: Container(
                      padding: const EdgeInsets.symmetric(horizontal: 14, vertical: 6),
                      decoration: BoxDecoration(
                        color: Colors.black26,
                        borderRadius: BorderRadius.circular(20),
                      ),
                      child: const Row(
                        children: [
                          Icon(Icons.edit, color: Colors.white, size: 18),
                          SizedBox(width: 4),
                          Text('Edit Positions', style: TextStyle(color: Colors.white, fontWeight: FontWeight.bold)),
                        ],
                      ),
                    ),
                  ),
          ),
        ],
      ),
      body: Stack(
        children: [
          Positioned.fill(
            child: Image.asset(
              'assets/sprites/Greenhouse_Background.png',
              fit: BoxFit.cover,
            ),
          ),

          Positioned(
            top: 270,
            left: 0,
            right: 0,
            child: Center(
              child: Container(
                width: 400,
                height: 280,
                decoration: BoxDecoration(
                  color: Colors.green[100],
                  border: Border.all(
                    color: editMode ? Colors.green[400]! : Colors.green[700]!,
                    width: editMode ? 2 : 4,
                  ),
                  borderRadius: BorderRadius.circular(12),
                ),
                child: Padding(
                  padding: const EdgeInsets.all(12.0),
                  child: GridView.builder(
                    physics: const NeverScrollableScrollPhysics(),
                    gridDelegate: const SliverGridDelegateWithFixedCrossAxisCount(
                      crossAxisCount: 3,
                      crossAxisSpacing: 4,
                      mainAxisSpacing: 4,
                    ),
                    itemCount: 6,
                    itemBuilder: (context, slotIndex) {
                      final mappedSlot = displayToSlot(slotIndex);
                      final habitat = habitatForSlot(habitats, mappedSlot);

                      if (editMode) {
                        return DragTarget<Habitat>(
                          onWillAcceptWithDetails: (details) => true,
                          onAcceptWithDetails: (details) async {
                            final incoming = details.data;
                            final existing = habitatForSlot(habitats, mappedSlot);
                              if (existing != null && existing.id != incoming.id) {
                                existing.slotIndex = incoming.slotIndex;
                                await existing.save();
                              }
                              incoming.slotIndex = mappedSlot;
                            await incoming.save();
                            setState(() {});
                          },
                          builder: (context, candidateData, rejectedData) {
                            final isHighlighted = candidateData.isNotEmpty;

                            return Container(
                              decoration: BoxDecoration(
                                color: isHighlighted
                                    ? Colors.green[300]
                                    : habitat != null
                                        ? Colors.green[400]
                                        : Colors.green[200],
                                border: Border.all(
                                  color: isHighlighted ? Colors.green[700]! : Colors.green[600]!,
                                  width: 2,
                                ),
                                borderRadius: BorderRadius.circular(8),
                              ),
                              child: habitat != null
                                  ? Draggable<Habitat>(
                                      data: habitat,
                                      feedback: Material(
                                        color: Colors.transparent,
                                        child: Opacity(
                                          opacity: 0.85,
                                          child: Container(
                                            width: 90,
                                            height: 90,
                                            decoration: BoxDecoration(
                                              color: Colors.green[500],
                                              borderRadius: BorderRadius.circular(8),
                                              boxShadow: [
                                                BoxShadow(color: Colors.black38, blurRadius: 8),
                                              ],
                                            ),
                                            child: Column(
                                              mainAxisAlignment: MainAxisAlignment.center,
                                              children: [
                                                const Icon(Icons.eco, color: Colors.white, size: 28),
                                                const SizedBox(height: 4),
                                                Text(
                                                  habitat.name,
                                                  style: const TextStyle(
                                                    color: Colors.white,
                                                    fontWeight: FontWeight.bold,
                                                    fontSize: 11,
                                                  ),
                                                  textAlign: TextAlign.center,
                                                  overflow: TextOverflow.ellipsis,
                                                ),
                                              ],
                                            ),
                                          ),
                                        ),
                                      ),
                                      childWhenDragging: Container(
                                        decoration: BoxDecoration(
                                          color: Colors.green[100],
                                          border: Border.all(color: Colors.green[300]!, width: 2),
                                          borderRadius: BorderRadius.circular(8),
                                        ),
                                        child: Center(
                                          child: Icon(Icons.eco_outlined, color: Colors.green[300], size: 28),
                                        ),
                                      ),
                                      child: Center(
                                        child: Column(
                                          mainAxisSize: MainAxisSize.min,
                                          children: [
                                            const Icon(Icons.eco, color: Colors.white, size: 28),
                                            const SizedBox(height: 4),
                                            Text(
                                              habitat.name,
                                              style: const TextStyle(
                                                color: Colors.white,
                                                fontWeight: FontWeight.bold,
                                                fontSize: 11,
                                              ),
                                              textAlign: TextAlign.center,
                                              overflow: TextOverflow.ellipsis,
                                            ),
                                            const SizedBox(height: 4),
                                            Icon(Icons.drag_indicator, color: Colors.white54, size: 16),
                                          ],
                                        ),
                                      ),
                                    )
                                  : Center(
                                      child: Icon(Icons.add, color: Colors.green[400], size: 28),
                                    ),
                            );
                          },
                        );
                      }

                      return GestureDetector(
                        onTap: habitat != null
                            ? () {
                                Navigator.push(
                                  context,
                                  MaterialPageRoute(
                                    builder: (_) => TomagachiScreen(habitat: habitat),
                                  ),
                                );
                              }
                            : null,
                        child: Container(
                          decoration: BoxDecoration(
                            color: habitat != null ? Colors.green[400] : Colors.green[200],
                            border: Border.all(color: Colors.green[700]!, width: 2),
                            borderRadius: BorderRadius.circular(8),
                          ),
                          child: Center(
                            child: habitat != null
                                ? Column(
                                    mainAxisSize: MainAxisSize.min,
                                    children: [
                                      const Icon(Icons.eco, color: Colors.white, size: 32),
                                      const SizedBox(height: 4),
                                      Text(
                                        habitat.name,
                                        style: const TextStyle(
                                          color: Colors.white,
                                          fontWeight: FontWeight.bold,
                                        ),
                                        textAlign: TextAlign.center,
                                        overflow: TextOverflow.ellipsis,
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
          ),
        ],
      ),
    );
  }
}
