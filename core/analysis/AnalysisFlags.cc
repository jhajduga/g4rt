// ========================= AnalysisFlags.cc ========================= //
#include "AnalysisFlags.hh"
#include <iostream>
#include <iomanip>

/**
 * @brief Returns the singleton instance of the AnalysisFlagRegistry.
 *
 * Ensures that only one instance of the registry exists throughout the program's lifetime.
 *
 * @return Pointer to the singleton AnalysisFlagRegistry instance.
 */
AnalysisFlagRegistry* AnalysisFlagRegistry::Instance() {
  static AnalysisFlagRegistry instance;
  return &instance;
}


/**
 * @brief Print the enabled/disabled state of every global analysis flag.
 *
 * Iterates all values of the AnalysisFlag enumeration and writes each flag's
 * name and boolean state ("true"/"false") to standard output in a human-
 * readable, column-aligned format.
 *
 * @note Uses IsEnabled(flag) to determine each flag's state.
 */
void AnalysisFlagRegistry::PrintAllFlags() const {
  std::cout << "[AnalysisFlagRegistry] Active global flags:\n";

  for (size_t i = 0; i < static_cast<size_t>(AnalysisFlag::COUNT); ++i) {
    auto flag = static_cast<AnalysisFlag>(i);
    std::string name;
    switch (flag) {
      case AnalysisFlag::StoreRunInfo:
        name = "StoreRunInfo";
        break;
      case AnalysisFlag::StorePositions:
        name = "StorePositions";
        break;
      case AnalysisFlag::StoreEnergies:
        name = "StoreEnergies";
        break;
      case AnalysisFlag::StoreTracks:
        name = "StoreTracks";
        break;
      case AnalysisFlag::StorePrimaries:
        name = "StorePrimaries";
        break;
      case AnalysisFlag::Voxelized:
        name = "Voxelized";
        break;
      case AnalysisFlag::MinimalMode:
        name = "MinimalMode";
        break;
      default:
        name = "Unknown";
        break;
    }

    std::cout << "  - " << std::setw(16) << std::left << name << ": " << (IsEnabled(flag) ? "true" : "false") << "\n";
  }
}
