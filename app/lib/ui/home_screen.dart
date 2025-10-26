import 'package:flutter/material.dart';
import 'package:provider/provider.dart';
import '../models/app_state.dart';


class HomeScreen extends StatelessWidget {
  const HomeScreen({super.key});

  @override
  Widget build(BuildContext context) {
    final appState = context.watch<MyAppState>();
    final habitats = appState.habitats;

    return Scaffold(
      appBar: AppBar(
                backgroundColor: Colors.green[700],
                title: Row(
                  children: [
                    Icon(Icons.eco, size: 32, color: Colors.white), // microgreen/leaf icon
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
                  print("meow");
                },
                child: const Text('Add a Habitat'),
              ),
            ],
          ),
        ),
      ),
    );
  }
}