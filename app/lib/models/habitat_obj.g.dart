// GENERATED CODE - DO NOT MODIFY BY HAND

part of 'habitat_obj.dart';

// **************************************************************************
// TypeAdapterGenerator
// **************************************************************************

class HabitatAdapter extends TypeAdapter<Habitat> {
  @override
  final int typeId = 0;

  @override
  Habitat read(BinaryReader reader) {
    final numOfFields = reader.readByte();
    final fields = <int, dynamic>{
      for (int i = 0; i < numOfFields; i++) reader.readByte(): reader.read(),
    };
    return Habitat(
      id: fields[0] as String,
      name: fields[1] as String,
      greenType: fields[2] as String,
      tempTarget: fields[3] as int,
      humidityTarget: fields[4] as int,
      lightStartMs: fields[5] as int,
      lightDurationMs: fields[6] as int,
      lightIntervalMs: fields[7] as int,
      waterStartMs: fields[8] as int,
      waterDurationMs: fields[9] as int,
      waterIntervalMs: fields[10] as int,
    );
  }

  @override
  void write(BinaryWriter writer, Habitat obj) {
    writer
      ..writeByte(11)
      ..writeByte(0)
      ..write(obj.id)
      ..writeByte(1)
      ..write(obj.name)
      ..writeByte(2)
      ..write(obj.greenType)
      ..writeByte(3)
      ..write(obj.tempTarget)
      ..writeByte(4)
      ..write(obj.humidityTarget)
      ..writeByte(5)
      ..write(obj.lightStartMs)
      ..writeByte(6)
      ..write(obj.lightDurationMs)
      ..writeByte(7)
      ..write(obj.lightIntervalMs)
      ..writeByte(8)
      ..write(obj.waterStartMs)
      ..writeByte(9)
      ..write(obj.waterDurationMs)
      ..writeByte(10)
      ..write(obj.waterIntervalMs);
  }

  @override
  int get hashCode => typeId.hashCode;

  @override
  bool operator ==(Object other) =>
      identical(this, other) ||
      other is HabitatAdapter &&
          runtimeType == other.runtimeType &&
          typeId == other.typeId;
}
