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

  bool operator[](AnalysisFlag flag) const { return m_bitset_flags.test(static_cast<size_t>(flag)); }

  void Set(AnalysisFlag flag, bool value = true) { m_bitset_flags.set(static_cast<size_t>(flag), value); }

  void Reset() { m_bitset_flags.reset(); }

 private:
  std::bitset<static_cast<size_t>(AnalysisFlag::COUNT)> m_bitset_flags;
};

// Singleton that manages the map of tree name → flag set
class AnalysisFlagRegistry {
 public:
  static AnalysisFlagRegistry* Instance();

  void SetFlag(AnalysisFlag flag, bool value) { m_analysis_flags.Set(flag, value); }
  bool IsEnabled(AnalysisFlag flag) const { return m_analysis_flags[flag]; }
  void ResetAll() { m_analysis_flags.Reset(); }
  void PrintAllFlags() const;

 private:
  AnalysisFlagRegistry() = default;
  AnalysisFlags m_analysis_flags;
};

#endif  // ANALYSIS_FLAG_REGISTRY_HH
