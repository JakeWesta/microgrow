import 'package:hive/hive.dart';
import 'sensor_history_obj.dart';
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
  })  : lightDurationSec = lightDurationSec ?? 0,
        lightIntervalSec = lightIntervalSec ?? 0,
        waterDurationSec = waterDurationSec ?? 0,
        waterIntervalSec = waterIntervalSec ?? 0,
        createdAt = createdAt ?? DateTime.now(),
        decorations = decorations ?? [],
        harvested = harvested ?? false,
        reservoirVolume = 50,
        history = history ?? [];
}

