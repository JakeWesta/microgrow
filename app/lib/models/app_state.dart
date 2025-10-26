import 'package:hive/hive.dart';

part 'app_state.g.dart';

@HiveType(typeId: 0)
class Habitat extends HiveObject {
  @HiveField(0)
  String id;

  @HiveField(1)
  String name;

  @HiveField(2)
  String greenType;

  @HiveField(3)
  int? lastSensorValue;

  Habitat({required this.id, required this.name, required this.greenType, this.lastSensorValue, });
}
