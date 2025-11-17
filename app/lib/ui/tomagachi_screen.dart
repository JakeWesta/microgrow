import 'dart:math';
import 'package:flutter/material.dart';
import 'package:provider/provider.dart';
import '../models/app_state.dart';

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

  @override
  void initState() {
    super.initState();
    controller = AnimationController(
      vsync: this,
      duration: const Duration(milliseconds: 800),
    )..repeat(reverse: true);
  }

  @override
  void dispose() {
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
              Container(color: Colors.lightBlue[300]),
              Positioned(
                top: 40,
                left: 40,
                child: Icon(Icons.wb_sunny, size: 60, color: Colors.yellow[700]),
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
