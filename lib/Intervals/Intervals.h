#pragma once
bool isElapsedMillis(uint32_t startTime, uint32_t interval);
bool isFlashingMillis(uint32_t startTime, uint32_t flashInterval);
uint32_t nextWaterMillis(uint32_t stopTime, uint32_t waterInterval);
// constexpr uint32_t daysToMillis(uint16_t days) {
//     return static_cast<uint32_t>(days) * 24U * 60U * 60U * 1000U;
// }
// constexpr uint64_t hoursToMicros(uint16_t hours) {
//     return static_cast<uint64_t>(hours) * 3600ULL * 1000000ULL;
// }
// constexpr uint32_t minutesToMillis(uint16_t minutes) {
//     return static_cast<uint32_t>(minutes) * 60U * 1000U;
// }
// constexpr uint32_t secondsToMillis(uint16_t seconds) {
//     return static_cast<uint32_t>(seconds) * 1000U;
// }