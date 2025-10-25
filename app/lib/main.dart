import 'package:flutter/material.dart';
import 'package:mqtt_client/mqtt_client.dart';
import 'mqtt_service.dart';

void main() => runApp(const MyApp());

class MyApp extends StatelessWidget {
  const MyApp({super.key});

  @override
  Widget build(BuildContext context) {
    return const MaterialApp(
      debugShowCheckedModeBanner: false,
      title: 'MicroGrow Data',
      home: MqttHomePage(),
    );
  }
}

class MqttHomePage extends StatefulWidget {
  const MqttHomePage({super.key});

  @override
  State<MqttHomePage> createState() => _MqttHomePageState();
}

class _MqttHomePageState extends State<MqttHomePage> {
  MqttClient? _client;
  String _status = 'Disconnected';
  String _topic = 'microgrow/esp32_data';
  String _message = '';
  final List<String> _receivedMessages = [];
  final TextEditingController _msgController = TextEditingController();

  // Connect on startup
  @override
  void initState() {
    super.initState();
    _connectMqtt();
  }

  Future<void> _connectMqtt() async {
    setState(() => _status = 'Connecting...');
    try {
      final client = await connect();
      client.subscribe(_topic, MqttQos.atLeastOnce);
      client.updates?.listen((List<MqttReceivedMessage<MqttMessage?>>? c) {
        if (c == null || c.isEmpty) return;
        final recMessage = c[0].payload as MqttPublishMessage;
        final payload = MqttPublishPayload.bytesToStringAsString(
          recMessage.payload.message,
        );
        setState(() {
          _message = payload;
          _receivedMessages.insert(0, '[$_topic] $payload');
        });
        print('Received message: "$payload" from topic: ${c[0].topic}');
      });
      setState(() {
        _client = client;
        _status = 'Connected';
      });
    } catch (e) {
      print('MQTT connection failed: $e');
      setState(() => _status = 'Failed to connect');
    }
  }

  void _sendMessage() {
    if (_client == null ||
        _client!.connectionStatus?.state != MqttConnectionState.connected) {
      ScaffoldMessenger.of(
        context,
      ).showSnackBar(const SnackBar(content: Text('Not connected')));
      return;
    }

    final builder = MqttClientPayloadBuilder()..addString(_msgController.text);
    _client!.publishMessage(_topic, MqttQos.atLeastOnce, builder.payload!);
    setState(() {
      _receivedMessages.insert(0, '[You] ${_msgController.text}');
      _msgController.clear();
    });
  }

  @override
  void dispose() {
    _client?.disconnect();
    _msgController.dispose();
    super.dispose();
  }

  @override
  Widget build(BuildContext context) {
    return Scaffold(
      appBar: AppBar(
        title: const Text(
          'MicroGrow',
          style: TextStyle(
            color: Colors.white,
            fontWeight: FontWeight.bold,
            fontSize: 40,
          ),
        ),
        backgroundColor: Colors.green,
      ),
      body: Padding(
        padding: const EdgeInsets.all(16.0),
        child: Column(
          children: [
            Row(
              children: [
                Icon(
                  _status == 'Connected' ? Icons.check_circle : Icons.error,
                  color: _status == 'Connected' ? Colors.green : Colors.red,
                ),
                const SizedBox(width: 8),
                Text('Status: $_status'),
              ],
            ),
            const SizedBox(height: 16),
            TextField(
              controller: _msgController,
              decoration: const InputDecoration(
                border: OutlineInputBorder(),
                labelText: 'Send message',
              ),
            ),
            const SizedBox(height: 10),
            Row(
              children: [
                Expanded(
                  child: ElevatedButton.icon(
                    onPressed: _sendMessage,
                    icon: const Icon(Icons.send),
                    label: const Text('Publish'),
                  ),
                ),
                const SizedBox(width: 10),
                Expanded(
                  child: ElevatedButton.icon(
                    onPressed: _connectMqtt,
                    icon: const Icon(Icons.refresh),
                    label: const Text('Reconnect'),
                  ),
                ),
              ],
            ),
            const Divider(height: 30),
            Text(
              'Received Data:',
              style: const TextStyle(
                fontWeight: FontWeight.bold,
                fontSize: 16.0,
              ),
            ),
            const SizedBox(height: 10),
            Expanded(
              child: _receivedMessages.isEmpty
                  ? const Text('No data received yet')
                  : ListView.builder(
                      itemCount: _receivedMessages.length,
                      itemBuilder: (context, index) {
                        return ListTile(
                          leading: const Icon(Icons.message),
                          title: Text(_receivedMessages[index]),
                        );
                      },
                    ),
            ),
          ],
        ),
      ),
    );
  }
}
