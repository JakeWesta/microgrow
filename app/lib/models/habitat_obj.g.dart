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
      lastSensorValue: fields[3] as int?,
    );
  }

  @override
  void write(BinaryWriter writer, Habitat obj) {
    writer
      ..writeByte(4)
      ..writeByte(0)
      ..write(obj.id)
      ..writeByte(1)
      ..write(obj.name)
      ..writeByte(2)
      ..write(obj.greenType)
      ..writeByte(3)
      ..write(obj.lastSensorValue);
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
