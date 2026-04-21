import 'dart:async';

import 'package:flutter/material.dart';

import '../main.dart';
import '../mqtt/mqtt_connect.dart';
import 'database.dart';
import 'growth_config.dart';
import 'habitat_obj.dart';

class MyAppState extends ChangeNotifier {
  List<Habitat> habitats = [];

  bool harvestNotified = false;
  bool harvestPushSent = false;

  Map<String, bool> reservoirNotified = {};
  Map<String, bool> blackoutNotified = {};
  Map<String, bool> blackoutAcknowledged = {};

  final Set<String> pulseSubscribedHabitats = {};

  final bool useDemoConfig = true;
  final int dailyCoinReward = 100;
  final int reservoirCoinReward = 200;
  final int harvestCoinReward = 500;

  final double flowRate = 6.0;
  final double lowThreshold = 300;

  MyAppState() {
    habitats = Database.habitatsBox.values.toList();

    for (var h in habitats) {
      reservoirNotified[h.id] = false;
      blackoutNotified[h.id] = false;
      blackoutAcknowledged[h.id] = h.blackoutAcknowledged;
      subscribeToHabitatPulse(h);
    }

    Timer.periodic(const Duration(seconds: 1), (_) async {
      for (var habitat in habitats) {
        if (habitat.currentGrowthStage != GrowthStage.ready) {
          await habitat.checkAndPublishGrowthStage();
        }
      }

      checkBlackoutComplete();
      checkHarvestWarnings();
      checkDailyCoinReward();
      notifyListeners();
    });
  }

  void subscribeToHabitatPulse(Habitat habitat) {
    if (pulseSubscribedHabitats.contains(habitat.id)) return;
    pulseSubscribedHabitats.add(habitat.id);

    MqttService.pulseSubscribe(
      habitatId: habitat.id,
      onPulse: () async {
        await handleWaterPulse(habitat.id);
      },
    );
  }

  void checkDailyCoinReward() {
    final activeHabitats =
        habitats.where((h) => !h.harvested && !isHabitatReady(h)).toList();

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

  void notifyBlackoutComplete(Habitat habitat) {
    sendNotification(
      'Blackout Complete',
      '${habitat.name} has completed its blackout period and is ready for light exposure.',
    );
    notifyListeners();
  }

  void checkBlackoutComplete() async {
    for (var habitat in habitats) {
      if (habitat.blackoutDuration <= 0) continue;
      if (blackoutNotified[habitat.id] == true) continue;

      final elapsed = DateTime.now().difference(habitat.createdAt).inSeconds;

      if (elapsed >= habitat.blackoutDuration) {
        blackoutNotified[habitat.id] = true;
        blackoutAcknowledged[habitat.id] = false;

        habitat.blackoutAcknowledged = false;
        await habitat.save();

        await MqttService.blackoutEnd(
          habitatId: habitat.id,
        );

        notifyBlackoutComplete(habitat);
      }
    }
  }

  bool get showBlackoutNotification => habitats.any(
        (h) =>
            blackoutNotified[h.id] == true &&
            blackoutAcknowledged[h.id] != true,
      );

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

  void notifyReservoirLow(Habitat habitat) {
    reservoirNotified[habitat.id] = true;
    sendNotification(
      'Water Level Low',
      '${habitat.name}: Water reservoir is running low! Please refill soon.',
    );
    notifyListeners();
  }

  Future<void> handleWaterPulse(String habitatId) async {
    final habitat = habitats.firstWhere((h) => h.id == habitatId);

    final amountUsed = habitat.waterDurationSec * flowRate;

    habitat.currentReservoirVolume =
        (habitat.currentReservoirVolume - amountUsed)
            .clamp(0.0, habitat.maxReservoirVolume);

    await habitat.save();

    if (habitat.currentReservoirVolume <= lowThreshold &&
        reservoirNotified[habitat.id] != true) {
      notifyReservoirLow(habitat);
    }

    notifyListeners();
  }

  void checkHarvestWarnings() {
    if (harvestPushSent) return;

    for (var habitat in habitats) {
      if (isHabitatReady(habitat)) {
        notifyHarvestReady(habitat);
        return;
      }
    }

    if (!hasHarvestReady) {
      harvestPushSent = false;
    }
  }

  void notifyHarvestReady(Habitat habitat) {
    harvestPushSent = true;
    sendNotification(
      'Harvest Ready',
      '${habitat.name} is ready to harvest! Please check your habitats.',
    );
    notifyListeners();
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

    reservoirNotified[habitat.id] = false;
    blackoutNotified[habitat.id] = false;
    blackoutAcknowledged[habitat.id] = false;

    subscribeToHabitatPulse(habitat);
    notifyListeners();
  }

  bool get hasHarvestReady {
    return habitats.any((h) => isHabitatReady(h));
  }

  List<Habitat> get harvestReadyHabitats {
    return habitats.where((h) => isHabitatReady(h)).toList();
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
      harvestPushSent = false;
    }
  }

  bool get showReservoirNotification =>
      reservoirNotified.values.any((notified) => notified);

  Future<void> acknowledgeReservoirNotification([String? habitatId]) async {
    final user = Database.user;

    if (habitatId != null) {
      final habitat = habitats.firstWhere((h) => h.id == habitatId);

      if (reservoirNotified[habitatId] == true) {
        user.coins += reservoirCoinReward;
        await user.save();
      }

      reservoirNotified[habitatId] = false;
      habitat.currentReservoirVolume = habitat.maxReservoirVolume;
      await habitat.save();
    } else {
      for (var habitat in habitats) {
        if (reservoirNotified[habitat.id] == true) {
          user.coins += reservoirCoinReward;
        }

        reservoirNotified[habitat.id] = false;
        habitat.currentReservoirVolume = habitat.maxReservoirVolume;
        await habitat.save();
      }

      await user.save();
    }

    notifyListeners();
  }

  Future<void> deleteHabitat(Habitat habitat) async {
    await Database.habitatsBox.delete(habitat.id);
    habitats = Database.habitatsBox.values.toList();

    reservoirNotified.remove(habitat.id);
    blackoutNotified.remove(habitat.id);
    blackoutAcknowledged.remove(habitat.id);
    pulseSubscribedHabitats.remove(habitat.id);

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
    harvestPushSent = false;
    notifyListeners();
  }

  int get userCoins => Database.user.coins;
}