//
// Created by brachwal on 30.05.2020.
//

#include "NTupleEventAnalisys.hh"
#include "VPatient.hh"
#include "G4Event.hh"
#include "G4Run.hh"
#include "G4SDManager.hh"
#include "G4AnalysisManager.hh"
#include "VoxelHit.hh"
#include "G4Threading.hh"
#include "Services.hh"
#include "D3DCell.hh"
#include <algorithm>
#include <vector>
#include <set>

G4Cache<std::vector<NTupleEventAnalisys::TTreeCollection>> NTupleEventAnalisys::m_ttree_collection;

////////////////////////////////////////////////////////////////////////////////
///
NTupleEventAnalisys *NTupleEventAnalisys::GetInstance() {
  static NTupleEventAnalisys instance = NTupleEventAnalisys();
  return &instance;
}
////////////////////////////////////////////////////////////////////////////////
///
void NTupleEventAnalisys::DefineTTree(const G4String& treeName, bool cellVoxelisation, 
                                      const G4String& hcName, const G4String& treeDescription) {
  if (Service<ConfigSvc>()->GetValue<bool>("RunSvc", "NTupleAnalysis") == false)
    return;

  // Obtain a reference to the underlying vector from the cache
  std::vector<TTreeCollection>& ttreeVec = m_ttree_collection.Get();

  if(hcName.empty()){
    LOGSVC_INFO("Defining TTree: {}",treeName);
    // Given tree is related to single scoring volume (hits collection)
    ttreeVec.emplace_back(TTreeCollection());
    ttreeVec.back().m_name = treeName;
    ttreeVec.back().m_hc_names.emplace_back(treeName);
    ttreeVec.back().m_description = treeDescription;
    ttreeVec.back().m_voxel_tree_structure = cellVoxelisation;
  }
  else {
    G4int treeIdx = -1;
    G4bool treeExists = false;
    for(const auto& tree : ttreeVec){
      ++treeIdx;
        if (tree.m_name == treeName){
        treeExists = true;
        break;
        }
    }
    if (!treeExists){
      LOGSVC_INFO("Defining TTree: {}", treeName);
      ttreeVec.emplace_back(TTreeCollection());
      ttreeVec.back().m_name = treeName;
      ttreeVec.back().m_description = treeDescription;
      ttreeVec.back().m_hc_names.emplace_back(hcName);
      ttreeVec.back().m_voxel_tree_structure = cellVoxelisation;
    } else {
    ttreeVec.at(treeIdx).m_hc_names.emplace_back(hcName);
    }
  }
}

////////////////////////////////////////////////////////////////////////////////
///
void NTupleEventAnalisys::SetTracksAnalysis(const G4String& treeName, bool flag){
  if (Service<ConfigSvc>()->GetValue<bool>("RunSvc", "NTupleAnalysis") == false)
    return;
  for(auto& tree : m_ttree_collection.Get()){
    if (tree.m_name==treeName){
      //G4cout<< "[INFO]:: NTupleEventAnalisys:: Set Tracks Analysis for " << treeName << " to:" << flag << G4endl;
      tree.m_tracks_analysis = flag;
    }
  }
}

////////////////////////////////////////////////////////////////////////////////
///
void NTupleEventAnalisys::BeginOfRun(const G4Run* runPtr, G4bool isMaster){
  if (Service<ConfigSvc>()->GetValue<bool>("RunSvc", "NTupleAnalysis") == false)
    return;
  // Book Voxel data Ntuple for all HitsColletions
  //------------------------------------------
  m_runId = runPtr->GetRunID();
  auto runSvc = Service<RunSvc>();
  auto control_point = runSvc->CurrentControlPoint();
  m_degree_rotation = control_point->GetDegreeRotation();
  
  // Get the underlying vector from the cache
  const std::vector<TTreeCollection>& ttreeVec = m_ttree_collection.Get();
  for(const auto& tree : ttreeVec){
    if(GetNTupleId(tree.m_name + m_treeNamePostfix) == -1){ // Not created yet
      CreateNTuple(tree);
    }
  }
}


////////////////////////////////////////////////////////////////////////////////
///
// void NTupleEventAnalisys::CreateNTuple(const G4String& treeName, const G4String& treeDescription){
void NTupleEventAnalisys::CreateNTuple(const TTreeCollection& treeColl){
  auto treeName = treeColl.m_name+m_treeNamePostfix;
  auto treeDescription = treeColl.m_description;
  // G4cout << "[DEUBG]:: NTupleEventAnalisys::Creating TTree: "<< treeName << ":"  << treeDescription << G4endl;
  auto analysisManager =  G4AnalysisManager::Instance();
  m_ntuple_collection.Insert(treeName,TTreeEventCollection());
  auto& evtNTupleColl = m_ntuple_collection.Get(treeName);
  auto ntupleId = analysisManager->CreateNtuple(treeName,treeDescription);
  evtNTupleColl.m_ntupleId = ntupleId;
  evtNTupleColl.m_tracks_analysis = treeColl.m_tracks_analysis;
  evtNTupleColl.m_minimalistic_ttree = treeColl.m_minimalistic;
  evtNTupleColl.m_voxel_tree_structure = treeColl.m_voxel_tree_structure;

  /* NOTE:
    The G4AnalysisManager CreateNtuple(X)Column creates each column with unique ID,
    being inremented in each method call. Once we want to call FillNtuple(X)Column
    in order to set value explicitly we have to know ID matched to the defined column.
    Here it's why we create the map of ID to each column...
  */
  auto createNtupleIColumn = [&](const char* name){
    evtNTupleColl.m_colId[name] = analysisManager->CreateNtupleIColumn(ntupleId, name); 
  };
  auto createNtupleDColumn = [&](const char* name){
    evtNTupleColl.m_colId[name] = analysisManager->CreateNtupleDColumn(ntupleId, name); 
  };

  auto createNtupleVecIColumn = [&](const char* name,std::vector<G4int>& vec){
    evtNTupleColl.m_colId[name] = analysisManager->CreateNtupleIColumn(ntupleId, name, vec); 
  };

  auto createNtupleVecDColumn = [&](const char* name,std::vector<G4double>& vec){
    evtNTupleColl.m_colId[name] = analysisManager->CreateNtupleDColumn(ntupleId, name, vec); 
  };
  
  // General TTree branches
  if(!treeColl.m_minimalistic){
    createNtupleIColumn("ThreadId");
    createNtupleIColumn("G4EvtId");
    createNtupleIColumn("G4RunId");
    createNtupleDColumn("G4EvtGlobalTime");
    createNtupleVecDColumn("G4EvtPrimaryE",evtNTupleColl.m_G4EvtPrimaryEnergy);
    createNtupleIColumn("G4EvtPrimaryN");
    createNtupleDColumn("GantryAngle");
  }

  // Dose-3D cell level scoring
  if(evtNTupleColl.m_cell_tree_structure){
    createNtupleIColumn("CellIdX");
    createNtupleIColumn("CellIdY");
    createNtupleIColumn("CellIdZ");
    if(!treeColl.m_minimalistic){
      createNtupleDColumn("CellPositionX");
      createNtupleDColumn("CellPositionY");
      createNtupleDColumn("CellPositionZ");
      createNtupleDColumn("CellEDeposit");
      createNtupleDColumn("CellMeanEDeposit");
    }
    createNtupleDColumn("CellDose");
  }

  // Any voxel-like scoring (+ Dose-3D scope)
  if(evtNTupleColl.m_voxel_tree_structure){
    createNtupleIColumn("VoxelIdX");
    createNtupleIColumn("VoxelIdY");
    createNtupleIColumn("VoxelIdZ");
    if(!treeColl.m_minimalistic){
      createNtupleDColumn("VoxelPositionX");
      createNtupleDColumn("VoxelPositionY");
      createNtupleDColumn("VoxelPositionZ");
      createNtupleDColumn("VoxelEDeposit");
      createNtupleDColumn("VoxelMeanEDeposit");
    }
    createNtupleDColumn("VoxelDose");
    
    if(evtNTupleColl.m_tracks_analysis){
      createNtupleVecIColumn("VoxelTrkId",evtNTupleColl.m_VoxelTrkId);
      createNtupleVecIColumn("VoxelTrkTypeId",evtNTupleColl.m_VoxelTrkTypeId);
      createNtupleVecDColumn("VoxelTrkE",evtNTupleColl.m_VoxelTrkEnergy);
      createNtupleVecDColumn("VoxelTrkTheta",evtNTupleColl.m_VoxelTrkTheta);
      createNtupleVecDColumn("VoxelTrkLength",evtNTupleColl.m_VoxelTrkLength);
      createNtupleVecDColumn("VoxelTrkPositionX",evtNTupleColl.m_VoxelTrkPositionX);
      createNtupleVecDColumn("VoxelTrkPositionY",evtNTupleColl.m_VoxelTrkPositionY);
      createNtupleVecDColumn("VoxelTrkPositionZ",evtNTupleColl.m_VoxelTrkPositionZ);
    }
  }
  analysisManager->FinishNtuple(ntupleId);
}

////////////////////////////////////////////////////////////////////////////////
///
G4int NTupleEventAnalisys::GetNTupleId(const G4String& treeName){
  auto exists = m_ntuple_collection.Has(treeName);
  if(exists){
    auto evtCollection = m_ntuple_collection.Get(treeName);
    return evtCollection.m_ntupleId;
  }
  return -1;
}

////////////////////////////////////////////////////////////////////////////////
///
void NTupleEventAnalisys::FillEventCollection(const G4String& treeName, const G4Event *evt, VoxelHitsCollection* hitsColl){
  auto isoToSim = Service<ConfigSvc>()->GetValue<G4ThreeVector>("WorldConstruction", "IsoToSimTransformation");
  auto analysisManager = G4AnalysisManager::Instance();
  auto ntplId = GetNTupleId(treeName);
  if (ntplId==-1){
    G4cout << " [WARNING]::FillEventCollection:: given TTree ("<< treeName <<") collection doesn't exists! " << G4endl;
    return;
  }
  auto& evtColl = m_ntuple_collection.Get(treeName); // Note: once the treeName is wrong this gives undefined beheviour!!
  int nHits = hitsColl->entries();
  // LOGSVC_INFO("nHist: {}",nHits);
  if(nHits==0){
   return; // no hits in this event
  }
  evtColl.m_evtId = evt->GetEventID();
  G4double totalEnergyDeposit = 0;
  G4double totalDose = 0;

  // std::cout << "EvtId: " << evt->GetEventID() << std::endl;
  std::vector<G4double> hits_evt_global_time;
  for (int i=0;i<nHits;i++){ // a.k.a. voxel loop

    auto hit = dynamic_cast<VoxelHit*>(hitsColl->GetHit(i));
    auto hitEDeposit = hit->GetEnergyDeposit() / keV;
    auto hitMeanEDeposit = hit->GetMeanEnergyDeposit() / keV;

    evtColl.m_CellIdX.push_back(hit->GetGlobalID(0));
    evtColl.m_CellIdY.push_back(hit->GetGlobalID(1));
    evtColl.m_CellIdZ.push_back(hit->GetGlobalID(2));
    
    if(!evtColl.m_minimalistic_ttree){
      auto cell_centre = hit->GetGlobalCentre() - isoToSim;
      evtColl.m_CellPositionX.push_back(cell_centre.x());
      evtColl.m_CellPositionY.push_back(cell_centre.y());
      evtColl.m_CellPositionZ.push_back(cell_centre.z());
    }

    auto voxelDose = hit->GetDose(); // note: it's in gray already;
    auto size = D3DCell::SIZE; 
    double cellVolume = pow(size,3);
    auto cellDose = voxelDose * hit->GetVolume() / cellVolume;
    evtColl.m_CellIDose.emplace_back( cellDose );

    // G4cout << hitsColl->GetName() << " dose: " << cellDose << G4endl;


    evtColl.m_VoxelIdX.push_back(hit->GetID(0));
    evtColl.m_VoxelIdY.push_back(hit->GetID(1));
    evtColl.m_VoxelIdZ.push_back(hit->GetID(2));

    if(!evtColl.m_minimalistic_ttree){
      auto hitCentre = (hit->GetCentre()) + hit->GetGlobalCentre();
      evtColl.m_VoxelPositionX.push_back(hitCentre.x());
      evtColl.m_VoxelPositionY.push_back(hitCentre.y());
      evtColl.m_VoxelPositionZ.push_back(hitCentre.z());
    }

    totalEnergyDeposit+=hitEDeposit;
    if(!evtColl.m_minimalistic_ttree){
      evtColl.m_VoxelHitEDeposit.emplace_back(hitEDeposit);
      evtColl.m_VoxelHitMeanEDeposit.emplace_back(hitMeanEDeposit);
    }
    evtColl.m_VoxelHitDose.emplace_back( voxelDose );

    if(!evtColl.m_minimalistic_ttree){
      auto evtPE = hit->GetEvtPrimariesEnergy();
      evtColl.m_G4EvtPrimaryEnergy.assign(evtPE.begin(),evtPE.end());
      evtColl.m_EvtPrimariesN = hit->GetEvtNPrimaries();
      hits_evt_global_time.emplace_back( hit->GetGlobalTime() );
    }


    if (evtColl.m_tracks_analysis){
      std::vector<G4int> trkType;
      std::vector<G4int> trkTypeId;
      for(const auto& iTrkType : hit->GetTrkType()){
        trkType.emplace_back(iTrkType.first);
        trkTypeId.emplace_back(iTrkType.second);
      }
      evtColl.m_VoxelHitsTrkId.emplace_back(trkType);
      evtColl.m_VoxelHitsTrkTypeId.emplace_back(trkTypeId);

      std::vector<G4double> trkEnergy;
      for(const auto& iTrkE : hit->GetTrkEnergy()){
        trkEnergy.emplace_back(iTrkE.second / MeV);
      }
      evtColl.m_VoxelHitsTrkEnergy.emplace_back(trkEnergy);

      std::vector<G4double> trkTheta;
      for(const auto& iTrkTheta : hit->GetTrkTheta()){
        trkTheta.emplace_back(iTrkTheta.second);
      }
      evtColl.m_VoxelHitsTrkTheta.emplace_back(trkTheta);

      std::vector<G4double> trkPosX;
      std::vector<G4double> trkPosY;
      std::vector<G4double> trkPosZ;
      for(const auto& iTrkPos : hit->GetTrkPosition()){
        trkPosX.emplace_back(iTrkPos.second.getX()-isoToSim.x());
        trkPosY.emplace_back(iTrkPos.second.getY()-isoToSim.y());
        trkPosZ.emplace_back(iTrkPos.second.getZ()-isoToSim.z());
      }
      evtColl.m_VoxelHitsTrkPosX.emplace_back(trkPosX);
      evtColl.m_VoxelHitsTrkPosY.emplace_back(trkPosY);
      evtColl.m_VoxelHitsTrkPosZ.emplace_back(trkPosZ);

      std::vector<G4double> trkLength;
      for(const auto& iTrkLength : hit->GetTrkLength()){
        trkLength.emplace_back(iTrkLength.second);
      }
      evtColl.m_VoxelHitsTrkLength.emplace_back(trkLength);
    }

  }
  if(!evtColl.m_minimalistic_ttree){
    evtColl.m_global_time = *std::max_element(hits_evt_global_time.begin(), hits_evt_global_time.end());
  }
}

////////////////////////////////////////////////////////////////////////////////
///
void NTupleEventAnalisys::FillNTupleEvent(){
  auto analysisManager = G4AnalysisManager::Instance();
  
  for(auto coll = m_ntuple_collection.Begin(); coll != m_ntuple_collection.End(); ++coll){
    auto minimalistic_ttree = coll->second.m_minimalistic_ttree;
    auto nHits = coll->second.m_CellIdX.size();
    if(nHits > 0){ // otheriwse empty event
      auto& treeEvtColl = coll->second;
      auto ntupleId = treeEvtColl.m_ntupleId;

      auto fillNtupleIColumn = [&](const char* name, G4int val){
        // TODO: utilize G4 utilities: G4GenericFileManager::GetFileManager->G4RootAnalysisManager...
        // if(analysisManager->GetNtupleIColumn(ntupleId, treeEvtColl.m_colId[name])) 
          analysisManager->FillNtupleIColumn(ntupleId,treeEvtColl.m_colId[name], val);
      };

      auto fillNtupleDColumn = [&](const char* name, G4double val){
        analysisManager->FillNtupleDColumn(ntupleId,treeEvtColl.m_colId[name], val);
      };

      for (int i=0;i<nHits;++i){ // a.k.a. voxel loop => creates nEvents=nHits in the NTuple
        // ThreadId: -1 for the master thread, and -2 if it is used in the sequential mode.
        // std::string doseMapName;
        // // doseMapName.clear();
        if(!minimalistic_ttree){
          fillNtupleIColumn("ThreadId",G4Threading::G4GetThreadId());
          fillNtupleIColumn("G4EvtId",treeEvtColl.m_evtId);
          fillNtupleDColumn("G4EvtGlobalTime",treeEvtColl.m_global_time);
          fillNtupleIColumn("G4EvtPrimaryN",treeEvtColl.m_EvtPrimariesN);
          fillNtupleIColumn("G4RunId",m_runId);
          fillNtupleDColumn("GantryAngle",m_degree_rotation);
        }

        if(treeEvtColl.m_cell_tree_structure){
          fillNtupleIColumn("CellIdX",treeEvtColl.m_CellIdX.at(i));
          fillNtupleIColumn("CellIdY",treeEvtColl.m_CellIdY.at(i));
          fillNtupleIColumn("CellIdZ",treeEvtColl.m_CellIdZ.at(i));
          if(!minimalistic_ttree){
            fillNtupleDColumn("CellPositionX",treeEvtColl.m_CellPositionX.at(i));
            fillNtupleDColumn("CellPositionY",treeEvtColl.m_CellPositionY.at(i));
            fillNtupleDColumn("CellPositionZ",treeEvtColl.m_CellPositionZ.at(i));
            fillNtupleDColumn("CellEDeposit",treeEvtColl.m_VoxelHitEDeposit.at(i));         // take value from voxel (vox volume==cell)
            fillNtupleDColumn("CellMeanEDeposit",treeEvtColl.m_VoxelHitMeanEDeposit.at(i)); // take value from voxel (vox volume==cell)
          }
          fillNtupleDColumn("CellDose",treeEvtColl.m_CellIDose.at(i));

        }
        if(treeEvtColl.m_voxel_tree_structure){
          fillNtupleIColumn("VoxelIdX",treeEvtColl.m_VoxelIdX.at(i));
          fillNtupleIColumn("VoxelIdY",treeEvtColl.m_VoxelIdY.at(i));
          fillNtupleIColumn("VoxelIdZ",treeEvtColl.m_VoxelIdZ.at(i));
          if(!minimalistic_ttree){
            fillNtupleDColumn("VoxelPositionX",treeEvtColl.m_VoxelPositionX.at(i));
            fillNtupleDColumn("VoxelPositionY",treeEvtColl.m_VoxelPositionY.at(i));
            fillNtupleDColumn("VoxelPositionZ",treeEvtColl.m_VoxelPositionZ.at(i));
            fillNtupleDColumn("VoxelEDeposit",treeEvtColl.m_VoxelHitEDeposit.at(i));
            fillNtupleDColumn("VoxelMeanEDeposit",treeEvtColl.m_VoxelHitMeanEDeposit.at(i));
          }
          fillNtupleDColumn("VoxelDose",treeEvtColl.m_VoxelHitDose.at(i));
        }

        if(treeEvtColl.m_tracks_analysis){
          treeEvtColl.m_VoxelTrkId.clear();
          treeEvtColl.m_VoxelTrkId = treeEvtColl.m_VoxelHitsTrkId.at(i);
          treeEvtColl.m_VoxelTrkTypeId.clear();
          treeEvtColl.m_VoxelTrkTypeId = treeEvtColl.m_VoxelHitsTrkTypeId.at(i);
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

        analysisManager->AddNtupleRow(ntupleId);
      }
    }
  }
}

////////////////////////////////////////////////////////////////////////////////
/// This member is called at the end of every event from EventAction::EndOfEventAction
void NTupleEventAnalisys::EndOfEventAction(const G4Event *evt){
  // G4cout << "NTupleEventAnalisys::EndOfEventAction" << G4endl;
  auto hCofThisEvent = evt->GetHCofThisEvent();
  if(hCofThisEvent){
    auto nColl = hCofThisEvent->GetNumberOfCollections();
    ClearEventCollections();
    const std::vector<TTreeCollection>& ttreeVec = m_ttree_collection.Get();
  for(const auto& tree : ttreeVec){
    for(const auto& hc : tree.m_hc_names){
      //G4cout << "NTupleEventAnalisys::filling: " << tree.m_name+m_treeNamePostfix << " / " << hc << G4endl;

      // Related SensitiveDetector collection ID (Geant4 architecture)
      // collID==-1 the collection is not found
      // collID==-2 the collection name is ambiguous
      auto collection_id = G4SDManager::GetSDMpointer()->GetCollectionID(hc);
      if(collection_id<0)
          G4cout<< "[ERROR]:: NTupleEventAnalisys::EndOfEventAction TTree: " << tree.m_name+m_treeNamePostfix << "G4SDManager err: " << collection_id  << G4endl;
      else{
        auto hitsColl = dynamic_cast<VoxelHitsCollection*>(hCofThisEvent->GetHC(collection_id));
        FillEventCollection(tree.m_name+m_treeNamePostfix,evt,hitsColl);
      }
    }
  }
  FillNTupleEvent();
  }
}

////////////////////////////////////////////////////////////////////////////////
///
void NTupleEventAnalisys::ClearEventCollections(){
  for(auto coll = m_ntuple_collection.Begin(); coll != m_ntuple_collection.End(); ++coll){
    coll->second.m_evtId = -1;
    coll->second.m_global_time= 0.;
    coll->second.m_G4EvtPrimaryEnergy.clear();
    coll->second.m_EvtPrimariesN = 0;
    coll->second.m_CellIdX.clear();
    coll->second.m_CellIdY.clear();
    coll->second.m_CellIdZ.clear();
    coll->second.m_CellPositionX.clear();
    coll->second.m_CellPositionY.clear();
    coll->second.m_CellPositionZ.clear();
    coll->second.m_VoxelIdX.clear();
    coll->second.m_VoxelIdY.clear();
    coll->second.m_VoxelIdZ.clear();
    coll->second.m_VoxelPositionX.clear();
    coll->second.m_VoxelPositionY.clear();
    coll->second.m_VoxelPositionZ.clear();
    coll->second.m_VoxelHitEDeposit.clear();
    coll->second.m_VoxelHitMeanEDeposit.clear();
    coll->second.m_VoxelHitDose.clear();
    coll->second.m_VoxelHitsTrkId.clear();
    coll->second.m_VoxelHitsTrkTypeId.clear();
    coll->second.m_VoxelHitsTrkEnergy.clear();
    coll->second.m_VoxelHitsTrkTheta.clear();
    coll->second.m_VoxelHitsTrkLength.clear();
    coll->second.m_VoxelHitsTrkPosX.clear();
    coll->second.m_VoxelHitsTrkPosY.clear();
    coll->second.m_VoxelHitsTrkPosZ.clear();
    coll->second.m_CellIDose.clear();

  }
}