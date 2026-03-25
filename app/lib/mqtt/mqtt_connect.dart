import 'package:mqtt_client/mqtt_client.dart';
import 'package:mqtt_client/mqtt_server_client.dart';
import 'dart:convert';
import 'package:typed_data/typed_data.dart';
import '../ui/add_habitat_screen.dart';
import '../models/growth_config.dart';

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

    //client.subscribe('microgrow/init', MqttQos.atLeastOnce);

    return client;

  }

  static Future<void> setupHabitat({
    required String habitatId,
    required HabitatConfig config,
  }) async {
    final client = await connect();
    final topic = 'microgrow/$habitatId/init';

    final msg2 = jsonEncode({
      "light": {
        "startTimeSec": config.lightStartSec,
        "durationSec": config.lightDurationSec,
        "intervalSec": config.lightIntervalSec,
      },
      "water": {
        "startTimeSec": config.waterStartSec,
        "durationSec": config.waterDurationSec,
        "intervalSec": config.waterIntervalSec,
      },
      "blackout" : config.blackoutDuration
    });

    final msg1 = jsonEncode({
      "greenType": config.greenType,
      "target": {
        "temp": config.tempTarget,
        "humidity": config.humidityTarget,
      }
    });

    client.publishMessage(
      topic,
      MqttQos.atLeastOnce,
      Uint8Buffer()..addAll(utf8.encode(msg1)),
    );

    await Future.delayed(const Duration(milliseconds: 200));

    client.publishMessage(
      topic,
      MqttQos.atLeastOnce,
      Uint8Buffer()..addAll(utf8.encode(msg2)),
    );
  }

  static Future<void> deleteHabitat({required String habitatId}) async {
    final client = await connect();
    final topic = 'microgrow/$habitatId/delete';

    final msg = jsonEncode({
      'delete': 0
    });

    client.publishMessage(topic, MqttQos.atLeastOnce, Uint8Buffer()..addAll(utf8.encode(msg)));
  }

  static Future<void> blackoutEnd({required String habitatId}) async {
    final client = await connect();
    final topic = 'microgrow/$habitatId/blackout';

    final msg = jsonEncode({
      'blackout': 0
    });

    client.publishMessage(topic, MqttQos.atLeastOnce, Uint8Buffer()..addAll(utf8.encode(msg)), retain: true);
  }

  static Future<void> actuatorPublish({required String habitatId,  required String actuatorName, 
  required int val, int? r, int? g, int? b}) async {
    final client = await connect();
    final topic = "microgrow/$habitatId/override";

    final Map<String, int> options = {"water": 1, "light": 2, "fan": 0, "mister": 3};

    final msg = jsonEncode({
      'actuator': options[actuatorName],
      'enable': val,
      'value': 1,
      'r': r ?? 0,
      'g': g ?? 0,
      'b': b ?? 0
    });

    client.publishMessage(topic, MqttQos.atLeastOnce, Uint8Buffer()..addAll(utf8.encode(msg)));
  }

  static Future<void> sensorSubscribe({required String habitatId, required void Function(String topic, String payload) onMessage}) async {
    final client = await connect();
    final topics = ['microgrow/$habitatId/light', 'microgrow/$habitatId/humidity', 'microgrow/$habitatId/temp', 'microgrow/$habitatId/water'];

    for (final t in topics) {
      client.subscribe(t, MqttQos.atLeastOnce);
    }

    client.updates?.listen(
      (List<MqttReceivedMessage<MqttMessage?>>? event) {
        if (event == null || event.isEmpty) return;

        final rec = event[0];
        final message = rec.payload as MqttPublishMessage;
        final payload = MqttPublishPayload.bytesToStringAsString(
          message.payload.message,
        );

        final topic = rec.topic; 

        onMessage(topic, payload);
      },
    );
  }

  static Future<void> requestHistory({
    required String habitatId,
    required void Function(String payload) onMessage, 
  }) async {
    final client = await connect();
    final topic = 'microgrow/$habitatId/refresh';

    client.subscribe(topic, MqttQos.atLeastOnce);

    client.updates?.listen(
      (List<MqttReceivedMessage<MqttMessage?>>? event) {
        if (event == null || event.isEmpty) return;

        final rec = event[0];
        if (rec.topic == topic) {
          final message = rec.payload as MqttPublishMessage;
          final payload = MqttPublishPayload.bytesToStringAsString(
            message.payload.message,
          );

          onMessage(payload);
        }
      },
    );

    final msg = jsonEncode({'meow': 1});
    client.publishMessage(
      topic,
      MqttQos.atLeastOnce,
      Uint8Buffer()..addAll(utf8.encode(msg)),
    );
  }

  static Future<void> publishGrowthStage({
    required String habitatId,
    required GrowthStage stage,
  }) async {
    final client = await connect();
    final topic = 'microgrow/$habitatId/growth';
    int stageValue = 0; 

    switch (stage) {
      case GrowthStage.sapling:
        stageValue = 1;
        break;
      case GrowthStage.mature:
        stageValue = 2;
        break;
      case GrowthStage.ready:
        stageValue = 2;
        break;
      default:
        stageValue = 0;
        break;
    }

    final msg = jsonEncode({'growthStage': stageValue});
    client.publishMessage(
      topic,
      MqttQos.atLeastOnce,
      Uint8Buffer()..addAll(utf8.encode(msg)),
    );
  }



}

