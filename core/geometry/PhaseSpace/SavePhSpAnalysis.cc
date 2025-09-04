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
 * @brief Return the singleton instance of SavePhSpAnalysis.
 *
 * The instance is created on first call and the same pointer is returned on
 * subsequent calls. Initialization of the function-local static is thread-safe
 * (since C++11) and the instance has static storage lifetime (destroyed at
 * program termination).
 *
 * @return SavePhSpAnalysis* Pointer to the single SavePhSpAnalysis instance.
 */
SavePhSpAnalysis *SavePhSpAnalysis::GetInstance() {
  static SavePhSpAnalysis instance = SavePhSpAnalysis();
  return &instance;
}


////////////////////////////////////////////////////////////////////////////////
/**
 * @brief Prepare phase-space output before a simulation run.
 *
 * Initializes the Geant4 analysis manager for phase-space recording: opens the output
 * file (name read from configuration key "RunSvc"."PhspOutputFileName"), sets the
 * ntuple directory to "phasespaces", creates an ntuple named "Space1" and defines
 * columns for particle position (x,y,z), direction cosines (u,v,w), energy (E),
 * particle type, and an index. The created ntuple id is stored in m_ntupleId.
 *
 * @param runPtr Pointer to the current run (unused by this implementation).
 * @param isMaster True when running in the master thread/process (intended for master-only logging).
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
 * @brief Fill the phase-space ntuple with data from a simulation step.
 *
 * Extracts phase-space information (position, direction cosines, energy,
 * particle type/index, etc.) from the provided G4Step and records it to the
 * analysis ntuple prepared in BeginOfRun. Currently unimplemented — left as a
 * TODO placeholder (intended to mirror the behavior of SavePhSpSD::ProcessHits).
 *
 * @param step Pointer to the current Geant4 simulation step containing the
 *             particle state to be recorded. May be null-checked by callers;
 *             this function currently performs no action.
 */
void SavePhSpAnalysis::FillPhSp(G4Step* step) {
  // TODO :: put here the stuff from SavePhSpSD::ProcessHits
}
