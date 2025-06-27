//
// Created by brachwal on 30.05.2020.
//
// Updated and refactored for clarity by jhajduga
// 26.06.2025

#include "NTupleEventAnalisys.hh"

#include <algorithm>
#include <iostream>
#include <vector>

#include "G4AnalysisManager.hh"
#include "G4Event.hh"
#include "G4Run.hh"
#include "G4SDManager.hh"
#include "G4Threading.hh"
#include "Services.hh"
#include "VPatient.hh"
#include "VoxelHit.hh"

G4Cache<std::vector<NTupleEventAnalisys::TTreeCollection>> NTupleEventAnalisys::m_ttree_collection;

////////////////////////////////////////////////////////////////////////////////
///
NTupleEventAnalisys* NTupleEventAnalisys::GetInstance() {
  static NTupleEventAnalisys instance = NTupleEventAnalisys();
  return &instance;
}

////////////////////////////////////////////////////////////////////////////////
/// DefineTTree rewritten to preserve logic: single/multi HC registration
void NTupleEventAnalisys::DefineTTree(const G4String& treeName, bool cellVoxelisation, const G4String& hcName, const G4String& treeDescription) {
  if (!Service<ConfigSvc>()->GetValue<bool>("RunSvc", "NTupleAnalysis")) return;

  std::vector<TTreeCollection>& ttreeVec = m_ttree_collection.Get();

  if (hcName.empty()) {
    LOGSVC_INFO("Defining TTree: {} (single scoring volume)", treeName);

    // Create new entry for single-scoring-volume case
    ttreeVec.emplace_back();
    auto& tree = ttreeVec.back();
    tree.m_name = treeName;
    tree.m_description = treeDescription;
    tree.m_hc_names.emplace_back(treeName);


    if (cellVoxelisation) {
      AnalysisFlagRegistry::Instance().SetFlag(AnalysisFlag::Voxelized, true);
    }

  } else {
    // Multi-volume TTree (shared structure across HC)
    G4int treeIdx = -1;
    G4bool treeExists = false;
    for (const auto& tree : ttreeVec) {
      ++treeIdx;
      if (tree.m_name == treeName) {
        treeExists = true;
        break;
      }
    }

    if (!treeExists) {
      LOGSVC_INFO("Defining TTree: {} (new shared tree)", treeName);
      ttreeVec.emplace_back();
      auto& tree = ttreeVec.back();
      tree.m_name = treeName;
      tree.m_description = treeDescription;
      tree.m_hc_names.emplace_back(hcName);

      if (cellVoxelisation) {
        AnalysisFlagRegistry::Instance().SetFlag(AnalysisFlag::Voxelized, true);
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

  SetAnalysisFlag(AnalysisFlag::StoreRunInfo,  Service<ConfigSvc>()->GetValue<bool>("RunSvc", "StoreRunInfo"));
  SetAnalysisFlag(AnalysisFlag::StorePositions, Service<ConfigSvc>()->GetValue<bool>("RunSvc", "StorePositions"));
  SetAnalysisFlag(AnalysisFlag::StoreEnergies, Service<ConfigSvc>()->GetValue<bool>("RunSvc", "StoreEnergies"));
  SetAnalysisFlag(AnalysisFlag::StoreTracks, Service<ConfigSvc>()->GetValue<bool>("RunSvc", "StoreTracks"));
  SetAnalysisFlag(AnalysisFlag::StorePrimaries, Service<ConfigSvc>()->GetValue<bool>("RunSvc", "StorePrimaries"));
  SetAnalysisFlag(AnalysisFlag::MinimalMode, Service<ConfigSvc>()->GetValue<bool>("RunSvc", "MinimalMode"));

  m_runId = runPtr->GetRunID();
  m_degree_rotation = Service<RunSvc>()->CurrentControlPoint()->GetDegreeRotation();

  for (const auto& tree : m_ttree_collection.Get()) {
    const auto fullName = tree.m_name + m_treeNamePostfix;
    if (GetNTupleId(fullName) == -1) {
      std::cout << "Creating NTuple for: " << fullName << std::endl;
      CreateNTuple(tree);
      std::cout << "NTuple created successfully." << std::endl;
    }
  }
}

////////////////////////////////////////////////////////////////////////////////
/// Refactored CreateNTuple to use AnalysisFlags instead of legacy bools
void NTupleEventAnalisys::CreateNTuple(const TTreeCollection& treeColl) {
  const auto treeName = treeColl.m_name + m_treeNamePostfix;
  auto& registry = AnalysisFlagRegistry::Instance();

  const auto& treeDescription = treeColl.m_description;

  auto analysisManager = G4AnalysisManager::Instance();
  m_ntuple_collection.Insert(treeName, TTreeEventCollection());
  auto& evtNTupleColl = m_ntuple_collection.Get(treeName);
  evtNTupleColl.m_ntupleId = analysisManager->CreateNtuple(treeName, treeDescription);

  auto createI = [&](const char* name) { evtNTupleColl.m_colId[name] = analysisManager->CreateNtupleIColumn(evtNTupleColl.m_ntupleId, name); };
  auto createD = [&](const char* name) { evtNTupleColl.m_colId[name] = analysisManager->CreateNtupleDColumn(evtNTupleColl.m_ntupleId, name); };
  auto createVecI = [&](const char* name, std::vector<G4int>& vec) { evtNTupleColl.m_colId[name] = analysisManager->CreateNtupleIColumn(evtNTupleColl.m_ntupleId, name, vec); };
  auto createVecD = [&](const char* name, std::vector<G4double>& vec) { evtNTupleColl.m_colId[name] = analysisManager->CreateNtupleDColumn(evtNTupleColl.m_ntupleId, name, vec); };


      // General data that are always stored
      createI("CellIdX");
  createI("CellIdY");
  createI("CellIdZ");
  createD("CellDose");
  if (registry.IsEnabled(AnalysisFlag::Voxelized)) {
    createI("VoxelIdX");
    createI("VoxelIdY");
    createI("VoxelIdZ");
    createD("VoxelDose");
  }

  if (!registry.IsEnabled(AnalysisFlag::MinimalMode)) {
    createI("G4EvtId");
    createD("G4EvtGlobalTime");

    // General per-event branches (only in detailed mode)
    if (registry.IsEnabled(AnalysisFlag::StoreRunInfo)) {
      createI("ThreadId");
      createI("G4RunId");
      createD("GantryAngle");
    }

    // Cell-level scoring info
    if (registry.IsEnabled(AnalysisFlag::StorePositions)) {
      createD("CellPositionX");
      createD("CellPositionY");
      createD("CellPositionZ");
      if (registry.IsEnabled(AnalysisFlag::Voxelized)) {
        createD("VoxelPositionX");
        createD("VoxelPositionY");
        createD("VoxelPositionZ");
      }
    }

    if (registry.IsEnabled(AnalysisFlag::StoreEnergies)) {
      createD("CellEDeposit");
      createD("CellMeanEDeposit");
      if (registry.IsEnabled(AnalysisFlag::Voxelized)) {
        createD("VoxelEDeposit");
        createD("VoxelMeanEDeposit");
      }
    }

    // Voxel-level branches
    if (registry.IsEnabled(AnalysisFlag::StoreTracks)) {
      createVecI("VoxelTrkId", evtNTupleColl.m_VoxelTrkId);
      createVecI("VoxelTrkTypeId", evtNTupleColl.m_VoxelTrkTypeId);
      createVecI("ProcessTypeId", evtNTupleColl.m_ProcessTypeId);
      createVecI("ElectronOriginTypeId", evtNTupleColl.m_ElectronOriginTypeId);
      createVecD("VoxelTrkE", evtNTupleColl.m_VoxelTrkEnergy);
      createVecD("VoxelTrkTheta", evtNTupleColl.m_VoxelTrkTheta);
      createVecD("VoxelTrkLength", evtNTupleColl.m_VoxelTrkLength);
      createVecD("VoxelTrkPositionX", evtNTupleColl.m_VoxelTrkPositionX);
      createVecD("VoxelTrkPositionY", evtNTupleColl.m_VoxelTrkPositionY);
      createVecD("VoxelTrkPositionZ", evtNTupleColl.m_VoxelTrkPositionZ);
    }
    if (registry.IsEnabled(AnalysisFlag::StorePrimaries)) {
      createD("G4EvtPrimaryE");
      createI("G4EvtPrimaryN");
    }
  }
  analysisManager->FinishNtuple(evtNTupleColl.m_ntupleId);
}

////////////////////////////////////////////////////////////////////////////////
///
/// \brief Set an analysis flag for a specific TTree
void NTupleEventAnalisys::SetAnalysisFlag(AnalysisFlag which, bool enable) {
  AnalysisFlagRegistry::Instance().SetFlag(which, enable);
}

////////////////////////////////////////////////////////////////////////////////
/// Refactored FillEventCollection to match final AnalysisFlags layout
void NTupleEventAnalisys::FillEventCollection(const G4String& treeName, const G4Event* evt, VoxelHitsCollection* hitsColl) {
  auto& registry = AnalysisFlagRegistry::Instance();

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
    const auto eDep = hit->GetEnergyDeposit() / keV;
    const auto meanEDep = hit->GetMeanEnergyDeposit() / keV;
    const auto dose = hit->GetDose();
    const auto cellVolume = Service<GeoSvc>()->Patient()->GetCellVolume();
    const auto cell = hit->GetGlobalCentre() - isoToSim;
    const auto center = hit->GetCentre() + hit->GetGlobalCentre();

    // Always stored
    evtColl.m_CellIdX.push_back(hit->GetGlobalID(0));
    evtColl.m_CellIdY.push_back(hit->GetGlobalID(1));
    evtColl.m_CellIdZ.push_back(hit->GetGlobalID(2));
    evtColl.m_CellIDose.push_back(dose * hit->GetVolume() / cellVolume);

    if (registry.IsEnabled(AnalysisFlag::Voxelized)) {
      evtColl.m_VoxelIdX.push_back(hit->GetID(0));
      evtColl.m_VoxelIdY.push_back(hit->GetID(1));
      evtColl.m_VoxelIdZ.push_back(hit->GetID(2));
      evtColl.m_VoxelHitDose.push_back(dose);
    }

    if (!registry.IsEnabled(AnalysisFlag::MinimalMode)) {
      if (registry.IsEnabled(AnalysisFlag::StorePositions)) {
        evtColl.m_CellPositionX.push_back(cell.x());
        evtColl.m_CellPositionY.push_back(cell.y());
        evtColl.m_CellPositionZ.push_back(cell.z());
        if (registry.IsEnabled(AnalysisFlag::Voxelized)) {
          evtColl.m_VoxelPositionX.push_back(center.x());
          evtColl.m_VoxelPositionY.push_back(center.y());
          evtColl.m_VoxelPositionZ.push_back(center.z());
        }
      }

      if (registry.IsEnabled(AnalysisFlag::StoreEnergies)) {
        evtColl.m_VoxelHitEDeposit.push_back(eDep);
        evtColl.m_VoxelHitMeanEDeposit.push_back(meanEDep);
      }

      if (registry.IsEnabled(AnalysisFlag::StorePrimaries)) {
        const auto& primE = hit->GetEvtPrimariesEnergy();
        evtColl.m_G4EvtPrimaryEnergy.assign(primE.begin(), primE.end());
        evtColl.m_EvtPrimariesN = hit->GetEvtNPrimaries();
      }

      if (registry.IsEnabled(AnalysisFlag::StoreTracks)) {
        const auto& trkMap = hit->GetTrackIdTypeMappingList();
        const auto& procMap = hit->GetTrkIdProcessTypeMappingList();
        const auto& origMap = hit->GetTrkIdElectronOriginTypeMappingList();

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
        for (const auto& [_, val] : hit->GetTrkIdEnergyMappingList()) eList.push_back(val / keV);
        for (const auto& [_, val] : hit->GetTrkIdThetaMappingList()) thetaList.push_back(val);
        for (const auto& [_, val] : hit->GetTrkIdLengthMappingList()) lenList.push_back(val);
        for (const auto& [_, pos] : hit->GetTrkIdPositionMappingList()) {
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
      hits_evt_global_time.push_back(hit->GetGlobalTime());
    }
  }

  if (registry.IsEnabled(AnalysisFlag::StorePrimaries) && !hits_evt_global_time.empty()) {
    evtColl.m_global_time = *std::max_element(hits_evt_global_time.begin(), hits_evt_global_time.end());
  }
}
////////////////////////////////////////////////////////////////////////////////
/// Refactored FillNTupleEvent to use AnalysisFlags instead of legacy flags
void NTupleEventAnalisys::FillNTupleEvent() {
  auto analysisManager = G4AnalysisManager::Instance();

  for (auto coll = m_ntuple_collection.Begin(); coll != m_ntuple_collection.End(); ++coll) {
    const auto& treeName = coll->first;
    auto& registry = AnalysisFlagRegistry::Instance();

    auto& treeEvtColl = coll->second;
    const auto ntupleId = treeEvtColl.m_ntupleId;
    const G4int nHits = treeEvtColl.m_CellIdX.size();

    // AnalysisFlagRegistry::Instance().PrintAllFlags();

    // std::cout << "Voxelized: " << registry.IsEnabled(AnalysisFlag::Voxelized) << std::endl;
    // std::cout << "MinimalMode: " << registry.IsEnabled(AnalysisFlag::MinimalMode) << std::endl;
    // std::cout << "StoreRunInfo: " << registry.IsEnabled(AnalysisFlag::StoreRunInfo) << std::endl;
    // std::cout << "StorePositions: " << registry.IsEnabled(AnalysisFlag::StorePositions) << std::endl;
    // std::cout << "StoreEnergies: " << registry.IsEnabled(AnalysisFlag::StoreEnergies) << std::endl;

    if (nHits == 0) continue;  // empty event

    auto fillI = [&](const char* name, G4int val) { analysisManager->FillNtupleIColumn(ntupleId, treeEvtColl.m_colId[name], val); };
    auto fillD = [&](const char* name, G4double val) { analysisManager->FillNtupleDColumn(ntupleId, treeEvtColl.m_colId[name], val); };

    for (int i = 0; i < nHits; ++i) {
      fillI("CellIdX", treeEvtColl.m_CellIdX.at(i));
      fillI("CellIdY", treeEvtColl.m_CellIdY.at(i));
      fillI("CellIdZ", treeEvtColl.m_CellIdZ.at(i));
      fillD("CellDose", treeEvtColl.m_CellIDose.at(i));
      if (registry.IsEnabled(AnalysisFlag::Voxelized)) {
        fillI("VoxelIdX", treeEvtColl.m_VoxelIdX.at(i));
        fillI("VoxelIdY", treeEvtColl.m_VoxelIdY.at(i));
        fillI("VoxelIdZ", treeEvtColl.m_VoxelIdZ.at(i));
        fillD("VoxelDose", treeEvtColl.m_VoxelHitDose.at(i));
      }

      if (!registry.IsEnabled(AnalysisFlag::MinimalMode)) {
        fillI("G4EvtId", treeEvtColl.m_evtId);
        fillD("G4EvtGlobalTime", treeEvtColl.m_global_time);
        if (registry.IsEnabled(AnalysisFlag::StoreRunInfo)) {
          fillI("ThreadId", G4Threading::G4GetThreadId());
          fillI("G4RunId", m_runId);
          fillD("GantryAngle", m_degree_rotation);
        }
        if (registry.IsEnabled(AnalysisFlag::StorePositions)) {
          fillD("CellPositionX", treeEvtColl.m_CellPositionX.at(i));
          fillD("CellPositionY", treeEvtColl.m_CellPositionY.at(i));
          fillD("CellPositionZ", treeEvtColl.m_CellPositionZ.at(i));
          if (registry.IsEnabled(AnalysisFlag::Voxelized)) {
            fillD("VoxelPositionX", treeEvtColl.m_VoxelPositionX.at(i));
            fillD("VoxelPositionY", treeEvtColl.m_VoxelPositionY.at(i));
            fillD("VoxelPositionZ", treeEvtColl.m_VoxelPositionZ.at(i));
          }
        }
        if (registry.IsEnabled(AnalysisFlag::StoreEnergies)) {
          fillD("CellEDeposit", treeEvtColl.m_VoxelHitEDeposit.at(i));
          fillD("CellMeanEDeposit", treeEvtColl.m_VoxelHitMeanEDeposit.at(i));
          if (registry.IsEnabled(AnalysisFlag::Voxelized)) {
            fillD("VoxelEDeposit", treeEvtColl.m_VoxelHitEDeposit.at(i));
            fillD("VoxelMeanEDeposit", treeEvtColl.m_VoxelHitMeanEDeposit.at(i));
          }
        }
        if (registry.IsEnabled(AnalysisFlag::StoreTracks)) {
          treeEvtColl.m_VoxelTrkId.clear();
          treeEvtColl.m_VoxelTrkId = treeEvtColl.m_VoxelHitsTrkId.at(i);

          treeEvtColl.m_VoxelTrkTypeId.clear();
          treeEvtColl.m_VoxelTrkTypeId = treeEvtColl.m_VoxelHitsTrkTypeId.at(i);

          treeEvtColl.m_ProcessTypeId.clear();
          treeEvtColl.m_ProcessTypeId = treeEvtColl.m_VoxelHitsProcessTypeId.at(i);

          treeEvtColl.m_ElectronOriginTypeId.clear();
          treeEvtColl.m_ElectronOriginTypeId = treeEvtColl.m_VoxelHitsElectronOriginTypeId.at(i);

          treeEvtColl.m_VoxelTrkEnergy.clear();
          treeEvtColl.m_VoxelTrkEnergy = treeEvtColl.m_VoxelHitsTrkEnergy.at(i);

          treeEvtColl.m_VoxelTrkTheta.clear();
          treeEvtColl.m_VoxelTrkTheta = treeEvtColl.m_VoxelHitsTrkTheta.at(i);

          treeEvtColl.m_VoxelTrkLength.clear();
          treeEvtColl.m_VoxelTrkLength = treeEvtColl.m_VoxelHitsTrkLength.at(i);

          treeEvtColl.m_VoxelTrkPositionX.clear();
          treeEvtColl.m_VoxelTrkPositionX = treeEvtColl.m_VoxelHitsTrkPosX.at(i);

          treeEvtColl.m_VoxelTrkPositionY.clear();
          treeEvtColl.m_VoxelTrkPositionY = treeEvtColl.m_VoxelHitsTrkPosY.at(i);

          treeEvtColl.m_VoxelTrkPositionZ.clear();
          treeEvtColl.m_VoxelTrkPositionZ = treeEvtColl.m_VoxelHitsTrkPosZ.at(i);
        }
      }
      analysisManager->AddNtupleRow(ntupleId);
    }
  }
}

////////////////////////////////////////////////////////////////////////////////
/// Refactored ClearEventCollections to respect flag logic
void NTupleEventAnalisys::ClearEventCollections() {
  for (auto coll = m_ntuple_collection.Begin(); coll != m_ntuple_collection.End(); ++coll) {
    const auto& treeName = coll->first;
    auto& registry = AnalysisFlagRegistry::Instance();

    auto& evt = coll->second;

    evt.m_evtId = -1;
    evt.m_global_time = 0.;
    evt.m_G4EvtPrimaryEnergy.clear();
    evt.m_EvtPrimariesN = 0;
    evt.m_CellIdX.clear();
    evt.m_CellIdY.clear();
    evt.m_CellIdZ.clear();
    evt.m_CellIDose.clear();
    evt.m_CellPositionX.clear();
    evt.m_CellPositionY.clear();
    evt.m_CellPositionZ.clear();
    evt.m_VoxelHitEDeposit.clear();
    evt.m_VoxelIdX.clear();
    evt.m_VoxelIdY.clear();
    evt.m_VoxelIdZ.clear();
    evt.m_VoxelHitDose.clear();
    evt.m_VoxelPositionX.clear();
    evt.m_VoxelPositionY.clear();
    evt.m_VoxelPositionZ.clear();
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


////////////////////////////////////////////////////////////////////////////////
G4int NTupleEventAnalisys::GetNTupleId(const G4String& treeName) {
  auto exists = m_ntuple_collection.Has(treeName);
  if (exists) {
    auto evtCollection = m_ntuple_collection.Get(treeName);
    return evtCollection.m_ntupleId;
  }
  return -1;
}
////////////////////////////////////////////////////////////////////////////////
/// This member is called at the end of every event from
/// EventAction::EndOfEventAction
void NTupleEventAnalisys::EndOfEventAction(const G4Event* evt) {
  auto hCofThisEvent = evt->GetHCofThisEvent();
  if (!hCofThisEvent) return;

  ClearEventCollections();

  for (const auto& tree : m_ttree_collection.Get()) {
    for (const auto& hc : tree.m_hc_names) {
      const auto fullName = tree.m_name + m_treeNamePostfix;

      auto collection_id = G4SDManager::GetSDMpointer()->GetCollectionID(hc);
      if (collection_id < 0) {
        G4cout << "[ERROR]:: NTupleEventAnalisys::EndOfEventAction TTree: " << fullName << " G4SDManager err: " << collection_id << G4endl;
        continue;
      }

      auto hitsColl = dynamic_cast<VoxelHitsCollection*>(hCofThisEvent->GetHC(collection_id));
      if (hitsColl) {
        FillEventCollection(fullName, evt, hitsColl);
      }
    }
  }

  FillNTupleEvent();
}
