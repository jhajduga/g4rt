//
// Created by brachwal on 06.05.2020.
//

#include "SavePhSpAnalysis.hh"
#include "G4Step.hh"
#include "G4Run.hh"
#include "G4SDManager.hh"
#include "G4AnalysisManager.hh"
#include "Services.hh"

////////////////////////////////////////////////////////////////////////////////
///
SavePhSpAnalysis *SavePhSpAnalysis::GetInstance() {
  static SavePhSpAnalysis instance = SavePhSpAnalysis();
  return &instance;
}


////////////////////////////////////////////////////////////////////////////////
///
void SavePhSpAnalysis::BeginOfRun(const G4Run* runPtr, G4bool isMaster){
  // Extract from VPatient geometry information, and define NTuples structure
  //
  auto analysisManager = G4AnalysisManager::Instance();

  auto phsp_planes_filename = Service<ConfigSvc>()->GetValue<std::string>("RunSvc", "PhspOutputFileName");
  analysisManager->OpenFile(phsp_planes_filename);

  // Book SavePhSp data Ntuple
  //------------------------------------------
  analysisManager->SetNtupleDirectoryName("phasespaces");
  m_ntupleId = analysisManager->CreateNtuple("Space1", "Some nice title");
  analysisManager->CreateNtupleDColumn("x");      // 0
  analysisManager->CreateNtupleDColumn("y");      // 1
  analysisManager->CreateNtupleDColumn("z");      // 2
  analysisManager->CreateNtupleDColumn("u");      // 3
  analysisManager->CreateNtupleDColumn("v");      // 4
  analysisManager->CreateNtupleDColumn("w");      // 5
  analysisManager->CreateNtupleDColumn("E");      // 6
  analysisManager->CreateNtupleIColumn("type");   // 7
  analysisManager->CreateNtupleIColumn("index");  // 8
  analysisManager->FinishNtuple();

  // auto ntuple = analysisManager->GetNtuple(m_ntupleId);
  // if(isMaster)
  //   G4cout << "[INFO]:: SavePhSpAnalysis:: The "<<ntuple->title()<<" ntuple with id = "<< m_ntupleId <<" has been created: " <<G4endl;

}

////////////////////////////////////////////////////////////////////////////////
///
void SavePhSpAnalysis::FillPhSp(G4Step* step) {
  // TODO :: put here the stuff from SavePhSpSD::ProcessHits
}
