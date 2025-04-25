// HDF5EventAnalysis.cc
#include "HDF5EventAnalysis.hh"
#include "G4SDManager.hh"
#include <mutex>

// Static member definitions
std::vector<HDF5EventAnalysis::DatasetDef>
    HDF5EventAnalysis::m_dataset_defs;
std::mutex HDF5EventAnalysis::m_defs_mutex;

HDF5EventAnalysis* HDF5EventAnalysis::GetInstance() {
    static G4ThreadLocal HDF5EventAnalysis* instance = nullptr;
    if (!instance) {
        instance = new HDF5EventAnalysis();
    }
    return instance;
}

HDF5EventAnalysis::HDF5EventAnalysis() {}
HDF5EventAnalysis::~HDF5EventAnalysis() {}

void HDF5EventAnalysis::DefineDataset(const G4String& datasetName,
                                      const G4String& hitsCollName) {
    std::lock_guard<std::mutex> locker(m_defs_mutex);
    auto& defs = m_dataset_defs;
    for (auto& d : defs) {
        if (d.name == datasetName) {
            d.hc_names.push_back(hitsCollName);
            return;
        }
    }
    defs.push_back({datasetName, {hitsCollName}});
}

void HDF5EventAnalysis::BeginOfRun(const G4Run* runPtr, G4bool) {
    m_currentRunGroup = "Run_" + std::to_string(runPtr->GetRunID());
    auto& file = HDF5Manager::Instance().File();
  
    if (!file.exist(m_currentRunGroup))
      file.createGroup(m_currentRunGroup);
  
    HighFive::DataSpace ds({0}, {HighFive::DataSpace::UNLIMITED});
    HighFive::DataSetCreateProps props;
    props.add(HighFive::Chunking({10240}));
  
    // 3) każda definicja datasetów
    for (auto const& def : m_dataset_defs) {
      std::string grpPath = m_currentRunGroup + "/" + def.name;
      if (!file.exist(grpPath))
        file.createGroup(grpPath);
  
      // tu guard przed istnieniem, chociaż w czystym per-thread pliku 
      // będzie go nie było za pierwszym razem:
      auto makeField = [&](const std::string& field){
        auto dsPath = grpPath + "/" + field;
        if (!file.exist(dsPath)) {
          file.createDataSet<int>(dsPath, ds, props);
        }
      };
      makeField("VoxelIdX");
      makeField("VoxelIdY");
      makeField("VoxelIdZ");
      // dla double trzeba osobno:
      auto dsPath = grpPath + "/VoxelDose";
      if (!file.exist(dsPath)) {
        file.createDataSet<double>(dsPath, ds, props);
      }
    }
  }
  


void HDF5EventAnalysis::FillEventData(const G4String& datasetName,
                                      VoxelHitsCollection* hitsColl) {
    auto& mapCache = m_event_collections.Get();
    auto& evtCol = mapCache[datasetName];
    for (size_t i = 0; i < hitsColl->entries(); ++i) {
        auto hit = dynamic_cast<VoxelHit*>(hitsColl->GetHit(i));
        evtCol.voxelIdX.push_back(hit->GetID(0));
        evtCol.voxelIdY.push_back(hit->GetID(1));
        evtCol.voxelIdZ.push_back(hit->GetID(2));
        evtCol.voxelDose.push_back(hit->GetDose());
    }
}

void HDF5EventAnalysis::ClearEventData(const G4String& datasetName) {
    auto& mapCache = m_event_collections.Get();
    auto& evtCol = mapCache[datasetName];
    evtCol.voxelIdX.clear();
    evtCol.voxelIdY.clear();
    evtCol.voxelIdZ.clear();
    evtCol.voxelDose.clear();
}

void HDF5EventAnalysis::EndOfEventAction(const G4Event* evt) {
    auto hCof = evt->GetHCofThisEvent();
    if (!hCof) return;
    auto& defs = m_dataset_defs;
    auto& hdf5 = HDF5Manager::Instance();
    auto& file = hdf5.File();
  
    // Dla każdej rejestracji DatasetDef:
    for (auto const& def : defs) {
      // 1) Zbierz event do cache’a ze wszystkich kolekcji
      for (auto const& hcName : def.hc_names) {
        int collID = G4SDManager::GetSDMpointer()->GetCollectionID(hcName);
        if (collID < 0) continue;
        auto hitsColl = dynamic_cast<VoxelHitsCollection*>(hCof->GetHC(collID));
        FillEventData(def.name, hitsColl);
      }
  
      // 2) Pod mutexem dopisz do własnego pliku
      std::lock_guard<std::mutex> lock(hdf5.Mutex());
      std::string grpPath = m_currentRunGroup + "/" + def.name;
      auto& evtCol = m_event_collections.Get()[def.name];
  
      auto append = [&](auto const& dataVec, const std::string& field) {
        auto ds = file.getDataSet(grpPath + "/" + field);
        auto dims = ds.getSpace().getDimensions();
        size_t oldSize = dims[0], newCount = dataVec.size();
        ds.resize({oldSize + newCount});
        ds.select({oldSize}, {newCount}).write(dataVec);
      };
  
      append(evtCol.voxelIdX, "VoxelIdX");
      append(evtCol.voxelIdY, "VoxelIdY");
      append(evtCol.voxelIdZ, "VoxelIdZ");
      append(evtCol.voxelDose, "VoxelDose");
  
      ClearEventData(def.name);
    }
  }
