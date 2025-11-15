import 'package:hive/hive.dart';

part 'habitat_obj.g.dart';

@HiveType(typeId: 0)
class Habitat extends HiveObject {
  @HiveField(0)
  String id;

  @HiveField(1)
  String name;

  @HiveField(2)
  String greenType;

  // Targets
  @HiveField(3)
  int tempTarget;

  @HiveField(4)
  int humidityTarget;

  // Light Schedule
  @HiveField(5)
  int lightStartMs;

  @HiveField(6)
  int lightDurationMs;

  @HiveField(7)
  int lightIntervalMs;

  // Water Schedule
  @HiveField(8)
  int waterStartMs;

  @HiveField(9)
  int waterDurationMs;

  @HiveField(10)
  int waterIntervalMs;

  Habitat({
    required this.id,
    required this.name,
    required this.greenType,
    required this.tempTarget,
    required this.humidityTarget,
    required this.lightStartMs,
    required this.lightDurationMs,
    required this.lightIntervalMs,
    required this.waterStartMs,
    required this.waterDurationMs,
    required this.waterIntervalMs
  });
}
