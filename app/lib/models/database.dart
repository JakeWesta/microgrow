import 'package:hive_flutter/hive_flutter.dart';
import 'habitat_obj.dart';
import 'user_obj.dart';

class Database {
  static Future<void> init() async {
    await Hive.initFlutter();

    Hive.registerAdapter(HabitatAdapter());
    Hive.registerAdapter(UserAdapter());

    await Hive.openBox<Habitat>('habitatsBox');
    await Hive.openBox<User>('userBox');

    final userBox = Hive.box<User>('userBox');
    if (userBox.isEmpty) {
      await userBox.put('user', User());
    }
  }

  static Box<Habitat> get habitatsBox =>
      Hive.box<Habitat>('habitatsBox');

  static Future<void> saveHabitat(Habitat habitat) async {
    await habitatsBox.put(habitat.id, habitat);
  }

  static Box<User> get userBox =>
      Hive.box<User>('userBox');

  static User get user =>
      userBox.get('user')!;
}
