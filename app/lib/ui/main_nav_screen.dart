import 'package:flutter/material.dart';
import 'home_screen.dart';
import 'tomagachi_screen.dart';

class HomePage extends StatefulWidget {
  const HomePage({super.key});

  @override
  State<HomePage> createState() => _HomePageState();
}

class _HomePageState extends State<HomePage> {
  int selectedIndex = 0;

  @override
  Widget build(BuildContext context) {
    Widget page;
    switch (selectedIndex) {
      case 0:
        page = const HomeScreen();
        break;
      case 1:
        page = const TomagachiScreen();
        break;
      default:
        page = const Center(child: Text('Unknown page'));
    }

    return Scaffold(
      body: AnimatedSwitcher(
        duration: const Duration(milliseconds: 200),
        child: page,
      ),
      bottomNavigationBar: BottomNavigationBar(
        items: const [
          BottomNavigationBarItem(
            icon: Icon(Icons.home),
            label: 'Home',
          ),
          BottomNavigationBarItem(
            icon: Icon(Icons.eco),
            label: 'Garden',
          ),
        ],
        currentIndex: selectedIndex,
        onTap: (value) {
          setState(() => selectedIndex = value);
        },
        backgroundColor: Color.fromARGB(255, 82, 175, 88), 
        selectedItemColor: Color.fromARGB(255, 134, 245, 153), 
        unselectedItemColor: Colors.white, 
        selectedLabelStyle: const TextStyle(
          fontSize: 16,
          fontWeight: FontWeight.bold,
        ),
        unselectedLabelStyle: const TextStyle(
          fontSize: 14,
          fontWeight: FontWeight.normal,
        ),
      ),
    );
  }
}
