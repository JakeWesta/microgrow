import 'package:flutter/material.dart';
import 'package:provider/provider.dart';
import 'package:firebase_core/firebase_core.dart';
import 'package:firebase_messaging/firebase_messaging.dart';
import 'firebase_options.dart';

import 'models/app_state.dart';
import 'models/database.dart';
import 'ui/main_nav_screen.dart';

Future<void> main() async {
  WidgetsFlutterBinding.ensureInitialized();

  // Init Firebase
  await Firebase.initializeApp(
    options: DefaultFirebaseOptions.currentPlatform,
  );

  // Init local database
  await Database.init();

  // Init push notifications
  initPush();

  // Run the app
  runApp(const MicroGrowApp());
}

Future<void> initPush() async {
  final messaging = FirebaseMessaging.instance;

  await messaging.requestPermission(
    alert: true,
    badge: true,
    sound: true,
  );

  // Get FSM token and print it
  try {
    final token = await messaging.getToken();
    debugPrint('FCM Token: $token');
  } 
  catch (error) {
    debugPrint('Failed to get FCM token: $error');
  }
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
          colorScheme: ColorScheme.fromSeed(
            seedColor: const Color.fromARGB(255, 15, 156, 10),
          ),
        ),
        home: HomePage(),
      ),
    );
  }
}