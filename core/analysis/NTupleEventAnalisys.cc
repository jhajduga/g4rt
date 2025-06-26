//
// Created by brachwal on 30.05.2020.
//
// Updated and refactored for clarity by jhajduga
// 26.06.2025

#include "NTupleEventAnalisys.hh"
#include "G4AnalysisManager.hh"
#include "G4Event.hh"
#include "G4Run.hh"
#include "G4SDManager.hh"
#include "G4Threading.hh"
#include "Services.hh"
#include "VPatient.hh"
#include "VoxelHit.hh"
#include <algorithm>
#include <vector>



G4Cache<std::vector<NTupleEventAnalisys::TTreeCollection>>
    NTupleEventAnalisys::m_ttree_collection;

////////////////////////////////////////////////////////////////////////////////
///
NTupleEventAnalisys *NTupleEventAnalisys::GetInstance() {
  static NTupleEventAnalisys instance = NTupleEventAnalisys();
  return &instance;
}
////////////////////////////////////////////////////////////////////////////////
/// Refactored CreateNTuple to use AnalysisFlags instead of legacy bools
void NTupleEventAnalisys::CreateNTuple(const TTreeCollection &treeColl) {
  const auto &flags = treeColl.flags;
  const auto treeName = treeColl.m_name + m_treeNamePostfix;
  const auto &treeDescription = treeColl.m_description;

  auto analysisManager = G4AnalysisManager::Instance();
  m_ntuple_collection.Insert(treeName, TTreeEventCollection());
  auto &evtNTupleColl = m_ntuple_collection.Get(treeName);
  evtNTupleColl.m_ntupleId =
      analysisManager->CreateNtuple(treeName, treeDescription);

  auto createI = [&](const char *name) {
    evtNTupleColl.m_colId[name] =
        analysisManager->CreateNtupleIColumn(evtNTupleColl.m_ntupleId, name);
  };
  auto createD = [&](const char *name) {
    evtNTupleColl.m_colId[name] =
        analysisManager->CreateNtupleDColumn(evtNTupleColl.m_ntupleId, name);
  };
  auto createVecI = [&](const char *name, std::vector<G4int> &vec) {
    evtNTupleColl.m_colId[name] = analysisManager->CreateNtupleIColumn(
        evtNTupleColl.m_ntupleId, name, vec);
  };
  auto createVecD = [&](const char *name, std::vector<G4double> &vec) {
    evtNTupleColl.m_colId[name] = analysisManager->CreateNtupleDColumn(
        evtNTupleColl.m_ntupleId, name, vec);
  };

  // General per-event branches (only in detailed mode)
  if (!flags[AnalysisFlag::MinimalMode]) {
    createI("ThreadId");
    createI("G4EvtId");
    createI("G4RunId");
    createD("G4EvtGlobalTime");
    createD("GantryAngle");
  }

  // Cell-level scoring info
  if (flags[AnalysisFlag::StorePositions] ||
      flags[AnalysisFlag::StoreEnergies]) {
    createI("CellIdX");
    createI("CellIdY");
    createI("CellIdZ");
    if (!flags[AnalysisFlag::MinimalMode]) {
      createD("CellPositionX");
      createD("CellPositionY");
      createD("CellPositionZ");
      createD("CellEDeposit");
      createD("CellMeanEDeposit");
    }
    createD("CellDose");
  }

  // Voxel-level branches
  if (flags[AnalysisFlag::StorePositions]) {
    createI("VoxelIdX");
    createI("VoxelIdY");
    createI("VoxelIdZ");
    if (!flags[AnalysisFlag::MinimalMode]) {
      createD("VoxelPositionX");
      createD("VoxelPositionY");
      createD("VoxelPositionZ");
      createD("VoxelEDeposit");
      createD("VoxelMeanEDeposit");
    }
    createD("VoxelDose");

    if (flags[AnalysisFlag::StoreTracks]) {
      createVecI("VoxelTrkId", evtNTupleColl.m_VoxelTrkId);
      createVecI("VoxelTrkTypeId", evtNTupleColl.m_VoxelTrkTypeId);
      createVecI("ProcessTypeId", evtNTupleColl.m_ProcessTypeId);
      createVecI("ElectronOriginTypeId", evtNTupleColl.m_ElectronOriginTypeId);

      if (!flags[AnalysisFlag::MinimalMode]) {
        createVecD("VoxelTrkPositionX", evtNTupleColl.m_VoxelTrkPositionX);
        createVecD("VoxelTrkPositionY", evtNTupleColl.m_VoxelTrkPositionY);
        createVecD("VoxelTrkPositionZ", evtNTupleColl.m_VoxelTrkPositionZ);
        createVecD("VoxelTrkE", evtNTupleColl.m_VoxelTrkEnergy);
        createVecD("VoxelTrkTheta", evtNTupleColl.m_VoxelTrkTheta);
        createVecD("VoxelTrkLength", evtNTupleColl.m_VoxelTrkLength);
        createD("G4EvtPrimaryE"); // optional, may still be temporary
        createI("G4EvtPrimaryN");
      }
    }
  }
  
  analysisManager->FinishNtuple(evtNTupleColl.m_ntupleId);
}


////////////////////////////////////////////////////////////////////////////////
///
void NTupleEventAnalisys::SetAnalysisFlag(const G4String &treeName,
                                          AnalysisFlag which, bool enable) {
  ::SetAnalysisFlag(treeName, which, enable);
}


////////////////////////////////////////////////////////////////////////////////
/// Refactored FillEventCollection to use AnalysisFlags
void NTupleEventAnalisys::FillEventCollection(const G4String& treeName, const G4Event* evt, VoxelHitsCollection* hitsColl) {
  const auto& flags = g_tree_flag_map[treeName];
  auto isoToSim = Service<ConfigSvc>()->GetValue<G4ThreeVector>("WorldConstruction", "IsoToSimTransformation");
  auto analysisManager = G4AnalysisManager::Instance();
  auto ntplId = GetNTupleId(treeName);
  if (ntplId == -1) {
    G4cout << " [WARNING]::FillEventCollection:: given TTree (" << treeName << ") collection doesn't exist!" << G4endl;
    return;
  }
  auto& evtColl = m_ntuple_collection.Get(treeName);
  const G4int nHits = hitsColl->entries();
  if (nHits == 0) return;

  evtColl.m_evtId = evt->GetEventID();
  std::vector<G4double> hits_evt_global_time;

  for (int i = 0; i < nHits; ++i) {
    auto hit = dynamic_cast<VoxelHit*>(hitsColl->GetHit(i));
    auto eDep = hit->GetEnergyDeposit() / keV;
    auto meanEDep = hit->GetMeanEnergyDeposit() / keV;
    const auto dose = hit->GetDose();

    evtColl.m_CellIdX.push_back(hit->GetGlobalID(0));
    evtColl.m_CellIdY.push_back(hit->GetGlobalID(1));
    evtColl.m_CellIdZ.push_back(hit->GetGlobalID(2));

    if (!flags[AnalysisFlag::MinimalMode]) {
      const auto cell = hit->GetGlobalCentre() - isoToSim;
      evtColl.m_CellPositionX.push_back(cell.x());
      evtColl.m_CellPositionY.push_back(cell.y());
      evtColl.m_CellPositionZ.push_back(cell.z());
    }

    const auto cellVolume = Service<GeoSvc>()->Patient()->GetCellVolume();
    evtColl.m_CellIDose.push_back(dose * hit->GetVolume() / cellVolume);

    evtColl.m_VoxelIdX.push_back(hit->GetID(0));
    evtColl.m_VoxelIdY.push_back(hit->GetID(1));
    evtColl.m_VoxelIdZ.push_back(hit->GetID(2));

    if (!flags[AnalysisFlag::MinimalMode]) {
      const auto center = hit->GetCentre() + hit->GetGlobalCentre();
      evtColl.m_VoxelPositionX.push_back(center.x());
      evtColl.m_VoxelPositionY.push_back(center.y());
      evtColl.m_VoxelPositionZ.push_back(center.z());
    }

    if (!flags[AnalysisFlag::MinimalMode]) {
      evtColl.m_VoxelHitEDeposit.push_back(eDep);
      evtColl.m_VoxelHitMeanEDeposit.push_back(meanEDep);
    }
    evtColl.m_VoxelHitDose.push_back(dose);

    // Optional primaries info
    const auto& primE = hit->GetEvtPrimariesEnergy();
    evtColl.m_G4EvtPrimaryEnergy.assign(primE.begin(), primE.end());
    if (!flags[AnalysisFlag::MinimalMode]) {
      evtColl.m_EvtPrimariesN = hit->GetEvtNPrimaries();
      hits_evt_global_time.push_back(hit->GetGlobalTime());
    }

    if (flags[AnalysisFlag::StoreTracks]) {
      // All below are multi-valued per hit → vector of vectors
      const auto& trkMap = hit->GetTrkType();
      const auto& procMap = hit->GetProcessType();
      const auto& origMap = hit->GetElectronOriginType();

      std::vector<G4int> trkType, trkTypeId;
      for (const auto& [id, type] : trkMap) {
        trkType.push_back(id);
        trkTypeId.push_back(type);
      }
      evtColl.m_VoxelHitsTrkId.push_back(trkType);
      evtColl.m_VoxelHitsTrkTypeId.push_back(trkTypeId);

      std::vector<G4int> procTypeId;
      for (const auto& [_, proc] : procMap) procTypeId.push_back(proc);
      evtColl.m_VoxelHitsProcessTypeId.push_back(procTypeId);

      std::vector<G4int> origTypeId;
      for (const auto& [_, org] : origMap) origTypeId.push_back(org);
      evtColl.m_VoxelHitsElectronOriginTypeId.push_back(origTypeId);

      std::vector<G4double> eList, thetaList, lenList;
      std::vector<G4double> xList, yList, zList;
      for (const auto& [_, val] : hit->GetTrkEnergy()) eList.push_back(val / MeV);
      for (const auto& [_, val] : hit->GetTrkTheta()) thetaList.push_back(val);
      for (const auto& [_, val] : hit->GetTrkLength()) lenList.push_back(val);
      for (const auto& [_, pos] : hit->GetTrkPosition()) {
        xList.push_back(pos.x() - isoToSim.x());
        yList.push_back(pos.y() - isoToSim.y());
        zList.push_back(pos.z() - isoToSim.z());
      }

      evtColl.m_VoxelHitsTrkEnergy.push_back(eList);
      evtColl.m_VoxelHitsTrkTheta.push_back(thetaList);
      evtColl.m_VoxelHitsTrkLength.push_back(lenList);
      evtColl.m_VoxelHitsTrkPosX.push_back(xList);
      evtColl.m_VoxelHitsTrkPosY.push_back(yList);
      evtColl.m_VoxelHitsTrkPosZ.push_back(zList);
    }
  }

  if (!flags[AnalysisFlag::MinimalMode] && !hits_evt_global_time.empty()) {
    evtColl.m_global_time = *std::max_element(hits_evt_global_time.begin(), hits_evt_global_time.end());
  }
}



////////////////////////////////////////////////////////////////////////////////
/// Refactored FillNTupleEvent to use AnalysisFlags instead of legacy flags
void NTupleEventAnalisys::FillNTupleEvent() {

  auto analysisManager = G4AnalysisManager::Instance();

  for (auto coll = m_ntuple_collection.Begin();
       coll != m_ntuple_collection.End(); ++coll) {
    const auto &treeName = coll->first;
    const auto &flags = g_tree_flag_map[treeName];
    auto &treeEvtColl = coll->second;
    const auto ntupleId = treeEvtColl.m_ntupleId;
    const G4int nHits = treeEvtColl.m_CellIdX.size();

    if (nHits == 0)
      continue; // empty event

    auto fillI = [&](const char *name, G4int val) {
      analysisManager->FillNtupleIColumn(ntupleId, treeEvtColl.m_colId[name],
                                         val);
    };
    auto fillD = [&](const char *name, G4double val) {
      analysisManager->FillNtupleDColumn(ntupleId, treeEvtColl.m_colId[name],
                                         val);
    };

    for (int i = 0; i < nHits; ++i) {
      if (!flags[AnalysisFlag::MinimalMode]) {
        fillI("ThreadId", G4Threading::G4GetThreadId());
        fillI("G4EvtId", treeEvtColl.m_evtId);
        fillD("G4EvtGlobalTime", treeEvtColl.m_global_time);
        fillI("G4EvtPrimaryN", treeEvtColl.m_EvtPrimariesN);
        fillI("G4RunId", m_runId);
        fillD("GantryAngle", m_degree_rotation);
      }

      if (flags[AnalysisFlag::StorePositions] ||
          flags[AnalysisFlag::StoreEnergies]) {
        fillI("CellIdX", treeEvtColl.m_CellIdX.at(i));
        fillI("CellIdY", treeEvtColl.m_CellIdY.at(i));
        fillI("CellIdZ", treeEvtColl.m_CellIdZ.at(i));

        if (!flags[AnalysisFlag::MinimalMode]) {
          fillD("CellPositionX", treeEvtColl.m_CellPositionX.at(i));
          fillD("CellPositionY", treeEvtColl.m_CellPositionY.at(i));
          fillD("CellPositionZ", treeEvtColl.m_CellPositionZ.at(i));
          fillD("CellEDeposit", treeEvtColl.m_VoxelHitEDeposit.at(i));
          fillD("CellMeanEDeposit", treeEvtColl.m_VoxelHitMeanEDeposit.at(i));
        }
        fillD("CellDose", treeEvtColl.m_CellIDose.at(i));
      }

      if (flags[AnalysisFlag::StorePositions]) {
        fillI("VoxelIdX", treeEvtColl.m_VoxelIdX.at(i));
        fillI("VoxelIdY", treeEvtColl.m_VoxelIdY.at(i));
        fillI("VoxelIdZ", treeEvtColl.m_VoxelIdZ.at(i));

        if (!flags[AnalysisFlag::MinimalMode]) {
          fillD("VoxelPositionX", treeEvtColl.m_VoxelPositionX.at(i));
          fillD("VoxelPositionY", treeEvtColl.m_VoxelPositionY.at(i));
          fillD("VoxelPositionZ", treeEvtColl.m_VoxelPositionZ.at(i));
          fillD("VoxelEDeposit", treeEvtColl.m_VoxelHitEDeposit.at(i));
          fillD("VoxelMeanEDeposit", treeEvtColl.m_VoxelHitMeanEDeposit.at(i));
        }
        fillD("VoxelDose", treeEvtColl.m_VoxelHitDose.at(i));
      }

      if (flags[AnalysisFlag::StoreTracks]) {
        treeEvtColl.m_VoxelTrkId = treeEvtColl.m_VoxelHitsTrkId.at(i);
        treeEvtColl.m_VoxelTrkTypeId = treeEvtColl.m_VoxelHitsTrkTypeId.at(i);
        treeEvtColl.m_ProcessTypeId =
            treeEvtColl.m_VoxelHitsProcessTypeId.at(i);
        treeEvtColl.m_ElectronOriginTypeId =
            treeEvtColl.m_VoxelHitsElectronOriginTypeId.at(i);
        treeEvtColl.m_VoxelTrkEnergy = treeEvtColl.m_VoxelHitsTrkEnergy.at(i);
        treeEvtColl.m_VoxelTrkTheta = treeEvtColl.m_VoxelHitsTrkTheta.at(i);
        treeEvtColl.m_VoxelTrkLength = treeEvtColl.m_VoxelHitsTrkLength.at(i);
        treeEvtColl.m_VoxelTrkPositionX = treeEvtColl.m_VoxelHitsTrkPosX.at(i);
        treeEvtColl.m_VoxelTrkPositionY = treeEvtColl.m_VoxelHitsTrkPosY.at(i);
        treeEvtColl.m_VoxelTrkPositionZ = treeEvtColl.m_VoxelHitsTrkPosZ.at(i);
      }

      analysisManager->AddNtupleRow(ntupleId);
    }
  }
}

////////////////////////////////////////////////////////////////////////////////
/// Refactored ClearEventCollections to respect flag logic
void NTupleEventAnalisys::ClearEventCollections() {
  for (auto coll = m_ntuple_collection.Begin();
       coll != m_ntuple_collection.End(); ++coll) {
    const auto &treeName = coll->first;
    const auto &flags = g_tree_flag_map[treeName];
    auto &evt = coll->second;

    evt.m_evtId = -1;
    evt.m_global_time = 0.;
    evt.m_G4EvtPrimaryEnergy.clear();
    evt.m_EvtPrimariesN = 0;

    evt.m_CellIdX.clear();
    evt.m_CellIdY.clear();
    evt.m_CellIdZ.clear();
    evt.m_CellIDose.clear();

    if (!flags[AnalysisFlag::MinimalMode]) {
      evt.m_CellPositionX.clear();
      evt.m_CellPositionY.clear();
      evt.m_CellPositionZ.clear();
      evt.m_VoxelHitEDeposit.clear();
      evt.m_VoxelHitMeanEDeposit.clear();
    }

    evt.m_VoxelIdX.clear();
    evt.m_VoxelIdY.clear();
    evt.m_VoxelIdZ.clear();
    evt.m_VoxelHitDose.clear();

    if (!flags[AnalysisFlag::MinimalMode]) {
      evt.m_VoxelPositionX.clear();
      evt.m_VoxelPositionY.clear();
      evt.m_VoxelPositionZ.clear();
    }

    if (flags[AnalysisFlag::StoreTracks]) {
      evt.m_VoxelHitsTrkId.clear();
      evt.m_VoxelHitsTrkTypeId.clear();
      evt.m_VoxelHitsTrkEnergy.clear();
      evt.m_VoxelHitsProcessTypeId.clear();
      evt.m_VoxelHitsElectronOriginTypeId.clear();
      evt.m_VoxelHitsTrkTheta.clear();
      evt.m_VoxelHitsTrkLength.clear();
      evt.m_VoxelHitsTrkPosX.clear();
      evt.m_VoxelHitsTrkPosY.clear();
      evt.m_VoxelHitsTrkPosZ.clear();

      evt.m_VoxelTrkId.clear();
      evt.m_VoxelTrkTypeId.clear();
      evt.m_ProcessTypeId.clear();
      evt.m_ElectronOriginTypeId.clear();
      evt.m_VoxelTrkEnergy.clear();
      evt.m_VoxelTrkTheta.clear();
      evt.m_VoxelTrkLength.clear();
      evt.m_VoxelTrkPositionX.clear();
      evt.m_VoxelTrkPositionY.clear();
      evt.m_VoxelTrkPositionZ.clear();
    }
  }
}

////////////////////////////////////////////////////////////////////////////////
/// DefineTTree rewritten to preserve logic: single/multi HC registration
void NTupleEventAnalisys::DefineTTree(const G4String &treeName,
                                     bool cellVoxelisation,
                                     const G4String &hcName,
                                     const G4String &treeDescription) {
  if (!Service<ConfigSvc>()->GetValue<bool>("RunSvc", "NTupleAnalysis"))
    return;

  std::vector<TTreeCollection> &ttreeVec = m_ttree_collection.Get();

  if (hcName.empty()) {
    LOGSVC_INFO("Defining TTree: {} (single scoring volume)", treeName);

    // Create new entry for single-scoring-volume case
    ttreeVec.emplace_back();
    auto &tree = ttreeVec.back();
    tree.m_name = treeName;
    tree.m_description = treeDescription;
    tree.m_hc_names.emplace_back(treeName);

    // Prepare flags if not yet configured externally
    if (!g_tree_flag_map.count(treeName)) {
      g_tree_flag_map[treeName] = AnalysisFlags();
    }
    if (cellVoxelisation) {
      g_tree_flag_map[treeName].Set(AnalysisFlag::StorePositions, true);
    }
  } else {
    // Multi-volume TTree (shared structure across HC)
    G4int treeIdx = -1;
    G4bool treeExists = false;
    for (const auto &tree : ttreeVec) {
      ++treeIdx;
      if (tree.m_name == treeName) {
        treeExists = true;
        break;
      }
    }

    if (!treeExists) {
      LOGSVC_INFO("Defining TTree: {} (new shared tree)", treeName);
      ttreeVec.emplace_back();
      auto &tree = ttreeVec.back();
      tree.m_name = treeName;
      tree.m_description = treeDescription;
      tree.m_hc_names.emplace_back(hcName);

      if (!g_tree_flag_map.count(treeName)) {
        g_tree_flag_map[treeName] = AnalysisFlags();
      }
      if (cellVoxelisation) {
        g_tree_flag_map[treeName].Set(AnalysisFlag::StorePositions, true);
      }
    } else {
      // Append HC name to existing tree
      ttreeVec.at(treeIdx).m_hc_names.emplace_back(hcName);
    }
  }
}


////////////////////////////////////////////////////////////////////////////////
/// BeginOfRun: initialize flags and create Ntuples if missing
void NTupleEventAnalisys::BeginOfRun(const G4Run* runPtr, G4bool /*isMaster*/) {
  if (!Service<ConfigSvc>()->GetValue<bool>("RunSvc", "NTupleAnalysis")) return;

  m_runId = runPtr->GetRunID();
  m_degree_rotation = Service<RunSvc>()->CurrentControlPoint()->GetDegreeRotation();

  for (const auto& tree : m_ttree_collection.Get()) {
    const auto fullName = tree.m_name + m_treeNamePostfix;
    if (GetNTupleId(fullName) == -1) {
      CreateNTuple(tree);
    }
  }
}

////////////////////////////////////////////////////////////////////////////////
/// Return Ntuple ID or -1 if not found
G4int NTupleEventAnalisys::GetNTupleId(const G4String& treeName) {
  return m_ntuple_collection.Has(treeName) ? m_ntuple_collection.Get(treeName).m_ntupleId : -1;
}

////////////////////////////////////////////////////////////////////////////////
/// This member is called at the end of every event from
/// EventAction::EndOfEventAction
void NTupleEventAnalisys::EndOfEventAction(const G4Event *evt) {
  auto hCofThisEvent = evt->GetHCofThisEvent();
  if (!hCofThisEvent)
    return;

  ClearEventCollections();

  for (const auto &tree : m_ttree_collection.Get()) {
    for (const auto &hc : tree.m_hc_names) {
      const auto fullName = tree.m_name + m_treeNamePostfix;

      auto collection_id = G4SDManager::GetSDMpointer()->GetCollectionID(hc);
      if (collection_id < 0) {
        G4cout << "[ERROR]:: NTupleEventAnalisys::EndOfEventAction TTree: "
               << fullName << " G4SDManager err: " << collection_id << G4endl;
        continue;
      }

      auto hitsColl = dynamic_cast<VoxelHitsCollection *>(
          hCofThisEvent->GetHC(collection_id));
      if (hitsColl) {
        FillEventCollection(fullName, evt, hitsColl);
      }
    }
  }

  FillNTupleEvent();
}
