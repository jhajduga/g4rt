#ifndef ANALYSIS_FLAG_REGISTRY_HH
#define ANALYSIS_FLAG_REGISTRY_HH

#include <bitset>
#include <map>
#include <string>

// Define all available analysis flags
enum class AnalysisFlag {
  StoreRunInfo,
  StorePositions,
  StoreEnergies,
  StoreTracks,
  StorePrimaries,
  Voxelized,
  MinimalMode,
  COUNT  // Number of flags (for std::bitset size)
};

// Container class for storing active flags for a given tree
class AnalysisFlags {
 public:
  AnalysisFlags() = default;

  bool operator[](AnalysisFlag flag) const { return flags.test(static_cast<size_t>(flag)); }

  void Set(AnalysisFlag flag, bool value = true) { flags.set(static_cast<size_t>(flag), value); }

  void Reset() { flags.reset(); }

 private:
  std::bitset<static_cast<size_t>(AnalysisFlag::COUNT)> flags;
};

// Singleton that manages the map of tree name → flag set
class AnalysisFlagRegistry {
 public:
  static AnalysisFlagRegistry& Instance();

  void SetFlag(AnalysisFlag flag, bool value = true);
  bool IsEnabled(AnalysisFlag flag) const;
  void ResetAll();
  void PrintAllFlags() const;

 private:
  AnalysisFlagRegistry() = default;
  AnalysisFlags m_flags;
};

#endif  // ANALYSIS_FLAG_REGISTRY_HH
