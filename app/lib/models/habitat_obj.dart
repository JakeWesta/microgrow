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
  int lightStartSec;

  @HiveField(6)
  int lightDurationSec;

  @HiveField(7)
  int lightIntervalSec;

  // Water Schedule
  @HiveField(8)
  int waterStartSec;

  @HiveField(9)
  int waterDurationSec;

  @HiveField(10)
  int waterIntervalSec;

  Habitat({
    required this.id,
    required this.name,
    required this.greenType,
    required this.tempTarget,
    required this.humidityTarget,
    required this.lightStartSec,
    required this.lightDurationSec,
    required this.lightIntervalSec,
    required this.waterStartSec,
    required this.waterDurationSec,
    required this.waterIntervalSec
  });
}
