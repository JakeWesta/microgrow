import 'dart:convert';

import 'package:mqtt_client/mqtt_client.dart';
import 'package:mqtt_client/mqtt_server_client.dart';
import 'package:typed_data/typed_data.dart';

import '../models/growth_config.dart';
import '../ui/add_habitat_screen.dart';

class MqttService {
  static const host = 'broker.emqx.io';
  static const port = 1883;

  static MqttServerClient? client;
  static bool _updatesListenerAttached = false;

  static final Set<String> _subscribedSensorHabitats = {};
  static final Set<String> _subscribedPulseHabitats = {};
  static final Set<String> _subscribedHistoryHabitats = {};

  static final Map<String, void Function(String topic, String payload)>
      _sensorHandlers = {};

  static final Map<String, void Function()> _pulseHandlers = {};

  static final Map<String, void Function(String payload)> _historyHandlers = {};

  static Future<MqttServerClient> connect() async {
    if (client != null &&
        client!.connectionStatus?.state == MqttConnectionState.connected) {
      return client!;
    }

    client = MqttServerClient(
      host,
      'flutter_native_${DateTime.now().millisecondsSinceEpoch}',
    );

    client!.port = port;
    client!.keepAlivePeriod = 30;
    client!.setProtocolV311();
    client!.logging(on: true);

    client!.onConnected = () => print('Connected');
    client!.onDisconnected = () => print('Disconnected');
    client!.onSubscribed = (t) => print('Subscribed: $t');

    final conn = MqttConnectMessage()
        .startClean()
        .withWillTopic('app/status')
        .withWillMessage('offline')
        .withWillQos(MqttQos.atLeastOnce);

    client!.connectionMessage = conn;

    print('Connecting to $host:$port...');
    await client!.connect();

    if (client!.connectionStatus?.state != MqttConnectionState.connected) {
      final rc = client!.connectionStatus?.returnCode;
      client!.disconnect();
      client = null;
      throw Exception('MQTT not connected. CONNACK=$rc');
    }

    _attachUpdatesListener();
    return client!;
  }

  static void _attachUpdatesListener() {
    if (_updatesListenerAttached || client == null) return;
    _updatesListenerAttached = true;

    client!.updates?.listen(
      (List<MqttReceivedMessage<MqttMessage?>>? event) {
        if (event == null || event.isEmpty) return;

        final rec = event[0];
        final message = rec.payload as MqttPublishMessage;
        final payload = MqttPublishPayload.bytesToStringAsString(
          message.payload.message,
        );
        final topic = rec.topic;

        for (final entry in _sensorHandlers.entries) {
          final habitatId = entry.key;
          final prefix = 'microgrow/$habitatId/';
          if (!topic.startsWith(prefix)) continue;

          final suffix = topic.substring(prefix.length);
          if (suffix == 'light' ||
              suffix == 'humidity' ||
              suffix == 'temp' ||
              suffix == 'water') {
            entry.value(topic, payload);
            return;
          }
        }

        for (final entry in _pulseHandlers.entries) {
          final expectedTopic = 'microgrow/${entry.key}/pulse';
          if (topic == expectedTopic) {
            entry.value();
            return;
          }
        }

        
        for (final entry in _historyHandlers.entries) {
          final expectedTopic = 'microgrow/${entry.key}/history'; 
          if (topic == expectedTopic) {
            entry.value(payload);
            return;
          }
        }
      },
    );
  }

  static Future<void> setupHabitat({
    required String habitatId,
    required HabitatConfig config,
  }) async {
    final mqtt = await connect();
    final topic = 'microgrow/$habitatId/init';

    bool blackout = false;

    if (config.blackoutDuration > 0){
        blackout = true;
    }

    final msg = jsonEncode({
      "greenType": config.greenType,
      "targetTemp": config.tempTarget,
      "targetHumidity": config.humidityTarget,
      "blackout": blackout,
      "light": {
      "startSec": config.lightStartSec,
      "durationSec": config.lightDurationSec,
      "intervalSec": config.lightIntervalSec
      },
      "water": {
      "startSec": config.waterStartSec,
      "durationSec": config.waterDurationSec,
      "intervalSec": config.waterIntervalSec
      }
      });
    

    mqtt.publishMessage(
      topic,
      MqttQos.atLeastOnce,
      Uint8Buffer()..addAll(utf8.encode(msg)),
    );
  }

  static Future<void> deleteHabitat({required String habitatId}) async {
    final mqtt = await connect();
    final topic = 'microgrow/$habitatId/delete';

    final msg = jsonEncode({});

    mqtt.publishMessage(
      topic,
      MqttQos.atLeastOnce,
      Uint8Buffer()..addAll(utf8.encode(msg)),
    );
  }

  static Future<void> blackoutEnd({required String habitatId}) async {
    final mqtt = await connect();
    final topic = 'microgrow/$habitatId/blackout';

    final msg = jsonEncode({'blackout': false});

    mqtt.publishMessage(
      topic,
      MqttQos.atLeastOnce,
      Uint8Buffer()..addAll(utf8.encode(msg)),
    );
  }

  static Future<void> actuatorPublish({
    required String habitatId,
    required String actuatorName,
    required int val,
  }) async {
    final start = DateTime.now();
    final mqtt = await connect();
    print('connect wait: ${DateTime.now().difference(start).inMilliseconds} ms');

    final topic = "microgrow/$habitatId/override";

    bool enable = false;

    final Map<String, int> options = {
      "water": 1,
      "light": 2,
      "fan": 0,
      "mister": 3,
    };

    if (val == 1) {
      enable = true;
    }

    final msg = jsonEncode({
      'actuator': options[actuatorName],
      'enable': enable,
    });

    mqtt.publishMessage(
      topic,
      MqttQos.atLeastOnce,
      Uint8Buffer()..addAll(utf8.encode(msg)),
    );
    print('published override at ${DateTime.now()}');
  }

  static Future<void> ledPublish({
    required String habitatId,
    required String actuatorName,
    required int val,
    int? r,
    int? g,
    int? b,
  }) async {
    final mqtt = await connect();
    final topic = "microgrow/$habitatId/override";

    final Map<String, int> options = {
      "water": 1,
      "light": 2,
      "fan": 0,
      "mister": 3,
    };

    bool enable = false;

    if (val == 1) {
      enable = true;
    }

    final msg = jsonEncode({
      'actuator': options[actuatorName],
      'enable': enable,
      "color": {"r": r ?? 0, "g": g ?? 0, "b": b ?? 0}
    });

    mqtt.publishMessage(
      topic,
      MqttQos.atLeastOnce,
      Uint8Buffer()..addAll(utf8.encode(msg)),
    );
  }

  static Future<void> sensorSubscribe({
    required String habitatId,
    required void Function(String topic, String payload) onMessage,
  }) async {
    final mqtt = await connect();
    _sensorHandlers[habitatId] = onMessage;

    if (_subscribedSensorHabitats.contains(habitatId)) return;
    _subscribedSensorHabitats.add(habitatId);

    final topics = [
      'microgrow/$habitatId/light',
      'microgrow/$habitatId/humidity',
      'microgrow/$habitatId/temp',
      'microgrow/$habitatId/water',
    ];

    for (final t in topics) {
      mqtt.subscribe(t, MqttQos.atLeastOnce);
    }
  }

  static Future<void> pulseSubscribe({
    required String habitatId,
    required void Function() onPulse,
  }) async {
    final mqtt = await connect();
    _pulseHandlers[habitatId] = onPulse;

    if (_subscribedPulseHabitats.contains(habitatId)) return;
    _subscribedPulseHabitats.add(habitatId);

    final topic = 'microgrow/$habitatId/pulse';
    mqtt.subscribe(topic, MqttQos.atLeastOnce);
  }

 static Future<void> requestHistory({
  required String habitatId,
  required void Function(String payload) onMessage,
}) async {
  final mqtt = await connect();

  if (!_subscribedHistoryHabitats.contains(habitatId)) {
    _subscribedHistoryHabitats.add(habitatId);
    mqtt.subscribe('microgrow/$habitatId/history', MqttQos.atLeastOnce);
  }

  _historyHandlers[habitatId] = onMessage;

  mqtt.publishMessage(
    'microgrow/$habitatId/refresh',
    MqttQos.atLeastOnce,
    Uint8Buffer()..addAll(utf8.encode(jsonEncode({}))),
  );
}

  static Future<void> publishGrowthStage({
    required String habitatId,
    required GrowthStage stage,
  }) async {
    final mqtt = await connect();
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
        stageValue = 3;
        break;
      default:
        stageValue = 0;
        break;
    }

    final msg = jsonEncode({'growthStage': stageValue});
    mqtt.publishMessage(
      topic,
      MqttQos.atLeastOnce,
      Uint8Buffer()..addAll(utf8.encode(msg)),
    );
  }
}