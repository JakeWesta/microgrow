// GENERATED CODE - DO NOT MODIFY BY HAND

part of 'sensor_history_obj.dart';

// **************************************************************************
// TypeAdapterGenerator
// **************************************************************************

class SensorHistoryAdapter extends TypeAdapter<SensorHistory> {
  @override
  final int typeId = 3;

  @override
  SensorHistory read(BinaryReader reader) {
    final numOfFields = reader.readByte();
    final fields = <int, dynamic>{
      for (int i = 0; i < numOfFields; i++) reader.readByte(): reader.read(),
    };
    return SensorHistory(
      temp: fields[0] as double,
      humidity: fields[1] as double,
      timestamp: fields[2] as DateTime,
    );
  }

  @override
  void write(BinaryWriter writer, SensorHistory obj) {
    writer
      ..writeByte(3)
      ..writeByte(0)
      ..write(obj.temp)
      ..writeByte(1)
      ..write(obj.humidity)
      ..writeByte(2)
      ..write(obj.timestamp);
  }

  @override
  int get hashCode => typeId.hashCode;

  @override
  bool operator ==(Object other) =>
      identical(this, other) ||
      other is SensorHistoryAdapter &&
          runtimeType == other.runtimeType &&
          typeId == other.typeId;
}
