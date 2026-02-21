import 'package:hive/hive.dart';

part 'user_obj.g.dart';

@HiveType(typeId: 2)
class User extends HiveObject {
  @HiveField(0)
  int coins;

  User({this.coins = 0});
}
