import 'package:mqtt_client/mqtt_client.dart';
import 'package:mqtt_client/mqtt_browser_client.dart';

Future<MqttClient> connect() async {
  const host = 'broker.emqx.io';
  const port = 8083;
  const path = '/mqtt';
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

  return client;
}
