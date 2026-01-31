import 'package:flutter/material.dart';
import 'habitat_obj.dart';
import 'database.dart';
import 'growth_config.dart';
import 'dart:async';



class MyAppState extends ChangeNotifier {
  List<Habitat> habitats = [];

  bool harvestNotified = false;

  MyAppState() {
    habitats = Database.box.values.toList();

    Timer.periodic(const Duration(seconds: 1), (_) {
      notifyListeners();
    });
  } 

  List<Habitat> get getHabitats => habitats;

  void addHabitat(Habitat habitat) async {
    await Database.saveHabitat(habitat);
    habitats = Database.box.values.toList();
    notifyListeners();
  }

  bool get hasHarvestReady {
    return habitats.any((h) => isHabitatReady(h.createdAt));
  }

  bool get showHarvestNotification {
    return hasHarvestReady && !harvestNotified;
  }

  List<Habitat> get harvestReadyHabitats {
    return habitats.where((h) => isHabitatReady(h.createdAt)).toList();
  }

  void acknowledgeHarvestNotification() {
    harvestNotified = true;
    notifyListeners();
  }

  void resetHarvestNotification() {
    if (!hasHarvestReady) {
      harvestNotified = false;
    }
  }

  void deleteHabitat(Habitat habitat) async {
    await Database.box.delete(habitat.id);
    habitats = Database.box.values.toList();
    notifyListeners();
  }


}