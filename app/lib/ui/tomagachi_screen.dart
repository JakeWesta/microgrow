import 'dart:async';
import 'dart:math';
import 'package:flutter/material.dart';
import '../models/habitat_obj.dart';
import '../models/decoration_obj.dart';
import '../models/database.dart';
import '../models/growth_config.dart'; 
import '../mqtt/mqtt_connect.dart';

class TomagachiScreen extends StatefulWidget {
  final Habitat habitat;

  const TomagachiScreen({super.key, required this.habitat});

  @override
  State<TomagachiScreen> createState() => _TomagachiScreenState();
}

class _TomagachiScreenState extends State<TomagachiScreen>
    with SingleTickerProviderStateMixin {
  late AnimationController controller;
  final Random random = Random();

  GrowthStage currentStage = GrowthStage.seed;
  Timer? spriteTimer;
  Timer? moveTimer;

  double plantX = 0;
  double plantY = 0;
  double plantTargetX = 0;
  double plantTargetY = 0;
  double plantScale = 1.0;
  double plantTargetScale = 1.0;
  double screenWidth = 300.0;

  final double plantWidth = 160;
  final double plantHeight = 160;

  bool editMode = false;
  bool isDraggingOverTrash = false;

  final GlobalKey trashKey = GlobalKey();

  @override
  void initState() {
    super.initState();
    controller = AnimationController(
      vsync: this,
      duration: const Duration(milliseconds: 800),
    )..repeat(reverse: true);

    plantX = 100 + random.nextDouble() * 150;
    plantY = 0;
    plantTargetX = plantX;
    plantTargetY = plantY;

    currentStage = getGrowthStage(widget.habitat);

    _scheduleNextMove();

    spriteTimer = Timer.periodic(const Duration(seconds: 1), (_) {
      setState(() {
        final newStage = getGrowthStage(widget.habitat);
        if (newStage != currentStage) {
          currentStage = newStage;
          MqttService.publishGrowthStage(
            habitatId: widget.habitat.id,
            stage: currentStage,
          );
        }
      });
    });
  }

  void _scheduleNextMove() {
    final pauseDuration = Duration(milliseconds: 800 + random.nextInt(1200));
    moveTimer = Timer(pauseDuration, () {
      if (!mounted) return;
      setState(() {
        final dx = (random.nextDouble() * 240 - 120);
        plantTargetX = (plantX + dx).clamp(20.0, screenWidth - plantWidth);

        final dy = (random.nextDouble() * 80 - 40);
        plantTargetY = (plantY + dy).clamp(-50.0, 50.0);

        plantTargetScale = 1.0 + (plantTargetY / 50) * 0.07;
      });
      _scheduleNextMove();
    });
  }

  @override
  void dispose() {
    moveTimer?.cancel();
    spriteTimer?.cancel();
    controller.dispose();
    super.dispose();
  }

  String spriteAsset(Habitat habitat, GrowthStage stage) {
    final type = habitat.greenType.toLowerCase();
    String stageName;
    switch (stage) {
      case GrowthStage.seed:
        stageName = 'seed';
        break;
      case GrowthStage.sapling:
        stageName = 'sapling';
        break;
      case GrowthStage.mature:
      case GrowthStage.ready:
        stageName = 'grown';
        break;
    }
    return 'assets/sprites/$type-$stageName.png';
  }

  void buyDecoration(String name, int cost) {
    final user = Database.user;
    if (user.coins >= cost) {
      user.coins -= cost;
      user.save();

      final decoration = DecorationObj(type: name, x: 150, y: 150);
      widget.habitat.decorations.add(decoration);
      widget.habitat.save();
      setState(() {
        editMode = true;
      });
    }
  }

  void updateDecorationPosition(int index, Offset position) {
    final deco = widget.habitat.decorations[index];
    deco.x = position.dx;
    deco.y = position.dy;
    widget.habitat.save();
    setState(() {});
  }

  bool isOverTrash(Offset globalPosition) {
    final RenderBox? box = trashKey.currentContext?.findRenderObject() as RenderBox?;
    if (box == null) return false;
    final trashPos = box.localToGlobal(Offset.zero);
    final trashRect = Rect.fromLTWH(trashPos.dx, trashPos.dy, box.size.width, box.size.height);
    return trashRect.contains(globalPosition);
  }

  Widget shopItem({
    required String assetPath,
    required String name,
    required int cost,
    required String type,
  }) {
    final user = Database.user;
    final canAfford = user.coins >= cost;

    return Container(
      margin: const EdgeInsets.symmetric(horizontal: 16, vertical: 8),
      decoration: BoxDecoration(
        color: canAfford ? Colors.green[50] : Colors.grey[100],
        borderRadius: BorderRadius.circular(16),
        border: Border.all(
          color: canAfford ? Colors.green[400]! : Colors.grey[300]!,
          width: 1.5,
        ),
        boxShadow: canAfford
            ? [BoxShadow(color: Colors.green[200]!.withOpacity(0.5), blurRadius: 8, offset: const Offset(0, 4))]
            : [],
      ),
      child: Material(
        color: Colors.transparent,
        child: InkWell(
          borderRadius: BorderRadius.circular(16),
          onTap: canAfford
              ? () {
                  buyDecoration(type, cost);
                  Navigator.pop(context);
                }
              : null,
          child: Padding(
            padding: const EdgeInsets.all(12),
            child: Row(
              children: [
                Container(
                  width: 56,
                  height: 56,
                  decoration: BoxDecoration(
                    color: Colors.green[100],
                    borderRadius: BorderRadius.circular(12),
                    border: Border.all(color: Colors.green[300]!, width: 1),
                  ),
                  padding: const EdgeInsets.all(6),
                  child: Image.asset(assetPath, fit: BoxFit.contain),
                ),
                const SizedBox(width: 14),
                Expanded(
                  child: Column(
                    crossAxisAlignment: CrossAxisAlignment.start,
                    children: [
                      Text(
                        name,
                        style: TextStyle(
                          color: canAfford ? Colors.green[900] : Colors.grey[600],
                          fontSize: 15,
                          fontWeight: FontWeight.bold,
                          letterSpacing: 0.3,
                        ),
                      ),
                      const SizedBox(height: 4),
                      Row(
                        children: [
                          const Icon(Icons.monetization_on, color: Colors.amber, size: 16),
                          const SizedBox(width: 4),
                          Text(
                            '$cost coins',
                            style: TextStyle(
                              color: canAfford ? Colors.green[300] : Colors.grey[400],
                              fontSize: 13,
                              fontWeight: FontWeight.w600,
                            ),
                          ),
                        ],
                      ),
                    ],
                  ),
                ),
                Icon(
                  canAfford ? Icons.add_circle : Icons.lock,
                  color: canAfford ? Colors.green[500] : Colors.grey[400],
                  size: 28,
                ),
              ],
            ),
          ),
        ),
      ),
    );
  }

  @override
  Widget build(BuildContext context) {
    screenWidth = MediaQuery.of(context).size.width;

    return Scaffold(
      appBar: AppBar(
        backgroundColor: Colors.green[700],
        title: Row(
          children: [
            const Icon(Icons.eco, size: 32, color: Color.fromARGB(255, 134, 245, 153)),
            const SizedBox(width: 10),
            Text(
              "${widget.habitat.name}'s House",
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
      endDrawer: Drawer(
        backgroundColor: Colors.white,
        child: SafeArea(
          child: Column(
            crossAxisAlignment: CrossAxisAlignment.start,
            children: [
              Container(
                width: double.infinity,
                padding: const EdgeInsets.fromLTRB(20, 24, 20, 20),
                decoration: BoxDecoration(
                  color: Colors.green[700],
                  border: Border(
                    bottom: BorderSide(color: Colors.green[300]!, width: 1.5),
                  ),
                ),
                child: const Row(
                  children: [
                    Icon(Icons.storefront, color: Color.fromARGB(255, 134, 245, 153), size: 28),
                    SizedBox(width: 10),
                    Text(
                      'Shop',
                      style: TextStyle(
                        color: Colors.white,
                        fontSize: 26,
                        fontWeight: FontWeight.bold,
                        letterSpacing: 1.2,
                      ),
                    ),
                  ],
                ),
              ),

              const SizedBox(height: 16),

              shopItem(
                assetPath: 'assets/sprites/MicroGrow_Sign.png',
                name: 'MicroGrow Sign',
                cost: 50,
                type: 'sign1',
              ),
              shopItem(
                assetPath: 'assets/sprites/Microgreen_Rug.png',
                name: 'Microgreen Rug',
                cost: 100,
                type: 'rug',
              ),
              shopItem(
                assetPath: 'assets/sprites/Couch.png',
                name: 'Couch',
                cost: 300,
                type: 'couch',
              ),
              shopItem(
                assetPath: 'assets/sprites/Dreamcatcher.png',
                name: 'Dreamcatcher',
                cost: 200,
                type: 'dreamcatcher',
              ),
              shopItem(
                assetPath: 'assets/sprites/Party_Button.png',
                name: 'Party Button',
                cost: 500,
                type: 'button1',
              ),

            ],
          ),
        ),
      ),
      body: AnimatedBuilder(
        animation: controller,
        builder: (context, child) {
          plantX += (plantTargetX - plantX) * 0.018;
          plantY += (plantTargetY - plantY) * 0.018;
          plantScale += (plantTargetScale - plantScale) * 0.018;

          final double distToTarget = (plantTargetX - plantX).abs() + (plantTargetY - plantY).abs();
          final double hopIntensity = (distToTarget / 30).clamp(0.0, 1.0);
          final double yOffset = sin(controller.value * pi) * 14 * hopIntensity;

          currentStage = getGrowthStage(widget.habitat);

          return Stack(
            children: [

              Positioned.fill(
                child: Image.asset(
                  'assets/sprites/Tomagachi_Background.png',
                  fit: BoxFit.cover,
                ),
              ),

              ...List.generate(widget.habitat.decorations.length, (index) {
                final deco = widget.habitat.decorations[index];
                final decoWidget = deco.type == 'sign1'
                        ? Image.asset('assets/sprites/MicroGrow_Sign.png', width: 300, height: 250)
                        : deco.type == 'rug'
                        ? Image.asset('assets/sprites/Microgreen_Rug.png', width: 400, height: 320)
                        : deco.type == 'dreamcatcher'
                        ? Image.asset('assets/sprites/Dreamcatcher.png', width: 300, height: 250)
                        : deco.type == 'couch'
                        ? Image.asset('assets/sprites/Couch.png', width: 400, height: 320)
                        : deco.type == 'button1'
                        ? Image.asset('assets/sprites/Party_Button.png', width: 100, height: 100)
                        : const SizedBox.shrink();

                return Positioned(
                  left: deco.x,
                  top: deco.y,
                  child: editMode
                      ? GestureDetector(
                          onPanUpdate: (details) {
                            updateDecorationPosition(
                              index,
                              Offset(
                                deco.x + details.delta.dx,
                                deco.y + details.delta.dy,
                              ),
                            );
                            setState(() {
                              isDraggingOverTrash = isOverTrash(details.globalPosition);
                            });
                          },
                          onPanEnd: (details) {
                            if (isDraggingOverTrash) {
                              setState(() {
                                widget.habitat.decorations.removeAt(index);
                                widget.habitat.save();
                                isDraggingOverTrash = false;
                              });
                            } else {
                              setState(() => isDraggingOverTrash = false);
                            }
                          },
                          child: Stack(
                            children: [
                              decoWidget,
                              Positioned.fill(
                                child: Container(
                                  decoration: BoxDecoration(
                                    border: Border.all(
                                      color: Colors.white.withOpacity(0.6),
                                      width: 2,
                                    ),
                                    borderRadius: BorderRadius.circular(8),
                                  ),
                                ),
                              ),
                            ],
                          ),
                        )
                      : decoWidget,
                );
              }),

              Positioned(
                bottom: 120 + yOffset - plantY,
                left: plantX,
                child: Transform.scale(
                  scale: plantScale,
                  alignment: Alignment.bottomCenter,
                  child: Column(
                    children: [
                      Text(
                        widget.habitat.name,
                        style: const TextStyle(
                          fontSize: 14,
                          fontWeight: FontWeight.bold,
                          color: Colors.white,
                          shadows: [
                            Shadow(offset: Offset(1, 1), blurRadius: 2, color: Colors.black),
                          ],
                        ),
                      ),
                      const SizedBox(height: 4),
                      Image.asset(
                        spriteAsset(widget.habitat, currentStage),
                        width: plantWidth,
                        height: plantHeight,
                        errorBuilder: (context, error, stackTrace) => Icon(
                          Icons.eco,
                          size: 60,
                          color: Colors.green[400],
                          shadows: const [
                            Shadow(offset: Offset(1, 1), blurRadius: 4, color: Colors.black54),
                          ],
                        ),
                      ),
                    ],
                  ),
                ),
              ),

              Positioned(
                top: 40,
                left: 16,
                child: Container(
                  padding: const EdgeInsets.symmetric(horizontal: 16, vertical: 8),
                  decoration: BoxDecoration(
                    color: Colors.black.withOpacity(0.4),
                    borderRadius: BorderRadius.circular(20),
                  ),
                  child: Row(
                    mainAxisSize: MainAxisSize.min,
                    children: [
                      const Icon(Icons.monetization_on, color: Colors.amber),
                      const SizedBox(width: 8),
                      Text(
                        Database.user.coins.toString(),
                        style: const TextStyle(
                          fontSize: 20,
                          fontWeight: FontWeight.bold,
                          color: Colors.white,
                        ),
                      ),
                    ],
                  ),
                ),
              ),

              if (!editMode)
                Positioned(
                  top: 40,
                  right: 16,
                  child: GestureDetector(
                    onTap: () => setState(() => editMode = true),
                    child: Container(
                      padding: const EdgeInsets.symmetric(horizontal: 16, vertical: 8),
                      decoration: BoxDecoration(
                        color: Colors.black.withOpacity(0.4),
                        borderRadius: BorderRadius.circular(20),
                      ),
                      child: const Row(
                        mainAxisSize: MainAxisSize.min,
                        children: [
                          Icon(Icons.edit, color: Colors.white, size: 20),
                          SizedBox(width: 6),
                          Text(
                            'Edit',
                            style: TextStyle(
                              fontSize: 16,
                              fontWeight: FontWeight.bold,
                              color: Colors.white,
                            ),
                          ),
                        ],
                      ),
                    ),
                  ),
                ),

              if (editMode)
                Positioned(
                  bottom: 32,
                  right: 24,
                  child: Column(
                    children: [
                      GestureDetector(
                        onTap: () => setState(() {
                          editMode = false;
                          isDraggingOverTrash = false;
                        }),
                        child: Container(
                          padding: const EdgeInsets.symmetric(horizontal: 16, vertical: 10),
                          decoration: BoxDecoration(
                            color: Colors.green[600],
                            borderRadius: BorderRadius.circular(20),
                            boxShadow: [
                              BoxShadow(color: Colors.black.withOpacity(0.3), blurRadius: 6, offset: const Offset(0, 3)),
                            ],
                          ),
                          child: const Row(
                            mainAxisSize: MainAxisSize.min,
                            children: [
                              Icon(Icons.check, color: Colors.white, size: 20),
                              SizedBox(width: 6),
                              Text(
                                'Done',
                                style: TextStyle(
                                  fontSize: 16,
                                  fontWeight: FontWeight.bold,
                                  color: Colors.white,
                                ),
                              ),
                            ],
                          ),
                        ),
                      ),

                      const SizedBox(height: 16),

                      Container(
                        key: trashKey,
                        width: 64,
                        height: 64,
                        decoration: BoxDecoration(
                          color: isDraggingOverTrash
                              ? Colors.red[600]
                              : Colors.black.withOpacity(0.5),
                          shape: BoxShape.circle,
                          border: Border.all(
                            color: isDraggingOverTrash ? Colors.red[200]! : Colors.white54,
                            width: 2,
                          ),
                          boxShadow: [
                            BoxShadow(color: Colors.black.withOpacity(0.3), blurRadius: 8, offset: const Offset(0, 3)),
                          ],
                        ),
                        child: Icon(
                          Icons.delete,
                          color: isDraggingOverTrash ? Colors.white : Colors.white70,
                          size: 30,
                        ),
                      ),
                    ],
                  ),
                ),

            ],
          );
        },
      ),
    );
  }
}
