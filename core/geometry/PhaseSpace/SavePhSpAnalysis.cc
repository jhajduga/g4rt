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
/**
 * @brief Returns the singleton instance of the SavePhSpAnalysis class.
 *
 * Ensures that only one instance of SavePhSpAnalysis exists throughout the program.
 *
 * @return Pointer to the singleton SavePhSpAnalysis instance.
 */
SavePhSpAnalysis *SavePhSpAnalysis::GetInstance() {
  static SavePhSpAnalysis instance = SavePhSpAnalysis();
  return &instance;
}


////////////////////////////////////////////////////////////////////////////////
/**
 * @brief Initializes phase space analysis at the start of a simulation run.
 *
 * Opens the output file for phase space data, sets up the ntuple directory, and defines the ntuple structure for recording particle phase space information, including spatial coordinates, direction cosines, energy, particle type, and index.
 */
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
/**
 * @brief Placeholder for filling phase space data from a simulation step.
 *
 * Intended to record phase space information using data from the provided simulation step.
 *
 * @param step Pointer to the current simulation step containing particle data.
 */
void SavePhSpAnalysis::FillPhSp(G4Step* step) {
  // TODO :: put here the stuff from SavePhSpSD::ProcessHits
}
