// HDF5EventAnalysis.cc
#include "HDF5EventAnalysis.hh"
#include "G4Threading.hh"  // ← dołącz, by mieć G4GetThreadId()
#include "G4Run.hh"
#include "G4Cache.hh"
#include "G4Event.hh"
#include "G4SDManager.hh"
#include "VoxelHit.hh"
#include "G4SystemOfUnits.hh"
#include <highfive/H5File.hpp>
#include <filesystem>


// Initialize thread-local instance
G4ThreadLocal HDF5EventAnalysis* HDF5EventAnalysis::fInstance = nullptr;

// Initialize static hits collection cache
G4Cache<std::vector<G4String>> HDF5EventAnalysis::m_hc_names;

HDF5EventAnalysis* HDF5EventAnalysis::GetInstance() {
    if (!fInstance) {
        fInstance = new HDF5EventAnalysis();
    }
    return fInstance;
}

void HDF5EventAnalysis::DefineHitsCollection(const G4String& hcName) {
    // Register a sensitive detector hits collection name
    m_hc_names.Get().push_back(hcName);
}

HDF5EventAnalysis::HDF5EventAnalysis() = default;
HDF5EventAnalysis::~HDF5EventAnalysis() = default;

void HDF5EventAnalysis::BeginOfRun(const G4Run* run, bool /*isMaster*/) {
    // Always open file per thread to avoid HDF5 conflicts
    m_runId = run->GetRunID();
    int threadId = G4Threading::G4GetThreadId();
    std::string filename = "run" + std::to_string(m_runId)
                         + "_thread" + std::to_string(threadId)
                         + ".h5";
    G4cout << "[HDF5] Opening file: " << filename << G4endl;
    m_file = std::make_unique<HighFive::File>(filename,
                 HighFive::File::Truncate);

    // Create group and datasets
    m_file->createGroup("Events");
    CreateDatasets();
}

void HDF5EventAnalysis::CreateDatasets() {
    // Create extendible (chunked) DataSpace
    hsize_t init = 0;
    hsize_t max  = HighFive::DataSpace::UNLIMITED;
    HighFive::DataSpace space({init}, {max});
    HighFive::DataSetCreateProps props;
    props.add(HighFive::Chunking{{1024}});

    auto grp = m_file->getGroup("Events");
    // Define minimal datasets
    m_ds_eventId  = grp.createDataSet<int>(   "EventID",     space, props);
    m_ds_cellX    = grp.createDataSet<int>(   "CellID_X",    space, props);
    m_ds_cellY    = grp.createDataSet<int>(   "CellID_Y",    space, props);
    m_ds_cellZ    = grp.createDataSet<int>(   "CellID_Z",    space, props);
    m_ds_cellDose = grp.createDataSet<double>("CellDose_Gy", space, props);
}

void HDF5EventAnalysis::EndOfEventAction(const G4Event* event) {
    // Only proceed if file is open
    if (!m_file) return;

    auto hcOfEvt = event->GetHCofThisEvent();
    if (!hcOfEvt) return;

    // Loop over each registered hits collection name
    auto& names = m_hc_names.Get();
    auto sdman = G4SDManager::GetSDMpointer();
    for (const auto& hcName : names) {
        int collId = sdman->GetCollectionID(hcName);
        if (collId < 0) {
            G4cout << "[WARNING] HDF5: HC '" << hcName
                   << "' not found (ID=" << collId << ")" << G4endl;
            continue;
        }
        auto hitsColl = dynamic_cast<VoxelHitsCollection*>(
                            hcOfEvt->GetHC(collId));
        if (!hitsColl) {
            G4cout << "[WARNING] HDF5: null pointer for HC '"
                   << hcName << "'" << G4endl;
            continue;
        }

        // Buffer each hit's minimal data
        for (int i = 0; i < hitsColl->entries(); ++i) {
            auto hit = dynamic_cast<VoxelHit*>(hitsColl->GetHit(i));
            m_buf_eventId .push_back(event->GetEventID());
            m_buf_cellX   .push_back(hit->GetGlobalID(0));
            m_buf_cellY   .push_back(hit->GetGlobalID(1));
            m_buf_cellZ   .push_back(hit->GetGlobalID(2));
            m_buf_cellDose.push_back(hit->GetDose());
        }
    }

    // Flush all buffered entries
    FlushBuffers();
    m_buf_eventId.clear();
    m_buf_cellX.clear();
    m_buf_cellY.clear();
    m_buf_cellZ.clear();
    m_buf_cellDose.clear();
}

void HDF5EventAnalysis::FlushBuffers() {
    auto oldSize = m_ds_eventId.getSpace().getDimensions()[0];
    auto batch   = m_buf_eventId.size();
    if (batch == 0) return;
    auto newSize = oldSize + batch;

    // Resize and write hyperslabs
    m_ds_eventId .resize({newSize});
    m_ds_cellX   .resize({newSize});
    m_ds_cellY   .resize({newSize});
    m_ds_cellZ   .resize({newSize});
    m_ds_cellDose.resize({newSize});

    m_ds_eventId .select({oldSize}, {batch}).write(m_buf_eventId);
    m_ds_cellX   .select({oldSize}, {batch}).write(m_buf_cellX);
    m_ds_cellY   .select({oldSize}, {batch}).write(m_buf_cellY);
    m_ds_cellZ   .select({oldSize}, {batch}).write(m_buf_cellZ);
    m_ds_cellDose.select({oldSize}, {batch}).write(m_buf_cellDose);
}

void HDF5EventAnalysis::EndOfRun(const G4Run*) {
    // Close file via unique_ptr destructor
    m_file.reset();
}
