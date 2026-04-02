import 'package:hive/hive.dart';
import 'growth_config.dart';
import 'sensor_history_obj.dart';
import '../mqtt/mqtt_connect.dart';
import 'decoration_obj.dart';

part 'habitat_obj.g.dart';

@HiveType(typeId: 0)
class Habitat extends HiveObject {
  @HiveField(0)
  String id;

  @HiveField(1)
  String name;

  @HiveField(2)
  String greenType;

  @HiveField(3)
  int tempTarget;

  @HiveField(4)
  int humidityTarget;

  @HiveField(5)
  int lightStartSec;

  @HiveField(6)
  int lightDurationSec;

  @HiveField(7)
  int lightIntervalSec;

  @HiveField(8)
  int waterStartSec;

  @HiveField(9)
  int waterDurationSec;

  @HiveField(10)
  int waterIntervalSec;

  @HiveField(11)
  final DateTime createdAt;

  @HiveField(12)
  bool harvested;

  @HiveField(13)
  List<SensorHistory> history;

  @HiveField(14)
  int reservoirVolume;

  @HiveField(15)
  List<DecorationObj> decorations;

  @HiveField(16)
  int blackoutDuration;

  @HiveField(17)
  bool blackoutAcknowledged;

  @HiveField(18)
  bool lightOverride;

  @HiveField(19)
  bool fanOverride;

  @HiveField(20)
  bool misterOverride;

  Habitat({
    required this.id,
    required this.name,
    required this.greenType,
    required this.tempTarget,
    required this.humidityTarget,
    required this.lightStartSec,
    int? lightDurationSec,
    int? lightIntervalSec,
    required this.waterStartSec,
    int? waterDurationSec,
    int? waterIntervalSec,
    DateTime? createdAt,
    bool? harvested,
    List<SensorHistory>? history,
    List<DecorationObj>? decorations,
    int? blackoutDuration,
    bool? blackoutAcknowledged,
    bool? lightOverride,
    bool? fanOverride,
    bool? misterOverride,
  })  : lightDurationSec = lightDurationSec ?? 0,
        lightIntervalSec = lightIntervalSec ?? 0,
        waterDurationSec = waterDurationSec ?? 0,
        waterIntervalSec = waterIntervalSec ?? 0,
        createdAt = createdAt ?? DateTime.now(),
        decorations = decorations ?? [],
        harvested = harvested ?? false,
        reservoirVolume = 50,
        history = history ?? [],
        blackoutAcknowledged = blackoutAcknowledged ?? false,
        lightOverride = lightOverride ?? false,
        fanOverride = fanOverride ?? false,
        misterOverride = misterOverride ?? false,
        blackoutDuration = blackoutDuration ?? 0;

  double get growthProgress {
    final spec = growthSpecs[greenType]!;
    final elapsed = DateTime.now().difference(createdAt);
    return elapsed.inMilliseconds / spec.totalDuration.inMilliseconds;
  }

  GrowthStage get currentGrowthStage {
    final spec = growthSpecs[greenType]!;
    final progress = growthProgress;

    if (progress >= spec.readyStart) {
      return GrowthStage.ready;
    } else if (progress >= spec.matureStart) {
      return GrowthStage.mature;
    } else if (progress >= spec.saplingStart) {
      return GrowthStage.sapling;
    } else {
      return GrowthStage.seed;
    }
  }

  Future<void> checkAndPublishGrowthStage() async {
    final newStage = currentGrowthStage;

    if (newStage != previousStage) {
      previousStage = newStage;
      await MqttService.publishGrowthStage(
        habitatId: id,
        stage: newStage,
      );
    }
  }

  Future<void> publishGrowthStage() async {
    final newStage = currentGrowthStage;
    await MqttService.publishGrowthStage(
      habitatId: id,
      stage: newStage,
    );
  }


  GrowthStage previousStage = GrowthStage.seed;
}