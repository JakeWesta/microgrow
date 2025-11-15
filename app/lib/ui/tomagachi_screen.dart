import 'package:flutter/material.dart';
import 'package:provider/provider.dart';
import '../models/app_state.dart';
import '../models/habitat_obj.dart';

class TomagachiScreen extends StatefulWidget {
  const TomagachiScreen({super.key});

  @override
  State<TomagachiScreen> createState() => _TomagachiScreenState();
}

class _TomagachiScreenState extends State<TomagachiScreen> {

  @override
  Widget build(BuildContext context) {
    final appState = context.watch<MyAppState>();
    final habitats = appState.getHabitats;

    return Scaffold(
      appBar: AppBar(
        backgroundColor: const Color.fromARGB(255, 57, 142, 60),
        title: Row(
          children: [
            Icon(Icons.eco, size: 32, color: const Color.fromARGB(255, 134, 245, 153)),
            const SizedBox(width: 10),
            const Text(
              'The Garden',
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
      
    );
  }
}
