import 'package:mqtt_client/mqtt_client.dart';
import 'package:mqtt_client/mqtt_browser_client.dart';
import 'dart:convert';
import 'package:typed_data/typed_data.dart';


class MqttService {
  static const host = 'broker.emqx.io';
  static const port = 8083;
  static const path = '/mqtt';
  static MqttBrowserClient? client;
  
  
  static Future<MqttBrowserClient> connect() async {
    final url = 'ws://$host:$port$path';

      final client = MqttBrowserClient(
        url,
        'flutter_web_${DateTime.now().millisecondsSinceEpoch}',
      );
      client.port = port;
      client.websocketProtocols =
          MqttClientConstants.protocolsSingleDefault; // ['mqtt']
      client.keepAlivePeriod = 30;
      client.setProtocolV311();
      client.logging(on: true);

      client.onConnected = () => print('Connected');
      client.onDisconnected = () => print('Disconnected');
      client.onSubscribed = (t) => print('Subscribed: $t');

      final conn = MqttConnectMessage()
          .startClean()
          .withWillTopic('app/status')
          .withWillMessage('offline')
          .withWillQos(MqttQos.atLeastOnce);
      client.connectionMessage = conn;

      print('Connecting to $url ...');
      await client.connect();

      if (client.connectionStatus?.state != MqttConnectionState.connected) {
        final rc = client.connectionStatus?.returnCode;
        client.disconnect();
        throw Exception('MQTT not connected. CONNACK=$rc');
      }

      client.subscribe('microgrow/init', MqttQos.atLeastOnce);

      return client;

  }

  static Future<void> setupHabitat({required String habitatId, required String greenType, required int schedule}) async {
    final client = await connect();
    final topic = 'microgrow/init';

    final msg = jsonEncode({
      'id': habitatId,
      'green': greenType,
      'schedule': schedule
    });
    client.publishMessage(topic, MqttQos.atLeastOnce, Uint8Buffer()..addAll(utf8.encode(msg)));
  }

  
}

