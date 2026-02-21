import 'package:hive/hive.dart';

part 'decoration_obj.g.dart';

@HiveType(typeId: 4)
class DecorationObj extends HiveObject {
  @HiveField(0)
  String type; 

  @HiveField(1)
  double x; 

  @HiveField(2)
  double y;

  DecorationObj({required this.type, required this.x, required this.y});
}