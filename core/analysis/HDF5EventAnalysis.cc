// HDF5EventAnalysis.cc
#include "HDF5EventAnalysis.hh"
#include "G4SDManager.hh"
#include "G4Threading.hh"
#include <mutex>

G4ThreadLocal HDF5EventAnalysis* HDF5EventAnalysis::fInstance = nullptr;

HDF5EventAnalysis* HDF5EventAnalysis::GetInstance() {
    if (!fInstance) {
        fInstance = new HDF5EventAnalysis();
    }
    return fInstance;
}

G4Cache<std::map<std::string, HDF5EventAnalysis::EventDataCollection>> HDF5EventAnalysis::m_event_collections;
G4Cache<std::vector<HDF5EventAnalysis::DatasetDef>> HDF5EventAnalysis::m_dataset_defs;

HDF5EventAnalysis::HDF5EventAnalysis() {}

HDF5EventAnalysis::~HDF5EventAnalysis() {}


void HDF5EventAnalysis::BeginOfRun(const G4Run* runPtr, G4bool isMaster) {
    // Master thread opens file and creates group + chunked datasets
    if (isMaster) {
        std::string fname = "output_run" + std::to_string(runPtr->GetRunID()) + ".h5";
        auto& hdf5 = HDF5Manager::Instance();
        hdf5.OpenFile(fname);

        m_currentRunGroup = "Run_" + std::to_string(runPtr->GetRunID());
        // Create top-level group if needed
        if (!hdf5.GroupExists(m_currentRunGroup)) {
            hdf5.File().createGroup(m_currentRunGroup);
        }

        // Define unlimited, chunked dataspace
        HighFive::DataSpace ds({0}, {HighFive::DataSpace::UNLIMITED});
        HighFive::DataSetCreateProps props;
        props.add(HighFive::Chunking({1024}));

        // Create all required datasets once
        auto& file = hdf5.File();
        file.createDataSet<int>(m_currentRunGroup + "/VoxelIdX", ds, props);
        file.createDataSet<int>(m_currentRunGroup + "/VoxelIdY", ds, props);
        file.createDataSet<int>(m_currentRunGroup + "/VoxelIdZ", ds, props);
        file.createDataSet<double>(m_currentRunGroup + "/VoxelDose", ds, props);
    }
}


void HDF5EventAnalysis::DefineDatasetStructure(const std::string& datasetName) {
    auto& collections = m_event_collections.Get();
    if (collections.find(datasetName) == collections.end()) {
        collections[datasetName] = EventDataCollection();
    }
}

void HDF5EventAnalysis::DefineDataset(const G4String& datasetName, const G4String& hcName) {
    auto& defs = m_dataset_defs.Get();
    // szukamy, czy już istnieje definition o takiej nazwie
    for (auto& d : defs) {
        if (d.name == datasetName) {
            d.hc_names.push_back(hcName);
        return;
    }
}

defs.push_back({datasetName, {hcName}});
}

void HDF5EventAnalysis::FillEventData(const G4Event* evt, VoxelHitsCollection* hitsColl) {
    auto& collections = m_event_collections.Get();
    auto& evtCollection = collections["EventData"];

    for (int i = 0; i < hitsColl->entries(); ++i) {
        auto hit = dynamic_cast<VoxelHit*>(hitsColl->GetHit(i));
        evtCollection.voxelIdX.push_back(hit->GetID(0));
        evtCollection.voxelIdY.push_back(hit->GetID(1));
        evtCollection.voxelIdZ.push_back(hit->GetID(2));
        evtCollection.voxelDose.push_back(hit->GetDose());
    }
}

void HDF5EventAnalysis::WriteEventData() {
    auto& collections = m_event_collections.Get();
    auto& hdf5 = HDF5Manager::Instance();

    for (const auto& coll : collections) {
        const auto& evtCollection = coll.second;

        std::string basePath = m_currentRunGroup + "/" + coll.first;
        hdf5.WriteDataset(basePath + "/VoxelIdX", evtCollection.voxelIdX);
        hdf5.WriteDataset(basePath + "/VoxelIdY", evtCollection.voxelIdY);
        hdf5.WriteDataset(basePath + "/VoxelIdZ", evtCollection.voxelIdZ);
        hdf5.WriteDataset(basePath + "/VoxelDose", evtCollection.voxelDose);
    }
}

void HDF5EventAnalysis::ClearEventData() {
    auto& collections = m_event_collections.Get();

    for (auto& coll : collections) {
        auto& evtCollection = coll.second;
        evtCollection.voxelIdX.clear();
        evtCollection.voxelIdY.clear();
        evtCollection.voxelIdZ.clear();
        evtCollection.voxelDose.clear();
    }
}


void HDF5EventAnalysis::EndOfEventAction(const G4Event* evt) {
    // Always collect data in thread-local cache
    auto hCofThisEvent = evt->GetHCofThisEvent();
    if (!hCofThisEvent) return;

    ClearEventData();

    auto collection_id = G4SDManager::GetSDMpointer()->GetCollectionID("VoxelHitsCollection");
    if (collection_id < 0) return;
    auto hitsColl = dynamic_cast<VoxelHitsCollection*>(hCofThisEvent->GetHC(collection_id));
    FillEventData(evt, hitsColl);

    // Append to HDF5 under mutex
    auto& hdf5 = HDF5Manager::Instance();
    std::lock_guard<std::mutex> lock(hdf5.Mutex());
    auto& file = hdf5.File();
    auto& evtCol = m_event_collections.Get()["EventData"];

    // Helper lambda for each dataset
    auto append = [&](auto const& dataVec, const std::string& name) {
        auto ds = file.getDataSet(m_currentRunGroup + "/" + name);
        auto dims = ds.getSpace().getDimensions();
        size_t oldSize = dims[0];
        size_t newCount = dataVec.size();
        ds.resize({oldSize + newCount});
        ds.select({oldSize}, {newCount}).write(dataVec);
    };

    append(evtCol.voxelIdX, "VoxelIdX");
    append(evtCol.voxelIdY, "VoxelIdY");
    append(evtCol.voxelIdZ, "VoxelIdZ");
    append(evtCol.voxelDose, "VoxelDose");

    ClearEventData();
}

