import 'package:hive/hive.dart';

part 'sensor_history_obj.g.dart';

@HiveType(typeId: 3)
class SensorHistory extends HiveObject {
  @HiveField(0)
  double temp;

  @HiveField(1)
  double humidity;

  @HiveField(2)
  DateTime timestamp;

  SensorHistory({
    required this.temp,
    required this.humidity,
    required this.timestamp,
  });
}
