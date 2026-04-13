import 'habitat_obj.dart';

enum GrowthStage {
  seed,
  sapling,
  mature,
  ready,
}

extension GrowthStageX on GrowthStage {
  String get label {
    switch (this) {
      case GrowthStage.seed:
        return "Seed";
      case GrowthStage.sapling:
        return "Sapling";
      case GrowthStage.mature:
        return "Mature";
      case GrowthStage.ready:
        return "Ready";
    }
  }
}

class GrowthSpec {
  final Duration totalDuration;

  final double saplingStart;
  final double matureStart;
  final double readyStart;

  const GrowthSpec({
    required this.totalDuration,
    required this.saplingStart,
    required this.matureStart,
    required this.readyStart,
  });
}


const Map<String, GrowthSpec> growthSpecs = {
  'Basil': GrowthSpec(
    totalDuration: Duration(minutes: 1),
    saplingStart: 0.25,
    matureStart: 0.6,
    readyStart: 1.0,
  ),
  'Broccoli': GrowthSpec(
    totalDuration: Duration(minutes: 1),
    saplingStart: 0.2,
    matureStart: 0.55,
    readyStart: 1.0,
  ),
  'Kale': GrowthSpec(
    totalDuration: Duration(minutes: 1),
    saplingStart: 0.2,
    matureStart: 0.55,
    readyStart: 1.0,
  ),
  'Sunflower': GrowthSpec(
    totalDuration: Duration(minutes: 1),
    saplingStart: 0.2,
    matureStart: 0.55,
    readyStart: 1.0,
  ),
  'Radish': GrowthSpec(
    totalDuration: Duration(minutes: 1),
    saplingStart: 0.2,
    matureStart: 0.55,
    readyStart: 1.0,
  ),
  'Pea': GrowthSpec(
    totalDuration: Duration(minutes: 1),
    saplingStart: 0.2,
    matureStart: 0.55,
    readyStart: 1.0,
  ),
  'Arugula': GrowthSpec(
    totalDuration: Duration(minutes: 1),
    saplingStart: 0.2,
    matureStart: 0.55,
    readyStart: 1.0,
  ),
};

const Duration realGrowthDuration = Duration(days: 7);
const Duration demoGrowthDuration = Duration(minutes: 1);

double calculateGrowthProgress(DateTime createdAt, GrowthSpec spec) {
  final elapsed = DateTime.now().difference(createdAt);
  final progress =
      elapsed.inMilliseconds / spec.totalDuration.inMilliseconds;
  return progress.clamp(0.0, 1.0);
}

double getHabitatProgress(Habitat habitat) {
  final spec = growthSpecs[habitat.greenType]!;
  return calculateGrowthProgress(habitat.createdAt, spec);
}


GrowthStage getGrowthStage(Habitat habitat) {
  final spec = growthSpecs[habitat.greenType]!;

  final progress =
      calculateGrowthProgress(habitat.createdAt, spec);

  if (progress >= spec.readyStart) {
    return GrowthStage.ready;
  } else if (progress >= spec.matureStart) {
    return GrowthStage.mature;
  } else if (progress >= spec.saplingStart) {
    return GrowthStage.sapling;
  } else {
    return GrowthStage.seed;
  }
}

bool isHabitatReady(Habitat habitat) {
  if (habitat.harvested) return false;
  return getGrowthStage(habitat) == GrowthStage.ready;
}

