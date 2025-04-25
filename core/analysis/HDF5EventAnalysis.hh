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
#include <vector>
#include <string>
#include <map>

class HDF5EventAnalysis {
    public:
    /// Singleton instance accessor for each thread
    static HDF5EventAnalysis* GetInstance();
    
    /// Begin of run action (initialization of data structures)
    void BeginOfRun(const G4Run* runPtr, G4bool isMaster);
    
    /// End of event action (handling and saving event data)
    void EndOfEventAction(const G4Event* evt);
    
    /// Define dataset structure for events
    static void DefineDatasetStructure(const std::string& datasetName);

    static void DefineDataset(const G4String& datasetName, const G4String& hitsCollectionName);
    
    /// Write event data to HDF5 file
    void WriteEventData();


    
    private:
    HDF5EventAnalysis();
    ~HDF5EventAnalysis();
    
    /// Fill event data from hits collection
    void FillEventData(const G4Event* evt, VoxelHitsCollection* hitsColl);
    
    
    /// Clear event data collections
    void ClearEventData();
    
    struct EventDataCollection {
        std::vector<int> voxelIdX, voxelIdY, voxelIdZ;
        std::vector<double> voxelDose;
        // Add more vectors according to data required
    };

    struct DatasetDef {
        G4String name;
        std::vector<G4String> hc_names;
    };
    
    static G4Cache<std::vector<DatasetDef>> m_dataset_defs;

    /// Thread-local singleton instance pointer
    G4ThreadLocal static HDF5EventAnalysis* fInstance;
    
    /// Cache for event data collections
    
    std::string m_currentRunGroup;
    static G4Cache<std::map<std::string, EventDataCollection>> m_event_collections;
};

#endif // HDF5_EVENT_ANALYSIS_HH
