// GENERATED CODE - DO NOT MODIFY BY HAND

part of 'decoration_obj.dart';

// **************************************************************************
// TypeAdapterGenerator
// **************************************************************************

class DecorationObjAdapter extends TypeAdapter<DecorationObj> {
  @override
  final int typeId = 4;

  @override
  DecorationObj read(BinaryReader reader) {
    final numOfFields = reader.readByte();
    final fields = <int, dynamic>{
      for (int i = 0; i < numOfFields; i++) reader.readByte(): reader.read(),
    };
    return DecorationObj(
      type: fields[0] as String,
      x: fields[1] as double,
      y: fields[2] as double,
    );
  }

  @override
  void write(BinaryWriter writer, DecorationObj obj) {
    writer
      ..writeByte(3)
      ..writeByte(0)
      ..write(obj.type)
      ..writeByte(1)
      ..write(obj.x)
      ..writeByte(2)
      ..write(obj.y);
  }

  @override
  int get hashCode => typeId.hashCode;

  @override
  bool operator ==(Object other) =>
      identical(this, other) ||
      other is DecorationObjAdapter &&
          runtimeType == other.runtimeType &&
          typeId == other.typeId;
}
