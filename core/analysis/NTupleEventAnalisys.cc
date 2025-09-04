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
AnalysisFlagRegistry* NTupleEventAnalisys::m_analysis_reg = AnalysisFlagRegistry::Instance();

////////////////////////////////////////////////////////////////////////////////
/**
 * @brief Returns the singleton instance of NTupleEventAnalisys.
 *
 * Ensures a single global instance of the NTupleEventAnalisys class is used throughout the application.
 *
 * @return Pointer to the singleton NTupleEventAnalisys instance.
 */
NTupleEventAnalisys* NTupleEventAnalisys::GetInstance() {
  static NTupleEventAnalisys instance;
  return &instance;
}

////////////////////////////////////////////////////////////////////////////////
/**
 * @brief Register a TTree definition used for per-event ntuple output.
 *
 * Registers a tree definition that maps one or more hit collections (HCs) to a named ntuple. If `hcName` is empty the tree is treated as a single-scoring-volume tree (the tree name is used as the HC name); otherwise the `hcName` is added to a shared tree entry (creating it if needed). If `cellVoxelisation` is true the global voxelized analysis flag is enabled. No action is performed if ntuple analysis is disabled in the run configuration.
 *
 * @param treeName Logical name of the TTree (also used as HC name for single-volume trees).
 * @param cellVoxelisation When true, enable voxel-aware columns for this tree.
 * @param hcName Hit-collection name to associate with the tree; empty => single-volume tree.
 * @param treeDescription Human-readable description stored with the tree definition.
 */
void NTupleEventAnalisys::DefineTTree(const G4String& treeName, bool cellVoxelisation, const G4String& hcName, const G4String& treeDescription) {
  if (!Service<ConfigSvc>()->GetValue<bool>("RunSvc", "NTupleAnalysis")) return;

  std::vector<TTreeCollection>& ttreeVec = m_ttree_collection.Get();

  if (hcName.empty()) {
    ANA_INFO("Defining TTree: {} (single scoring volume)", treeName);

    // Create new entry for single-scoring-volume case
    ttreeVec.emplace_back();
    auto& tree = ttreeVec.back();
    tree.m_name = treeName;
    tree.m_description = treeDescription;
    tree.m_hc_names.emplace_back(treeName);

    if (cellVoxelisation) {
      m_analysis_reg->SetFlag(AnalysisFlag::Voxelized, true);
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
      ANA_INFO("Defining TTree: {} (new shared tree)", treeName);
      ttreeVec.emplace_back();
      auto& tree = ttreeVec.back();
      tree.m_name = treeName;
      tree.m_description = treeDescription;
      tree.m_hc_names.emplace_back(hcName);

      if (cellVoxelisation) {
        m_analysis_reg->SetFlag(AnalysisFlag::Voxelized, true);
      }

    } else {
      // Append HC name to existing tree
      ttreeVec.at(treeIdx).m_hc_names.emplace_back(hcName);
    }
  }
}

////////////////////////////////////////////////////////////////////////////////
/**
 * @brief Initialize analysis flags and ensure ntuples exist for the run.
 *
 * Reads ntuple-related configuration, updates the global AnalysisFlagRegistry accordingly,
 * stores run-specific metadata (run ID and current gantry rotation), and creates any
 * missing ntuples for all previously defined TTrees. If NTuple analysis is disabled
 * in configuration, the call is a no-op.
 *
 * @param runPtr Pointer to the current Geant4 run (must be non-null).
 */
void NTupleEventAnalisys::BeginOfRun(const G4Run* runPtr, G4bool /*isMaster*/) {

  auto setAnalysisFlag = [](AnalysisFlag which, bool enable) {
      NTupleEventAnalisys::m_analysis_reg->SetFlag(which, enable);
  };

  if (!Service<ConfigSvc>()->GetValue<bool>("RunSvc", "NTupleAnalysis")) return;

  setAnalysisFlag(AnalysisFlag::StoreRunInfo, Service<ConfigSvc>()->GetValue<bool>("RunSvc", "StoreRunInfo"));
  setAnalysisFlag(AnalysisFlag::StorePositions, Service<ConfigSvc>()->GetValue<bool>("RunSvc", "StorePositions"));
  setAnalysisFlag(AnalysisFlag::StoreEnergies, Service<ConfigSvc>()->GetValue<bool>("RunSvc", "StoreEnergies"));
  setAnalysisFlag(AnalysisFlag::StoreTracks, Service<ConfigSvc>()->GetValue<bool>("RunSvc", "StoreTracks"));
  setAnalysisFlag(AnalysisFlag::StorePrimaries, Service<ConfigSvc>()->GetValue<bool>("RunSvc", "StorePrimaries"));
  setAnalysisFlag(AnalysisFlag::MinimalMode, Service<ConfigSvc>()->GetValue<bool>("RunSvc", "MinimalMode"));

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
/**
 * @brief Create and configure an ntuple for the given tree definition.
 *
 * Adds an ntuple to the analysis manager and defines its columns according to
 * the enabled analysis flags (voxelization, minimal/detailed mode, run info,
 * positions, energies, tracks, primaries). Columns include cell identifiers
 * and dose (always), optional voxel identifiers and dose, and—when not in
 * MinimalMode—per-event metadata, positions, energy deposits, per-voxel track
 * vectors, and primary-particle info. The ntuple is finalized with FinishNtuple.
 *
 * @param treeColl Description of the ntuple to create (name, description, and
 *                 associated hit-collection metadata used to form the ntuple).
 */
void NTupleEventAnalisys::CreateNTuple(const TTreeCollection& treeColl) {
  const auto treeName = treeColl.m_name + m_treeNamePostfix;

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
  if (m_analysis_reg->IsEnabled(AnalysisFlag::Voxelized)) {
    createI("VoxelIdX");
    createI("VoxelIdY");
    createI("VoxelIdZ");
    createD("VoxelDose");
  }

  if (!m_analysis_reg->IsEnabled(AnalysisFlag::MinimalMode)) {
    createI("G4EvtId");
    createD("G4EvtGlobalTime");

    // General per-event branches (only in detailed mode)
    if (m_analysis_reg->IsEnabled(AnalysisFlag::StoreRunInfo)) {
      createI("ThreadId");
      createI("G4RunId");
      createD("GantryAngle");
    }

    // Cell-level scoring info
    if (m_analysis_reg->IsEnabled(AnalysisFlag::StorePositions)) {
      createD("CellPositionX");
      createD("CellPositionY");
      createD("CellPositionZ");
      if (m_analysis_reg->IsEnabled(AnalysisFlag::Voxelized)) {
        createD("VoxelPositionX");
        createD("VoxelPositionY");
        createD("VoxelPositionZ");
      }
    }

    if (m_analysis_reg->IsEnabled(AnalysisFlag::StoreEnergies)) {
      createD("CellEDeposit");
      createD("CellMeanEDeposit");
      if (m_analysis_reg->IsEnabled(AnalysisFlag::Voxelized)) {
        createD("VoxelEDeposit");
        createD("VoxelMeanEDeposit");
      }
    }

    // Voxel-level branches
    if (m_analysis_reg->IsEnabled(AnalysisFlag::StoreTracks)) {
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
    if (m_analysis_reg->IsEnabled(AnalysisFlag::StorePrimaries)) {
      createD("G4EvtPrimaryE");
      createI("G4EvtPrimaryN");
    }
  }
  analysisManager->FinishNtuple(evtNTupleColl.m_ntupleId);
}


////////////////////////////////////////////////////////////////////////////////
/**
 * @brief Extracts hit data from a VoxelHitsCollection and stores it in the internal event buffers for a specific ntuple.
 *
 * Populates the per-event data structure associated with the given tree name using entries from the provided VoxelHitsCollection.
 * The function applies the configured world-to-simulation coordinate transformation and records fields according to enabled analysis flags:
 * - Always stored: cell global IDs and scaled cell dose.
 * - If voxelized: voxel IDs and per-voxel dose.
 * - If not in MinimalMode: optional positions, per-voxel energy deposits, primary particle info (energies and count), and detailed per-track data (IDs, type/process/origin IDs, energies, angles, lengths, and track positions).
 *
 * If the tree name is not associated with a created ntuple (GetNTupleId returns -1), the function logs a warning and returns without modifying internal state.
 *
 * @param treeName Name of the ntuple/tree whose event buffers will be filled.
 * @param evt Pointer to the current G4Event (used to set the event ID).
 * @param hitsColl Pointer to the VoxelHitsCollection supplying per-hit data.
 */
void NTupleEventAnalisys::FillEventCollection(const G4String& treeName, const G4Event* evt, VoxelHitsCollection* hitsColl) {
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

    if (m_analysis_reg->IsEnabled(AnalysisFlag::Voxelized)) {
      evtColl.m_VoxelIdX.push_back(hit->GetID(0));
      evtColl.m_VoxelIdY.push_back(hit->GetID(1));
      evtColl.m_VoxelIdZ.push_back(hit->GetID(2));
      evtColl.m_VoxelHitDose.push_back(dose);
    }

    if (!m_analysis_reg->IsEnabled(AnalysisFlag::MinimalMode)) {
      if (m_analysis_reg->IsEnabled(AnalysisFlag::StorePositions)) {
        evtColl.m_CellPositionX.push_back(cell.x());
        evtColl.m_CellPositionY.push_back(cell.y());
        evtColl.m_CellPositionZ.push_back(cell.z());
        if (m_analysis_reg->IsEnabled(AnalysisFlag::Voxelized)) {
          evtColl.m_VoxelPositionX.push_back(center.x());
          evtColl.m_VoxelPositionY.push_back(center.y());
          evtColl.m_VoxelPositionZ.push_back(center.z());
        }
      }

      if (m_analysis_reg->IsEnabled(AnalysisFlag::StoreEnergies)) {
        evtColl.m_VoxelHitEDeposit.push_back(eDep);
        evtColl.m_VoxelHitMeanEDeposit.push_back(meanEDep);
      }

      if (m_analysis_reg->IsEnabled(AnalysisFlag::StorePrimaries)) {
        const auto& primE = hit->GetEvtPrimariesEnergy();
        evtColl.m_G4EvtPrimaryEnergy.assign(primE.begin(), primE.end());
        evtColl.m_EvtPrimariesN = hit->GetEvtNPrimaries();
      }

      if (m_analysis_reg->IsEnabled(AnalysisFlag::StoreTracks)) {
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

  if (m_analysis_reg->IsEnabled(AnalysisFlag::StorePrimaries) && !hits_evt_global_time.empty()) {
    evtColl.m_global_time = *std::max_element(hits_evt_global_time.begin(), hits_evt_global_time.end());
  }
}
////////////////////////////////////////////////////////////////////////////////
/**
 * @brief Fills ntuple rows with event data for all defined ntuple collections.
 *
 * Iterates over all stored event data collections and populates the corresponding ntuple columns in the Geant4 analysis manager. The columns filled depend on the enabled analysis flags, including cell and voxel information, event and run metadata, positions, energies, and detailed track data. Adds a row to the ntuple for each hit in the event.
 */
void NTupleEventAnalisys::FillNTupleEvent() {
  auto analysisManager = G4AnalysisManager::Instance();

  for (auto coll = m_ntuple_collection.Begin(); coll != m_ntuple_collection.End(); ++coll) {
    const auto& treeName = coll->first;

    auto& treeEvtColl = coll->second;
    const auto ntupleId = treeEvtColl.m_ntupleId;
    const G4int nHits = treeEvtColl.m_CellIdX.size();

    if (nHits == 0) continue;  // empty event

    auto fillI = [&](const char* name, G4int val) { analysisManager->FillNtupleIColumn(ntupleId, treeEvtColl.m_colId[name], val); };
    auto fillD = [&](const char* name, G4double val) { analysisManager->FillNtupleDColumn(ntupleId, treeEvtColl.m_colId[name], val); };

    for (int i = 0; i < nHits; ++i) {
      fillI("CellIdX", treeEvtColl.m_CellIdX.at(i));
      fillI("CellIdY", treeEvtColl.m_CellIdY.at(i));
      fillI("CellIdZ", treeEvtColl.m_CellIdZ.at(i));
      fillD("CellDose", treeEvtColl.m_CellIDose.at(i));
      if (m_analysis_reg->IsEnabled(AnalysisFlag::Voxelized)) {
        fillI("VoxelIdX", treeEvtColl.m_VoxelIdX.at(i));
        fillI("VoxelIdY", treeEvtColl.m_VoxelIdY.at(i));
        fillI("VoxelIdZ", treeEvtColl.m_VoxelIdZ.at(i));
        fillD("VoxelDose", treeEvtColl.m_VoxelHitDose.at(i));
      }

      if (!m_analysis_reg->IsEnabled(AnalysisFlag::MinimalMode)) {
        fillI("G4EvtId", treeEvtColl.m_evtId);
        fillD("G4EvtGlobalTime", treeEvtColl.m_global_time);
        if (m_analysis_reg->IsEnabled(AnalysisFlag::StoreRunInfo)) {
          fillI("ThreadId", G4Threading::G4GetThreadId());
          fillI("G4RunId", m_runId);
          fillD("GantryAngle", m_degree_rotation);
        }
        if (m_analysis_reg->IsEnabled(AnalysisFlag::StorePositions)) {
          fillD("CellPositionX", treeEvtColl.m_CellPositionX.at(i));
          fillD("CellPositionY", treeEvtColl.m_CellPositionY.at(i));
          fillD("CellPositionZ", treeEvtColl.m_CellPositionZ.at(i));
          if (m_analysis_reg->IsEnabled(AnalysisFlag::Voxelized)) {
            fillD("VoxelPositionX", treeEvtColl.m_VoxelPositionX.at(i));
            fillD("VoxelPositionY", treeEvtColl.m_VoxelPositionY.at(i));
            fillD("VoxelPositionZ", treeEvtColl.m_VoxelPositionZ.at(i));
          }
        }
        if (m_analysis_reg->IsEnabled(AnalysisFlag::StoreEnergies)) {
          fillD("CellEDeposit", treeEvtColl.m_VoxelHitEDeposit.at(i));
          fillD("CellMeanEDeposit", treeEvtColl.m_VoxelHitMeanEDeposit.at(i));
          if (m_analysis_reg->IsEnabled(AnalysisFlag::Voxelized)) {
            fillD("VoxelEDeposit", treeEvtColl.m_VoxelHitEDeposit.at(i));
            fillD("VoxelMeanEDeposit", treeEvtColl.m_VoxelHitMeanEDeposit.at(i));
          }
        }
        if (m_analysis_reg->IsEnabled(AnalysisFlag::StoreTracks)) {
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
/**
 * @brief Clears all stored event data for each ntuple collection.
 *
 * Resets event IDs, counters, and all data vectors in every ntuple event collection, preparing for the next event.
 */
void NTupleEventAnalisys::ClearEventCollections() {
  for (auto coll = m_ntuple_collection.Begin(); coll != m_ntuple_collection.End(); ++coll) {
    const auto& treeName = coll->first;

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

/**
 * @brief Retrieves the ntuple ID associated with a given tree name.
 *
 * @param treeName Name of the ntuple tree.
 * @return The ntuple ID if the tree exists; otherwise, returns -1.
 */
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
/**
 * @brief Handle end-of-event processing: collect hit data and write ntuples.
 *
 * Iterates the configured TTree definitions, looks up each configured hit collection
 * in the event, fills the internal per-event data structures from any found
 * VoxelHitsCollection, and writes all collected data into the configured ntuples.
 * If the event has no HCofThisEvent, processing is skipped. Invalid collection IDs
 * are ignored and processing continues for other collections.
 *
 * Side effects:
 * - Clears and repopulates internal event buffers.
 * - Writes rows to the configured ntuples via the analysis manager.
 *
 * @param evt Pointer to the G4Event being finalized (must be non-null).
 */
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
