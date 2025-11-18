import 'dart:async';
import 'dart:math';
import 'package:flutter/material.dart';
import 'package:provider/provider.dart';
import '../models/app_state.dart';
import '../mqtt/mqtt_connect.dart';



class TomagachiScreen extends StatefulWidget {
  const TomagachiScreen({super.key});

  @override
  State<TomagachiScreen> createState() => _TomagachiScreenState();
}

class _TomagachiScreenState extends State<TomagachiScreen>
    with SingleTickerProviderStateMixin {
  late AnimationController controller;
  final Random random = Random();

  List<double> baseX = [];
  List<double> direction = [];

  final double iconWidth = 50;

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
  }

  @override
@override
  void dispose() {
    partyTimer?.cancel();
    controller.dispose();
    super.dispose();
  }


  IconData iconForGreenType(String type) {
    switch (type.toLowerCase()) {
      case 'basil':
        return Icons.grass;
      case 'broccoli':
        return Icons.local_florist;
      default:
        return Icons.spa;
    }
  }

  void triggerParty() {
    final List<Color> partyColors = [
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
      partyTimer = null;
      setState(() {
        isPartyOn = false;
        skyColor = Colors.lightBlue[300]!;
      });
    
    final habitats = context.read<MyAppState>().getHabitats;
    
    for (final habitat in habitats) {
        MqttService.actuatorPublish(
          habitatId: habitat.id,
          actuatorName: 'light',
          val: 0,
          r: 0,
          g: 0,
          b: 0,
        );
      }
    } else {
      isPartyOn = true;

      partyTimer = Timer.periodic(const Duration(milliseconds: 500), (timer) {
        final Color c = partyColors[Random().nextInt(partyColors.length)];

        setState(() => skyColor = c);

        final habitats = context.read<MyAppState>().getHabitats;
        for (final habitat in habitats) {
          MqttService.actuatorPublish(
            habitatId: habitat.id,
            actuatorName: 'light',
            val: 1,
              r: (c.r * 255).round(),
              g: (c.g * 255).round(),
              b: (c.b * 255).round(),
          );
        }
      });
    }
  }


  @override
  Widget build(BuildContext context) {
    final habitats = context.watch<MyAppState>().getHabitats;
    final screenWidth = MediaQuery.of(context).size.width;

    if (baseX.length != habitats.length) {
      baseX = List.generate(
          habitats.length, (i) => random.nextDouble() * (screenWidth - iconWidth));
      direction =
          List.generate(habitats.length, (i) => random.nextBool() ? 1.0 : -1.0);
    }

    return Scaffold(
      body: AnimatedBuilder(
        animation: controller,
        builder: (context, child) {
          for (int i = 0; i < habitats.length; i++) {
            double x = baseX[i] + direction[i] * 2;
            if (x < 0 || x > screenWidth - iconWidth) direction[i] *= -1;
            baseX[i] += direction[i] * 2;
          }

          return Stack(
            children: [
              Container(color: skyColor),
              Positioned(
                top: 40,
                left: 40,
                child: Icon(Icons.wb_sunny, size: 60, color: Colors.yellow[700]),
              ),
              Positioned(
              top: 40,
              right: 20,
              child: ElevatedButton(
                onPressed: triggerParty,
                style: ElevatedButton.styleFrom(
                  backgroundColor: const Color.fromARGB(255, 159, 156, 159),
                  padding: const EdgeInsets.symmetric(horizontal: 16, vertical: 8),
                  shape: RoundedRectangleBorder(borderRadius: BorderRadius.circular(12)),
                ),
                child: Text(
                  isPartyOn ? 'Stop Party' : 'Party Time',
                  style: const TextStyle(fontWeight: FontWeight.bold, color: Colors.white),
                ),
              ),
            ),
              Positioned(
                bottom: 0,
                left: 0,
                right: 0,
                height: 120,
                child: Container(
                  color: Colors.brown[400],
                ),
              ),
              ...List.generate(habitats.length, (index) {
                final habitat = habitats[index];

                double yOffset = sin(controller.value * pi) * 20;

                return Positioned(
                  bottom: 120 + yOffset,
                  left: baseX[index],
                  child: Column(
                    children: [
                      Text(
                        habitat.name,
                        style: const TextStyle(
                            fontSize: 14,
                            fontWeight: FontWeight.bold,
                            color: Colors.white,
                            shadows: [
                              Shadow(
                                  offset: Offset(1, 1),
                                  blurRadius: 2,
                                  color: Colors.black)
                            ]),
                      ),
                      const SizedBox(height: 4),
                      Icon(
                        iconForGreenType(habitat.greenType),
                        size: 40,
                        color: Colors.green[700],
                      ),
                    ],
                  ),
                );
              }),
            ],
          );
        },
      ),
    );
  }
}
