#pragma once
#include <cstdint>

namespace sudoku {

constexpr int N = 9;        // lato della griglia
constexpr int CELLS = 81;   // numero di celle

enum class Difficulty : uint8_t { Easy = 0, Medium = 1, Hard = 2 };

// RNG iniettabile: restituisce un intero in [0, maxExclusive).
// Permette test deterministici (LCG nei test, esp_random sul device).
using RandFn = uint32_t (*)(uint32_t maxExclusive);

} // namespace sudoku
