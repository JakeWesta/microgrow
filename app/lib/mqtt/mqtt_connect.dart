import 'package:mqtt_client/mqtt_client.dart';
import 'package:mqtt_client/mqtt_server_client.dart';
import 'dart:convert';
import 'package:typed_data/typed_data.dart';

// TODO: Implement class to support MQTT connection and 
// subcribing to topics 

class MqttService {
  static const host = 'broker.emqx.io';
  static const port = 1883;
  static MqttServerClient? client;
  
  
  static Future<MqttServerClient> connect() async {

    final client = MqttServerClient(
      host, 'flutter_native_${DateTime.now().millisecondsSinceEpoch}'
    );
    client.port = port;
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

    print('Connecting to $host:$port...');
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

  static Future<void> sensorSubscribe({required String habitatId, required void Function(String payload) onMessage}) async {
    final client = await connect();
    final topic = 'microgrow/$habitatId/sensor';

    client.subscribe(topic, MqttQos.atLeastOnce);

     client.updates?.listen(
      (List<MqttReceivedMessage<MqttMessage?>>? event) {
          if (event == null || event.isEmpty) return;
          final recMessage = event[0].payload as MqttPublishMessage;
          final payload = MqttPublishPayload.bytesToStringAsString(recMessage.payload.message);
          onMessage(payload);
          }
      );
  }


}

