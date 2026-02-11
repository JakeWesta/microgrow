import 'package:flutter/material.dart';
import 'habitat_obj.dart';
import 'database.dart';
import 'growth_config.dart';
import 'dart:async';

class MyAppState extends ChangeNotifier {
  List<Habitat> habitats = [];

  bool harvestNotified = false;

  MyAppState() {
    habitats = Database.habitatsBox.values.toList();

    Timer.periodic(const Duration(seconds: 1), (_) {
      notifyListeners();
    });
  }

  List<Habitat> get getHabitats => habitats;

  Future<void> addHabitat(Habitat habitat) async {
    await Database.saveHabitat(habitat);
    habitats = Database.habitatsBox.values.toList();
    notifyListeners();
  }

  bool get hasHarvestReady {
    return habitats.any(
      (h) => isHabitatReady(h.createdAt, h.harvested),
    );
  }

  List<Habitat> get harvestReadyHabitats {
    return habitats
        .where((h) => isHabitatReady(h.createdAt, h.harvested))
        .toList();
  }

  bool get showHarvestNotification {
    return hasHarvestReady && !harvestNotified;
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

  Future<void> deleteHabitat(Habitat habitat) async {
    await Database.habitatsBox.delete(habitat.id);
    habitats = Database.habitatsBox.values.toList();
    notifyListeners();
  }

  Future<void> harvestHabitat(Habitat habitat) async {
    if (habitat.harvested) return;

    habitat.harvested = true;
    await habitat.save();

    final user = Database.user;
    user.coins += 500;
    await user.save();

    habitats = Database.habitatsBox.values.toList();
    harvestNotified = false;
    notifyListeners();
  }

  int get userCoins => Database.user.coins;

}
