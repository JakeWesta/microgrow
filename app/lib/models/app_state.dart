import 'package:flutter/material.dart';
import 'habitat_obj.dart';
import 'database.dart';
import 'growth_config.dart';
import 'dart:async';
import '../mqtt/mqtt_connect.dart';
import '../main.dart';

class MyAppState extends ChangeNotifier {
  List<Habitat> habitats = [];
  bool harvestNotified = false;
  Map<String, bool> reservoirNotified = {};
  Map<String, bool> blackoutNotified = {};
  Map<String, bool> blackoutAcknowledged = {};
  final bool useDemoConfig = true; 
  final int dailyCoinReward = 100;
  final int reservoirCoinReward = 200;
  final int harvestCoinReward = 500;



  MyAppState() {
    habitats = Database.habitatsBox.values.toList();


    for (var h in habitats) {
      reservoirNotified[h.id] = false;
      blackoutNotified[h.id] = h.blackoutAcknowledged; 
      blackoutAcknowledged[h.id] = h.blackoutAcknowledged;
      reservoirLevels[h.id] = reservoirVolume;
    }

    Timer.periodic(const Duration(seconds: 1), (_) async {
      for (var habitat in habitats) {
        final now = DateTime.now().second;
        
        if ((now - habitat.waterStartSec) % habitat.waterIntervalSec == 0) {
          subtractWater(habitat);
        }

        if (habitat.currentGrowthStage != GrowthStage.ready){
          await habitat.checkAndPublishGrowthStage();
        }
      }

      checkReservoirWarnings();
      checkBlackoutComplete();
      checkDailyCoinReward();
      notifyListeners();
    });
  }

  void checkDailyCoinReward() {
    final activeHabitats = habitats.where((h) => !h.harvested && !isHabitatReady(h)).toList();
    if (activeHabitats.isEmpty) return;

    final user = Database.user;
    final now = DateTime.now();
    final last = user.lastDailyClaim;

    final interval = useDemoConfig
        ? const Duration(seconds: 10)
        : const Duration(days: 1);

    if (last == null || now.difference(last) >= interval) {
      user.coins += dailyCoinReward;
      user.lastDailyClaim = now;
      user.save();
      notifyListeners();
    }
  }

  void checkBlackoutComplete() async {
    for (var habitat in habitats) {
      if (habitat.blackoutDuration <= 0) continue;
      if (blackoutNotified[habitat.id] == true) continue;

      final elapsed = DateTime.now().difference(habitat.createdAt).inSeconds;
      if (elapsed >= habitat.blackoutDuration) {
        blackoutNotified[habitat.id] = true;
        await MqttService.blackoutEnd(
          habitatId: habitat.id,
        );
        notifyListeners();
      }
    }
  }

  bool get showBlackoutNotification =>
    habitats.any((h) => blackoutNotified[h.id] == true && blackoutAcknowledged[h.id] != true);

  void acknowledgeBlackoutNotification([String? habitatId]) async {
    if (habitatId != null) {
      blackoutAcknowledged[habitatId] = true;
      final habitat = habitats.firstWhere((h) => h.id == habitatId);
      habitat.blackoutAcknowledged = true;
      await habitat.save();
    } else {
      for (var h in habitats) {
        blackoutAcknowledged[h.id] = true;
        h.blackoutAcknowledged = true;
        await h.save();
      }
    }
    notifyListeners();
  }

  final double reservoirVolume = 40;
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
    sendNotification('Water Level Low', '${habitat.name}: Water reservoir is running low! Please refill soon.');
    notifyListeners();
  }

  void subtractWater(Habitat habitat) {
    reservoirLevels.putIfAbsent(habitat.id, () => reservoirVolume);
    final pumped = waterPerSession(habitat.waterDurationSec.toDouble());
    reservoirLevels[habitat.id] = (reservoirLevels[habitat.id]! - pumped).clamp(0, reservoirVolume);
  }

  void updateGrowthStage(Habitat habitat) async {
    final newStage = habitat.currentGrowthStage;

    if (newStage != habitat.previousStage) {
      habitat.previousStage = newStage; 
      await MqttService.publishGrowthStage(
        habitatId: habitat.id,
        stage: newStage,
      );
    }
  }

  List<Habitat> get getHabitats => habitats;

  Future<void> addHabitat(Habitat habitat) async {
    final usedSlots = habitats.map((h) => h.slotIndex).toSet();
    int assignedSlot = 0;
    for (int i = 0; i < 6; i++) {
      if (!usedSlots.contains(i)) {
        assignedSlot = i;
        break;
      }
    }
    habitat.slotIndex = assignedSlot;

    await Database.saveHabitat(habitat);
    habitats = Database.habitatsBox.values.toList();
    reservoirLevels[habitat.id] = reservoirVolume;
    blackoutNotified[habitat.id] = false;
    blackoutAcknowledged[habitat.id] = false;
    notifyListeners();
  }

  bool get hasHarvestReady {
    return habitats.any(
      (h) => isHabitatReady(h),
    );
  }

  List<Habitat> get harvestReadyHabitats {
    return habitats
        .where((h) => isHabitatReady(h))
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

  bool get showReservoirNotification => reservoirNotified.values.any((notified) => notified);

  void acknowledgeReservoirNotification([String? habitatId]) {
    final user = Database.user;

    if (habitatId != null) {
      if (reservoirNotified[habitatId] == true) {
        user.coins += reservoirCoinReward;
        user.save();
      }
      reservoirNotified[habitatId] = false;
      reservoirLevels[habitatId] = reservoirVolume;
    } else {
      for (var key in reservoirNotified.keys) {
        if (reservoirNotified[key] == true) {
          user.coins += reservoirCoinReward;
        }
        reservoirNotified[key] = false;
        reservoirLevels[key] = reservoirVolume;
      }
      user.save();
    }
    notifyListeners();
  }

  Future<void> deleteHabitat(Habitat habitat) async {
    await Database.habitatsBox.delete(habitat.id);
    habitats = Database.habitatsBox.values.toList();
    reservoirLevels.remove(habitat.id);
    blackoutNotified.remove(habitat.id);
    blackoutAcknowledged.remove(habitat.id);
    notifyListeners();
  }

  Future<void> harvestHabitat(Habitat habitat) async {
    if (habitat.harvested) return;

    habitat.harvested = true;
    await habitat.save();

    final user = Database.user;
    user.coins += harvestCoinReward;
    await user.save();

    habitats = Database.habitatsBox.values.toList();
    harvestNotified = false;
    notifyListeners();
  }

  int get userCoins => Database.user.coins;
}