import 'package:flutter/material.dart';
import 'package:provider/provider.dart';
import 'ui/home_screen.dart';
import 'models/app_state.dart';
import 'models/database.dart';
import 'ui/main_nav_screen.dart';

void main() async {
  WidgetsFlutterBinding.ensureInitialized();
  
  await Database.init();

  runApp(MicroGrowApp());
}

class MicroGrowApp extends StatelessWidget {
  const MicroGrowApp({super.key});

  @override
  Widget build(BuildContext context) {
    return ChangeNotifierProvider(
      create: (context) => MyAppState(),
      child: MaterialApp(
        title: 'Micro-Grow',
        theme: ThemeData(
          useMaterial3: true,
          colorScheme: ColorScheme.fromSeed(seedColor: const Color.fromARGB(255, 15, 156, 10)),
        ),
        home: HomePage(),
      ),
    );
  }
}