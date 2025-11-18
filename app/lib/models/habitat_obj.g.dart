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
      lightStartSec: fields[5] as int,
      lightDurationSec: fields[6] as int,
      lightIntervalSec: fields[7] as int,
      waterStartSec: fields[8] as int,
      waterDurationSec: fields[9] as int,
      waterIntervalSec: fields[10] as int,
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
      ..write(obj.lightStartSec)
      ..writeByte(6)
      ..write(obj.lightDurationSec)
      ..writeByte(7)
      ..write(obj.lightIntervalSec)
      ..writeByte(8)
      ..write(obj.waterStartSec)
      ..writeByte(9)
      ..write(obj.waterDurationSec)
      ..writeByte(10)
      ..write(obj.waterIntervalSec);
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
