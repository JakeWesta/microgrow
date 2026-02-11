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
    required this.waterIntervalSec,
    DateTime? createdAt,
    bool? harvested
  }) : createdAt = createdAt ?? DateTime.now(),
       harvested = false;
}
