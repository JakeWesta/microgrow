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
      lightDurationSec: fields[6] as int?,
      lightIntervalSec: fields[7] as int?,
      waterStartSec: fields[8] as int,
      waterDurationSec: fields[9] as int?,
      waterIntervalSec: fields[10] as int?,
      createdAt: fields[11] as DateTime?,
      harvested: fields[12] as bool?,
      history: (fields[13] as List?)?.cast<SensorHistory>(),
      decorations: (fields[15] as List?)?.cast<DecorationObj>(),
      blackoutDuration: fields[16] as int?,
      blackoutAcknowledged: fields[17] as bool?,
      lightOverride: fields[18] as bool?,
      fanOverride: fields[19] as bool?,
      misterOverride: fields[20] as bool?,
      slotIndex: fields[21] as int?,
    )..reservoirVolume = fields[14] as int;
  }

  @override
  void write(BinaryWriter writer, Habitat obj) {
    writer
      ..writeByte(22)
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
      ..write(obj.waterIntervalSec)
      ..writeByte(11)
      ..write(obj.createdAt)
      ..writeByte(12)
      ..write(obj.harvested)
      ..writeByte(13)
      ..write(obj.history)
      ..writeByte(14)
      ..write(obj.reservoirVolume)
      ..writeByte(15)
      ..write(obj.decorations)
      ..writeByte(16)
      ..write(obj.blackoutDuration)
      ..writeByte(17)
      ..write(obj.blackoutAcknowledged)
      ..writeByte(18)
      ..write(obj.lightOverride)
      ..writeByte(19)
      ..write(obj.fanOverride)
      ..writeByte(20)
      ..write(obj.misterOverride)
      ..writeByte(21)
      ..write(obj.slotIndex);
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
