//
// Created by brachwal on 30.05.2020.
//
// Updated and refactored for clarity by jhajduga
// 26.06.2025

// ========================= NTupleEventAnalisys.hh ========================= //
#ifndef D3D_EVENT_ANALYSIS_HH
#define D3D_EVENT_ANALYSIS_HH

#include "AnalysisFlags.hh"
#include "G4Cache.hh"
#include "VoxelHit.hh"
#include "tls.hh"
#include <map>
#include <vector>

class G4Event;
class G4Run;

class NTupleEventAnalisys {
 private:
  NTupleEventAnalisys() = default;
  ~NTupleEventAnalisys() = default;

  NTupleEventAnalisys(const NTupleEventAnalisys&) = delete;
  NTupleEventAnalisys& operator=(const NTupleEventAnalisys&) = delete;
  NTupleEventAnalisys(NTupleEventAnalisys&&) = delete;
  NTupleEventAnalisys& operator=(NTupleEventAnalisys&&) = delete;

  /// Event-local data collection (filled per event)
  class TTreeEventCollection {
   public:
    G4int m_evtId = -1;
    G4int m_ntupleId = -1;
    std::map<G4String, G4int> m_colId;
    G4double m_global_time = 0.;
    G4int m_EvtPrimariesN = 0;

    std::vector<G4int> m_CellIdX, m_CellIdY, m_CellIdZ;
    std::vector<G4int> m_VoxelIdX, m_VoxelIdY, m_VoxelIdZ;
    std::vector<G4int> m_VoxelHitIdX, m_VoxelHitIdY, m_VoxelHitIdZ;

    std::vector<G4double> m_CellIDose;
    std::vector<G4double> m_VoxelHitEDeposit, m_VoxelHitMeanEDeposit, m_VoxelHitDose;
    std::vector<G4double> m_PrimaryTrkEnergy, m_EvtTime, m_G4EvtPrimaryEnergy;

    std::vector<G4double> m_VoxelPositionX, m_VoxelPositionY, m_VoxelPositionZ;
    std::vector<G4double> m_CellPositionX, m_CellPositionY, m_CellPositionZ;

    std::vector<std::vector<G4int>> m_VoxelHitsTrkId;
    std::vector<G4int> m_VoxelTrkId;

    std::vector<std::vector<G4int>> m_VoxelHitsTrkTypeId;
    std::vector<G4int> m_VoxelTrkTypeId;

    std::vector<std::vector<G4int>> m_VoxelHitsProcessTypeId;
    std::vector<G4int> m_ProcessTypeId;

    std::vector<std::vector<G4int>> m_VoxelHitsElectronOriginTypeId;
    std::vector<G4int> m_ElectronOriginTypeId;

    std::vector<std::vector<G4double>> m_VoxelHitsTrkEnergy;
    std::vector<G4double> m_VoxelTrkEnergy;

    std::vector<std::vector<G4double>> m_VoxelHitsTrkTheta;
    std::vector<G4double> m_VoxelTrkTheta;

    std::vector<std::vector<G4double>> m_VoxelHitsTrkLength;
    std::vector<G4double> m_VoxelTrkLength;

    std::vector<std::vector<G4double>> m_VoxelHitsTrkPosX;
    std::vector<G4double> m_VoxelTrkPositionX;

    std::vector<std::vector<G4double>> m_VoxelHitsTrkPosY;
    std::vector<G4double> m_VoxelTrkPositionY;

    std::vector<std::vector<G4double>> m_VoxelHitsTrkPosZ;
    std::vector<G4double> m_VoxelTrkPositionZ;
  };

  /// Persistent tree-level configuration (flags, branches, names...)
  class TTreeCollection {
   public:
    G4String m_name;
    G4String m_description;
    std::vector<G4String> m_hc_names;
    AnalysisFlags flags;  ///< Unified set of tree-level configuration flags
  };

  static G4Cache<std::vector<TTreeCollection>> m_ttree_collection;
  G4MapCache<G4String, TTreeEventCollection> m_ntuple_collection;

  G4int m_runId = -1;
  G4double m_degree_rotation = -10000.;
  G4String m_treeNamePostfix = "TTree";
  static AnalysisFlagRegistry* m_analysis_reg;

  void CreateNTuple(const TTreeCollection& treeColl);
  G4int GetNTupleId(const G4String& treeName);
  void FillEventCollection(const G4String& treeName, const G4Event* evt, VoxelHitsCollection* hitsColl);
  void FillNTupleEvent();
  void ClearEventCollections();

 public:
  static NTupleEventAnalisys* GetInstance();

  void BeginOfRun(const G4Run* runPtr, G4bool isMaster);
  void EndOfEventAction(const G4Event* evt);

  static void DefineTTree(const G4String& treeName, bool cellVoxelisation = false, const G4String& hcName = G4String(), const G4String& treeDescription = G4String());


  static G4bool IsAnyTTreeDefined() { return m_ttree_collection.Get().empty() ? false : true; }
  static const std::vector<TTreeCollection>& TreeCollection() { return m_ttree_collection.Get(); }

  G4ThreadLocal static NTupleEventAnalisys* fInstance;
};

#endif  // D3D_EVENT_ANALYSIS_HH
