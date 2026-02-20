import 'package:flutter/material.dart';
import 'habitat_obj.dart';
import 'database.dart';
import 'growth_config.dart';
import 'dart:async';

class MyAppState extends ChangeNotifier {
  List<Habitat> habitats = [];

  bool harvestNotified = false;
  Map<String,bool> reservoirNotified = {}; 

  MyAppState() {
    habitats = Database.habitatsBox.values.toList();

    for (var h in habitats){
      reservoirNotified[h.id] = false;
    }

    Timer.periodic(const Duration(seconds: 1), (_) {
      for (var habitat in habitats) {
        final now = DateTime.now().second;
        if ((now - habitat.waterStartSec) % habitat.waterIntervalSec == 0) {
          subtractWater(habitat);
        }
      }

      checkReservoirWarnings();
      notifyListeners();
    });

  }

  final double reservoirVolume = 50; 
  final double flowRate = 100 / 60; 
  final double lowThreshold = 10; 

  double waterPerSession(double waterDurationSec) {
    return waterDurationSec * flowRate;
  }

  int sessionsUntilLow(double waterDurationSec, double currentAmount) {
    final perSession = waterPerSession(waterDurationSec);
    return ((currentAmount - lowThreshold) / perSession).floor();
  }

  Duration timeUntilLow(double waterDurationSec, int waterIntervalSec, double currentAmount) {
    final sessions = sessionsUntilLow(waterDurationSec, currentAmount);
    return Duration(seconds: sessions * waterIntervalSec);
  }

  final Map<String, double> reservoirLevels = {};

  void checkReservoirWarnings() {
    for (var habitat in habitats) {
      reservoirLevels.putIfAbsent(habitat.id, () => reservoirVolume);

      final remaining = reservoirLevels[habitat.id]!;

      if (remaining <= lowThreshold && reservoirNotified[habitat.id] != true) {
        notifyReservoirLow(habitat);
      }
    }
  }

  void notifyReservoirLow(Habitat habitat) {
    reservoirNotified[habitat.id] = true;
    notifyListeners();
  }



  void subtractWater(Habitat habitat) {
    reservoirLevels.putIfAbsent(habitat.id, () => reservoirVolume);
    final pumped = waterPerSession(habitat.waterDurationSec.toDouble());
    reservoirLevels[habitat.id] = (reservoirLevels[habitat.id]! - pumped).clamp(0, reservoirVolume);
  }

  List<Habitat> get getHabitats => habitats;

  Future<void> addHabitat(Habitat habitat) async {
    await Database.saveHabitat(habitat);
    habitats = Database.habitatsBox.values.toList();
    reservoirLevels[habitat.id] = reservoirVolume;
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

  bool get showReservoirNotification =>reservoirNotified.values.any((notified) => notified);

  void acknowledgeReservoirNotification([String? habitatId]) {
    if (habitatId != null) {
      reservoirNotified[habitatId] = false;

      reservoirLevels[habitatId] = reservoirVolume;
    } else {
      for (var key in reservoirNotified.keys) {
        reservoirNotified[key] = false;
        reservoirLevels[key] = reservoirVolume;
      }
    }
    notifyListeners();
  }



  Future<void> deleteHabitat(Habitat habitat) async {
    await Database.habitatsBox.delete(habitat.id);
    habitats = Database.habitatsBox.values.toList();
    reservoirLevels.remove(habitat.id);
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
