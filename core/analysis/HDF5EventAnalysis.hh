// HDF5EventAnalysis.hh
#ifndef HDF5_EVENT_ANALYSIS_HH
#define HDF5_EVENT_ANALYSIS_HH

#include "globals.hh"
#include "G4Event.hh"
#include "G4Run.hh"
#include "VoxelHit.hh"
#include "tls.hh"
#include "G4Cache.hh"
#include "HDF5Manager.hh"
#include <map>
#include <string>
#include <vector>

class HDF5EventAnalysis {
public:
    /// Thread-local singleton accessor
    static HDF5EventAnalysis* GetInstance();

    /// Register datasetName with its corresponding Geant4 hits collection name(s)
    static void DefineDataset(const G4String& datasetName,
                              const G4String& hitsCollName);

    /// Called at BeginOfRun: master thread opens file, creates group and chunked datasets
    void BeginOfRun(const G4Run* runPtr, G4bool isMaster);

    /// Called at EndOfEventAction: collects data and appends to HDF5 (MT-safe)
    void EndOfEventAction(const G4Event* evt);

private:
    HDF5EventAnalysis();
    ~HDF5EventAnalysis();

    // Definition of one dataset and its associated hits collections
    struct DatasetDef {
        G4String name;
        std::vector<G4String> hc_names;
    };

    // Global (shared) list of all dataset definitions
    static std::vector<DatasetDef> m_dataset_defs;
    // Protects m_dataset_defs across threads
    static std::mutex m_defs_mutex;

    // Data storage per thread: map from datasetName to collected hits
    struct EventDataCollection {
        std::vector<int> voxelIdX, voxelIdY, voxelIdZ;
        std::vector<double> voxelDose;
    };
    G4Cache<std::map<G4String, EventDataCollection>> m_event_collections;

    // Current HDF5 group path for this run, e.g. "Run_0"
    G4String m_currentRunGroup;

    /// Fill thread-local cache for one datasetName from one hits collection
    void FillEventData(const G4String& datasetName,
                       VoxelHitsCollection* hitsColl);

    /// Clear thread-local cache for one datasetName
    void ClearEventData(const G4String& datasetName);
};

#endif // HDF5_EVENT_ANALYSIS_HH
