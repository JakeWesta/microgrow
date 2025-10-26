import 'package:flutter/material.dart';

class Habitat{
  final String name;
  final String greenType;
  final String id;

  Habitat({required this.id, required this.name, required this.greenType});
}

class MyAppState extends ChangeNotifier {
  List<Habitat> habitats = [];

  void addHabitat(Habitat h) {
    habitats.add(h);
    notifyListeners();
  }
}