const Duration realGrowthDuration = Duration(days: 7);
const Duration demoGrowthDuration = Duration(minutes: 1);

double calculateGrowthProgress(DateTime createdAt) {
  final elapsed = DateTime.now().difference(createdAt);
  final progress =
      elapsed.inMilliseconds / demoGrowthDuration.inMilliseconds;
  return progress.clamp(0.0, 1.0);
}

bool isHabitatReady(DateTime createdAt, bool harvested) {
  if (harvested) return false;
  return DateTime.now().difference(createdAt) >= demoGrowthDuration;
}
