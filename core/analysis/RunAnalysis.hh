//
// Created by brachwal on 28.04.2020.
//

#ifndef RUN_ANALYSIS_HH
#define RUN_ANALYSIS_HH

#include "globals.hh"
#include <vector>
#include "G4ThreeVector.hh"
#include "G4SystemOfUnits.hh"
#include "G4Cache.hh"
#include "VoxelHit.hh"
#include "Types.hh"

class ControlPoint;
class CsvRunAnalysis;
class NTupleRunAnalysis;
class G4Event;
class G4Run;

typedef std::map<Scoring::Type, std::map<std::size_t, VoxelHit>> ScoringMap;

class RunAnalysis {

  private:
    ///
    RunAnalysis();

    /**
 * @brief Destructor for RunAnalysis.
 *
 * Releases RunAnalysis-owned resources. The destructor is private and invoked
 * only when the singleton instance is destroyed.
 */
    ~RunAnalysis() = default;

    /**
 * @brief Deleted copy constructor to prevent copying of the RunAnalysis singleton.
 */
    RunAnalysis(const RunAnalysis &) = delete;
    /**
 * @brief Deleted copy assignment operator to prevent copying of RunAnalysis instances.
 */
RunAnalysis &operator=(const RunAnalysis &) = delete;
    /**
 * @brief Move constructor is deleted to enforce singleton pattern.
 */
RunAnalysis(RunAnalysis &&) = delete;
    /**
 * @brief Move assignment operator is deleted to enforce singleton pattern.
 *
 * Prevents moving assignment of RunAnalysis instances to ensure only one instance exists.
 */
RunAnalysis &operator=(RunAnalysis &&) = delete;

    /// Many HitsCollections can be associated to given collection name 
    // (e.g. when many sensitive detectors constituting a single detection unit)
    static std::map<G4String,std::vector<G4String>> m_run_collection;

    ///
    CsvRunAnalysis* m_csv_run_analysis = nullptr;
    NTupleRunAnalysis* m_ntuple_run_analysis = nullptr;

    ///
    bool m_is_initialized = false;

    ///
    ControlPoint* m_current_cp = nullptr;

  public:
    ///
    static RunAnalysis* GetInstance();

    ///
    void BeginOfRun(const G4Run* runPtr, G4bool isMaster);

    ///
    void EndOfRun(const G4Run* runPtr);

    ///
    void EndOfEventAction(const G4Event *evt);

};
#endif //RUN_ANALYSIS_HH
