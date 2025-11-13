import 'package:flutter/material.dart';
import 'package:hive_flutter/hive_flutter.dart';
import 'habitat_obj.dart';
import 'database.dart';

class MyAppState extends ChangeNotifier {
  List<Habitat> habitats = [];

  MyAppState() {
    habitats = Database.box.values.toList();
  }

  List<Habitat> get getHabitats => habitats;

  void addHabitat(Habitat habitat) async {
    await Database.saveHabitat(habitat);
    habitats = Database.box.values.toList();
    notifyListeners();
  }

    void deleteHabitat(Habitat habitat) async {
    await Database.box.delete(habitat.id);
    habitats = Database.box.values.toList();
    notifyListeners();
  }

}