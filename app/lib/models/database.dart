import 'package:hive_flutter/hive_flutter.dart';
import 'habitat_obj.dart';

class Database {

  static Future<void> init() async {
    await Hive.initFlutter();
    Hive.registerAdapter(HabitatAdapter());
    await Hive.openBox<Habitat>('habitatsBox');
  }

  static Box<Habitat> get box => Hive.box<Habitat>('habitatsBox');

  static Future<void> saveHabitat(Habitat habitat) async {
    await box.put(habitat.id, habitat);
  }
  
}
