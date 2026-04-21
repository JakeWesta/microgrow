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
            top: 12,
            left: 0,
            right: 0,
            child: Center(
              child: GestureDetector(
                onTap: () => setState(() => editMode = !editMode),
                child: Container(
                  padding: const EdgeInsets.symmetric(horizontal: 14, vertical: 8),
                  decoration: BoxDecoration(
                    color: editMode ? Colors.green[600] : Colors.black26,
                    borderRadius: BorderRadius.circular(20),
                  ),
                  child: Row(
                    mainAxisSize: MainAxisSize.min,
                    children: [
                      Icon(
                        editMode ? Icons.check : Icons.edit,
                        color: Colors.white,
                        size: 18,
                      ),
                      const SizedBox(width: 4),
                      Text(
                        editMode ? 'Done' : 'Edit Positions',
                        style: const TextStyle(
                          color: Colors.white,
                          fontWeight: FontWeight.bold,
                        ),
                      ),
                    ],
                  ),
                ),
              ),
            ),
          ),

          Positioned(
            top: 210,
            left: 0,
            right: 0,
            child: Center(
              child: Container(
                width: 300,
                height: 202,
                decoration: BoxDecoration(
                  color: Colors.green[100],
                  border: Border.all(
                    color: editMode ? Colors.green[400]! : Colors.green[700]!,
                    width: editMode ? 2 : 3,
                  ),
                  borderRadius: BorderRadius.circular(12),
                ),
                child: Padding(
                  padding: const EdgeInsets.all(2.0),
                  child: GridView.builder(
                    physics: const NeverScrollableScrollPhysics(),
                    gridDelegate: const SliverGridDelegateWithFixedCrossAxisCount(
                      crossAxisCount: 3,
                      crossAxisSpacing: 2,
                      mainAxisSpacing: 2,
                      childAspectRatio: 1.0,
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
                                            width: 70,
                                            height: 70,
                                            decoration: BoxDecoration(
                                              color: Colors.green[500],
                                              borderRadius: BorderRadius.circular(8),
                                              boxShadow: const [
                                                BoxShadow(color: Colors.black38, blurRadius: 8),
                                              ],
                                            ),
                                            child: Column(
                                              mainAxisAlignment: MainAxisAlignment.center,
                                              children: [
                                                const Icon(Icons.eco, color: Colors.white, size: 22),
                                                const SizedBox(height: 2),
                                                Text(
                                                  habitat.name,
                                                  maxLines: 1,
                                                  style: const TextStyle(
                                                    color: Colors.white,
                                                    fontWeight: FontWeight.bold,
                                                    fontSize: 9,
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
                                          child: Icon(Icons.eco_outlined, color: Colors.green[300], size: 20),
                                        ),
                                      ),
                                      child: Center(
                                        child: Column(
                                          mainAxisSize: MainAxisSize.min,
                                          children: [
                                            const Icon(Icons.eco, color: Colors.white, size: 22),
                                            const SizedBox(height: 2),
                                            Text(
                                              habitat.name,
                                              maxLines: 1,
                                              style: const TextStyle(
                                                color: Colors.white,
                                                fontWeight: FontWeight.bold,
                                                fontSize: 9,
                                              ),
                                              textAlign: TextAlign.center,
                                              overflow: TextOverflow.ellipsis,
                                            ),
                                            const SizedBox(height: 2),
                                            const Icon(Icons.drag_indicator, color: Colors.white54, size: 12),
                                          ],
                                        ),
                                      ),
                                    )
                                  : Center(
                                      child: Icon(Icons.add, color: Colors.green[400], size: 22),
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
                                      const Icon(Icons.eco, color: Colors.white, size: 24),
                                      const SizedBox(height: 2),
                                      Text(
                                        habitat.name,
                                        maxLines: 1,
                                        style: const TextStyle(
                                          color: Colors.white,
                                          fontWeight: FontWeight.bold,
                                          fontSize: 10,
                                        ),
                                        textAlign: TextAlign.center,
                                        overflow: TextOverflow.ellipsis,
                                      ),
                                    ],
                                  )
                                : const Icon(Icons.add, color: Colors.white54, size: 22),
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