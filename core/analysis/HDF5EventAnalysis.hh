#ifndef HDF5EVENTANALISYS_HH
#define HDF5EVENTANALISYS_HH
#include "globals.hh"
#include "VoxelHit.hh"
#include <string>
#include <vector>
#include <memory>
#include <highfive/H5File.hpp>

class G4Event;
class G4Run;



/// \brief Minimalistic HDF5-based event analysis using HighFive.
///        Allows registration of multiple hits collection names to record.
class HDF5EventAnalysis {
    public:
        /// Get thread-local singleton instance
        static HDF5EventAnalysis* GetInstance();
    
        /// Register a hits collection name to be recorded
        static void DefineHitsCollection(const G4String& hcName);
    
        /// Called at the beginning of the run in each thread
        void BeginOfRun(const G4Run* run, bool isMaster);
    
        /// Called at the end of each event to collect and write data
        void EndOfEventAction(const G4Event* event);
    
        /// Called at the end of the run to close the file
        void EndOfRun(const G4Run* run);
    
        static G4Cache<std::vector<G4String>> m_hc_names;
    private:
        HDF5EventAnalysis();
        ~HDF5EventAnalysis();
        HDF5EventAnalysis(const HDF5EventAnalysis&) = delete;
        HDF5EventAnalysis& operator=(const HDF5EventAnalysis&) = delete;
    
        /// Thread-local instance pointer
        static G4ThreadLocal HDF5EventAnalysis* fInstance;
    
        /// Registered hits collection names cache
    
        /// Underlying HDF5 file handle
        std::unique_ptr<HighFive::File> m_file;
    
        /// Datasets for minimalistic columns
        HighFive::DataSet m_ds_eventId;
        HighFive::DataSet m_ds_cellX;
        HighFive::DataSet m_ds_cellY;
        HighFive::DataSet m_ds_cellZ;
        HighFive::DataSet m_ds_cellDose;
    
        /// Buffers for appending before flushing
        std::vector<int>    m_buf_eventId;
        std::vector<int>    m_buf_cellX;
        std::vector<int>    m_buf_cellY;
        std::vector<int>    m_buf_cellZ;
        std::vector<double> m_buf_cellDose;
    
        /// Current run identifier
        int m_runId{-1};
    
        /// Create extendible datasets under /Events
        void CreateDatasets();
    
        /// Flush buffered data into HDF5 file
        void FlushBuffers();
    };

#endif // HDF5EVENTANALISYS_HH