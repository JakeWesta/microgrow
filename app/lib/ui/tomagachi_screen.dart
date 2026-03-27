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
  int spriteFrame = 1;
  Timer? spriteTimer;

  double plantX = 0;
  double plantDirection = 1;

  final double plantWidth = 160; 
  final double plantHeight = 160;

  Color skyColor = Colors.lightBlue[300]!;

  Timer? partyTimer;
  bool isPartyOn = false;

  @override
  void initState() {
    super.initState();
    controller = AnimationController(
      vsync: this,
      duration: const Duration(milliseconds: 800),
    )..repeat(reverse: true);

    plantX = random.nextDouble() * 200;
    plantDirection = random.nextBool() ? 1.0 : -1.0;
    currentStage = getGrowthStage(widget.habitat);

    spriteTimer = Timer.periodic(const Duration(seconds: 1), (_) {
      setState(() {
        spriteFrame = spriteFrame == 1 ? 2 : 1;
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

  @override
  void dispose() {
    partyTimer?.cancel();
    spriteTimer?.cancel();
    controller.dispose();
    super.dispose();
  }

  String spriteAsset(Habitat habitat, GrowthStage stage, int frame) {
    final type = habitat.greenType.toLowerCase();
    String stageName;
    switch(stage) {
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
    return 'assets/sprites/$type-$stageName-$frame.png';
  }

  void triggerParty() {
    final partyColors = [
      Colors.red,
      Colors.blue,
      Colors.green,
      Colors.yellow,
      Colors.purple,
      Colors.orange,
      Colors.cyan,
      Colors.pink,
    ];

    if (isPartyOn) {
      partyTimer?.cancel();
      setState(() {
        isPartyOn = false;
        skyColor = Colors.lightBlue[300]!;
      });
    } else {
      isPartyOn = true;
      partyTimer = Timer.periodic(const Duration(milliseconds: 500), (timer) {
        setState(() => skyColor = partyColors[random.nextInt(partyColors.length)]);
      });
    }
  }

  void buyDecoration(String name, int cost) {
    final user = Database.user;
    if (user.coins >= cost) {
      user.coins -= cost;
      user.save();

      final decoration = DecorationObj(type: name, x: 150, y: 150);
      widget.habitat.decorations.add(decoration);
      widget.habitat.save();
      setState(() {});
    }
  }

  void updateDecorationPosition(int index, Offset position) {
    final deco = widget.habitat.decorations[index];
    deco.x = position.dx;
    deco.y = position.dy;
    widget.habitat.save();
    setState(() {});
  }

  @override
  Widget build(BuildContext context) {
    final screenWidth = MediaQuery.of(context).size.width;

    return Scaffold(
      appBar: AppBar(
        backgroundColor: Colors.green[700],
        title: Row(
          children: [
            Icon(Icons.eco, size: 32, color: const Color.fromARGB(255, 134, 245, 153)),
            const SizedBox(width: 10),
            Text(
              ("${widget.habitat.name}'s House"),
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
        child: ListView(
          padding: EdgeInsets.zero,
          children: [
            const DrawerHeader(child: Text('Shop')),
            ListTile(
              leading: Image.asset('assets/sprites/MicroGrow_Sign.png', width: 32, height: 32),
              title: const Text('MicroGrow Sign - 50 coins'),
              onTap: () {
                buyDecoration('sign', 50);
                Navigator.pop(context);
              },
            ),
            ListTile(
              leading: Image.asset('assets/sprites/Microgreen_Rug.png', width: 32, height: 32),
              title: const Text('Microgreen Rug - 100 coins'),
              onTap: () {
                buyDecoration('rug', 100);
                Navigator.pop(context);
              },
            ),
          ],
        ),
      ),
      body: AnimatedBuilder(
        animation: controller,
        builder: (context, child) {
          plantX += plantDirection * 2;
          if (plantX < 0 || plantX > screenWidth - plantWidth) plantDirection *= -1;
          double yOffset = sin(controller.value * pi) * 20;

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
                return Positioned(
                  left: deco.x,
                  top: deco.y,
                  child: GestureDetector(
                    onPanUpdate: (details) {
                      updateDecorationPosition(
                          index,
                          Offset(
                            deco.x + details.delta.dx,
                            deco.y + details.delta.dy,
                          ));
                    },
                    child: deco.type == 'sign'
                        ? Image.asset('assets/sprites/MicroGrow_Sign.png', width: 300, height: 250)
                        : deco.type == 'rug'
                            ? Image.asset('assets/sprites/Microgreen_Rug.png', width: 400, height: 320)
                            : const SizedBox.shrink(),
                  ),
                );
              }),

              Positioned(
                bottom: 120 + yOffset,
                left: plantX,
                child: Column(
                  children: [
                    Text(
                      widget.habitat.name,
                      style: const TextStyle(
                        fontSize: 14,
                        fontWeight: FontWeight.bold,
                        color: Colors.white,
                        shadows: [
                          Shadow(offset: Offset(1, 1), blurRadius: 2, color: Colors.black)
                        ],
                      ),
                    ),
                    const SizedBox(height: 4),
                    Image.asset(
                      spriteAsset(widget.habitat, currentStage, spriteFrame),
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
                            fontSize: 20, fontWeight: FontWeight.bold, color: Colors.white),
                      ),
                    ],
                  ),
                ),
              ),
            ],
          );
        },
      ),
    );
  }
}