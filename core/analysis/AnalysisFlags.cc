// ========================= AnalysisFlags.cc ========================= //
#include "AnalysisFlags.hh"
#include <iostream>
#include <iomanip>

AnalysisFlagRegistry* AnalysisFlagRegistry::Instance() {
  static AnalysisFlagRegistry instance;
  return &instance;
}


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
