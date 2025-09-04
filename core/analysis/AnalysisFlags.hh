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
  /**
 * @brief Constructs an AnalysisFlags object with all flags disabled.
 */
AnalysisFlags() = default;

  /**
 * @brief Checks whether a specific analysis flag is enabled.
 *
 * @param flag The analysis flag to query.
 * @return true if the flag is enabled; false otherwise.
 */
bool operator[](AnalysisFlag flag) const { return m_bitset_flags.test(static_cast<size_t>(flag)); }

  /**
 * @brief Enable or disable a specific analysis flag in this container.
 *
 * Sets the bit corresponding to the provided AnalysisFlag to the given boolean
 * value (defaults to true).
 *
 * @param flag The AnalysisFlag to modify.
 * @param value True to enable the flag, false to disable it. Defaults to true.
 */
void Set(AnalysisFlag flag, bool value = true) { m_bitset_flags.set(static_cast<size_t>(flag), value); }

  /**
 * @brief Clears all analysis flags, disabling every flag in the set.
 */
void Reset() { m_bitset_flags.reset(); }

 private:
  std::bitset<static_cast<size_t>(AnalysisFlag::COUNT)> m_bitset_flags;
};

// Singleton that manages the map of tree name → flag set
class AnalysisFlagRegistry {
 public:
  static AnalysisFlagRegistry* Instance();

  /**
 * @brief Sets the specified analysis flag to the given state.
 *
 * @param flag The analysis flag to modify.
 * @param value If true, enables the flag; if false, disables it.
 */
void SetFlag(AnalysisFlag flag, bool value) { m_analysis_flags.Set(flag, value); }
  /**
 * @brief Checks if a specific analysis flag is enabled.
 *
 * @param flag The analysis flag to query.
 * @return true if the flag is enabled; false otherwise.
 */
bool IsEnabled(AnalysisFlag flag) const { return m_analysis_flags[flag]; }
  /**
 * @brief Clears all analysis flags in the registry.
 *
 * Resets the state of every analysis flag to disabled within the singleton registry.
 */
void ResetAll() { m_analysis_flags.Reset(); }
  void PrintAllFlags() const;

 private:
  /**
 * @brief Default (private) constructor.
 *
 * Creates an AnalysisFlagRegistry instance. The constructor is private to enforce
 * singleton usage; callers should obtain the registry via AnalysisFlagRegistry::Instance().
 */
AnalysisFlagRegistry() = default;
  AnalysisFlags m_analysis_flags;
};

#endif  // ANALYSIS_FLAG_REGISTRY_HH
