// ========================= AnalysisFlags.hh ========================= //
#ifndef ANALYSIS_FLAGS_HH
#define ANALYSIS_FLAGS_HH

#include <bitset>
#include <map>
#include <string>

// Semantic flags that control what kind of data will be stored in a given TTree
// Can be extended as needed — order matters only for display
enum class AnalysisFlag {
  StorePositions,
  StoreEnergies,
  StoreTracks,
  StorePrimaries,
  StoreSecondaries,
  StoreTiming,
  MinimalMode,
  DetailedProcessInfo,
  VerboseLogging,
  COUNT // helper for bitset size
};

// Class that stores which flags are active for a given analysis context
class AnalysisFlags {
public:
  AnalysisFlags() = default;

  // Query a flag
  bool operator[](AnalysisFlag flag) const {
    return flags.test(static_cast<size_t>(flag));
  }

  // Set or unset a flag
  void Set(AnalysisFlag flag, bool value = true) {
    flags.set(static_cast<size_t>(flag), value);
  }

  // Reset all flags
  void Reset() { flags.reset(); }

  // Utility logic for derived meanings
  bool IsMinimal() const { return (*this)[AnalysisFlag::MinimalMode]; }

  bool ShouldStoreAnySpatial() const {
    return (*this)[AnalysisFlag::StorePositions] ||
           (*this)[AnalysisFlag::StoreTracks];
  }

  bool RequiresTiming() const {
    return (*this)[AnalysisFlag::StoreTiming] && !IsMinimal();
  }

private:
  std::bitset<static_cast<size_t>(AnalysisFlag::COUNT)> flags;
};

// Global registry for flags assigned per TTree name
inline std::map<std::string, AnalysisFlags> g_tree_flag_map;

// Helper for external configuration
inline void SetAnalysisFlag(const std::string &treeName, AnalysisFlag flag,
                            bool value = true) {
  g_tree_flag_map[treeName].Set(flag, value);
}

#endif // ANALYSIS_FLAGS_HH
