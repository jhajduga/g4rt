//
// Created by brachwal on 14.05.2020.
//

#include "StepAnalysis.hh"
#include "G4Event.hh"
#include "G4Step.hh"
#include "G4Run.hh"
#include "G4SDManager.hh"
#include "G4AnalysisManager.hh"
#include "G4Gamma.hh"
#include "G4Proton.hh"
#include "G4Positron.hh"
#include "G4Electron.hh"
#include "G4Neutron.hh"

#include "G4VProcess.hh"
////////////////////////////////////////////////////////////////////////////////
/**
 * @brief Returns the singleton instance of the StepAnalysis class.
 *
 * Ensures only one StepAnalysis object exists throughout the application.
 *
 * @return Pointer to the singleton StepAnalysis instance.
 */
StepAnalysis *StepAnalysis::GetInstance() {
  static StepAnalysis instance = StepAnalysis();
  return &instance;
}


////////////////////////////////////////////////////////////////////////////////
/**
 * @brief Initializes analysis ntuples for hits and tracks at the start of a simulation run.
 *
 * Defines the structure of two ntuples in the Geant4 analysis manager: one for storing per-step hit data and one for per-step track data. Each ntuple is configured with columns for relevant physical quantities such as positions, energy deposits, track IDs, particle types, and process IDs. This setup prepares the analysis framework to collect and store simulation data during the run.
 */
void StepAnalysis::BeginOfRun(const G4Run* runPtr, G4bool isMaster){
  // Extract from VPatient geometry information, and define NTuples structure
  //
  auto analysisManager =  G4AnalysisManager::Instance();

  // Book Hits data Ntuple
  //------------------------------------------
  auto ntupleId = analysisManager->CreateNtuple("HitsEventTree","Events contain hits data from each G4Step");
  m_hitsNtupleId.Put(ntupleId);
  m_nHits.Put(0);
  analysisManager->CreateNtupleIColumn(ntupleId, "nHits");                    // column Id = 0
  analysisManager->CreateNtupleDColumn(ntupleId, "EDeposit");                 // column Id = 1
  analysisManager->CreateNtupleDColumn(ntupleId, "HitX",m_hitsX.Get());  // column Id = 2
  analysisManager->CreateNtupleDColumn(ntupleId, "HitY",m_hitsY.Get());  // column Id = 3
  analysisManager->CreateNtupleDColumn(ntupleId, "HitZ",m_hitsZ.Get());  // column Id = 4
  analysisManager->CreateNtupleDColumn(ntupleId, "HitEDeposit",m_hitsEDeposit.Get());  // column Id = 5
  analysisManager->CreateNtupleDColumn(ntupleId, "HitProcessId",m_hitsProcessDefId.Get());  // column Id = 6
  analysisManager->FinishNtuple(ntupleId);

  // Book Tracks data Ntuple
  //------------------------------------------
  ntupleId = analysisManager->CreateNtuple("TracksEventTree","Events contain tracks data from each G4Step");
  m_trkNtupleId.Put(ntupleId);
  analysisManager->CreateNtupleDColumn(ntupleId, "HitTrkE",m_trkE.Get());  // column Id = 0
  analysisManager->CreateNtupleDColumn(ntupleId, "HitTrkX",m_trkX.Get());  // column Id = 1
  analysisManager->CreateNtupleDColumn(ntupleId, "HitTrkY",m_trkY.Get());  // column Id = 2
  analysisManager->CreateNtupleDColumn(ntupleId, "HitTrkZ",m_trkZ.Get());  // column Id = 3
  analysisManager->CreateNtupleDColumn(ntupleId, "HitTrkTheta",m_trkTheta.Get());  // column Id = 4
  analysisManager->CreateNtupleIColumn(ntupleId, "HitTrkId",m_trkId.Get());  // column Id = 5
  analysisManager->CreateNtupleIColumn(ntupleId, "HitTrkTypeId",m_trkTypeId.Get());  // column Id = 6
  analysisManager->CreateNtupleIColumn(ntupleId, "HitTrkCrProcessId",m_trkCreatorProcessId.Get());  // column Id = 7
  analysisManager->FinishNtuple(ntupleId);

  // Book histograms
  //------------------------------------------
  // TODO define some basics histograms

}

////////////////////////////////////////////////////////////////////////////////
///
/**
 * @brief Extracts and stores track information from a G4Track object for analysis.
 *
 * Records the track ID, position (in cm), kinetic energy (in keV), momentum theta angle, particle type ID, and creator process ID. Particle and process types are mapped to integer IDs for downstream analysis.
 */
void StepAnalysis::FillTrack(G4Track* aTrack){
  //__
  m_trkId.Push_back(aTrack->GetTrackID());

  //__
  auto position = aTrack->GetPosition();
  m_trkX.Push_back(position.x()/cm);
  m_trkY.Push_back(position.y()/cm);
  m_trkZ.Push_back(position.z()/cm);

  //__
  auto trkDef = aTrack->GetDynamicParticle()->GetDefinition();
  G4int trkTypeId = -1;
  if(trkDef==G4Gamma::Definition())     trkTypeId=1;
  if(trkDef==G4Electron::Definition())  trkTypeId=2;
  if(trkDef==G4Positron::Definition())  trkTypeId=3;
  if(trkDef==G4Neutron::Definition())   trkTypeId=4;
  if(trkDef==G4Proton::Definition())    trkTypeId=5;
  m_trkTypeId.Push_back(trkTypeId);

  //__
  auto trkEnergy = aTrack->GetKineticEnergy();
  m_trkE.Push_back(trkEnergy/keV);

  //__
  m_trkTheta.Push_back(aTrack->GetMomentum().theta());

  //__
  auto trkCreatorProcess = aTrack->GetCreatorProcess();
  G4int trkProcessId = -1;
  if(trkCreatorProcess) {
    auto pName = trkCreatorProcess->GetProcessName();
    if(pName=="mesh_x")  trkProcessId = 0;
    else if(pName=="mesh_y")  trkProcessId = 1;
    else if(pName=="mesh_z")  trkProcessId = 2;
    else if(pName=="msc")  trkProcessId = 3;
    else if(pName=="eIoni")  trkProcessId = 4;
    else if(pName=="ionIoni")  trkProcessId = 5;
    else if(pName=="compt")  trkProcessId = 6;
    else if(pName=="phot")  trkProcessId = 7;
    else if(pName=="eBrem")  trkProcessId = 8;
    else if(pName=="conv")  trkProcessId = 9;
    else if(pName=="annihil")  trkProcessId = 10;
    else if(pName=="CoupledTransportation")  trkProcessId = 11;
    else if(pName=="Rayl")  trkProcessId = 12;
    else{
      trkCreatorProcess->DumpInfo();
      G4cout << "[DEV]:: Trk CreatorProcess Name:: " << pName << "\n" << G4endl;
    }
  }
  m_trkCreatorProcessId.Push_back(trkProcessId);

}

////////////////////////////////////////////////////////////////////////////////
///
/**
 * @brief Records hit information from a simulation step.
 *
 * Extracts and stores the pre-step position (in cm), total energy deposit (in keV), and the process ID of the post-step process for the given step. Increments the hit count and appends the data to internal event containers. If the process name is unrecognized, outputs debug information.
 */
void StepAnalysis::FillHit(G4Step* aStep){

  //__
  m_nHits.Put(m_nHits.Get()+1);

  //__
  auto position = aStep->GetPreStepPoint()->GetPosition(); // in world volume frame
  m_hitsX.Push_back(position.x()/cm);
  m_hitsY.Push_back(position.y()/cm);
  m_hitsZ.Push_back(position.z()/cm);

  //__
  auto eDeposit = aStep->GetTotalEnergyDeposit();
  m_hitsEDeposit.Push_back(eDeposit/keV);

  //__
  auto postStepDefProcess = aStep->GetPostStepPoint()->GetProcessDefinedStep();
  G4int processId = -1;
  if(postStepDefProcess){
    auto pName = postStepDefProcess->GetProcessName();
    if(pName=="mesh_x")  processId = 0;
    else if(pName=="mesh_y")  processId = 1;
    else if(pName=="mesh_z")  processId = 2;
    else if(pName=="msc")  processId = 3;
    else if(pName=="eIoni")  processId = 4;
    else if(pName=="ionIoni")  processId = 5;
    else if(pName=="compt")  processId = 6;
    else if(pName=="phot")  processId = 7;
    else if(pName=="eBrem")  processId = 8;
    else if(pName=="conv")  processId = 9;
    else if(pName=="annihil")  processId = 10;
    else if(pName=="CoupledTransportation")  processId = 11;
    else if(pName=="Rayl")  processId = 12;
    else{
      postStepDefProcess->DumpInfo();
      G4cout << "[DEV]::POST ProcessDefinedStep Name:: " <<  pName << G4endl;
    }
  }
  m_hitsProcessDefId.Push_back(processId);

}

////////////////////////////////////////////////////////////////////////////////
/**
 * @brief Records accumulated hit and track data for the current event into analysis ntuples.
 *
 * Stores the number of hits and total event energy deposit in the hits ntuple, and commits the current track data to the tracks ntuple.
 *
 * @param evtEnergyDeposit Total energy deposited in the event, in keV.
 */
void StepAnalysis::FillEvent(G4double evtEnergyDeposit) {
  auto analysisManager = G4AnalysisManager::Instance();
  auto ntupleId = m_hitsNtupleId.Get();
  analysisManager->FillNtupleIColumn(ntupleId,0, m_nHits.Get());
  analysisManager->FillNtupleDColumn(ntupleId,1, evtEnergyDeposit);
  analysisManager->AddNtupleRow(ntupleId);

  ntupleId = m_trkNtupleId.Get();
  analysisManager->AddNtupleRow(ntupleId);
}

////////////////////////////////////////////////////////////////////////////////
/**
 * @brief Handles end-of-event data processing and storage.
 *
 * If any hits were recorded during the event, sums the total energy deposited, stores the event data, and clears all accumulated event data in preparation for the next event.
 */
void StepAnalysis::EndOfEventAction(const G4Event *evt){
  if(m_nHits.Get()>0) { // process info only if step hits exists
    G4double evtEnergyDeposit = 0;
    for (const auto &hitEDep : m_hitsEDeposit.Get())
      evtEnergyDeposit += hitEDep;
    FillEvent(evtEnergyDeposit);
    ClearEventData();
  }
}

////////////////////////////////////////////////////////////////////////////////
/**
 * @brief Resets all stored event data for hits and tracks.
 *
 * Clears all internal containers and counters to prepare for the next event.
 */
void StepAnalysis::ClearEventData(){
  m_nHits.Put(0);
  m_hitsX.Clear();
  m_hitsY.Clear();
  m_hitsZ.Clear();
  m_hitsEDeposit.Clear();
  m_hitsProcessDefId.Clear();
  m_trkE.Clear();
  m_trkX.Clear();
  m_trkY.Clear();
  m_trkZ.Clear();
  m_trkTheta.Clear();
  m_trkId.Clear();
  m_trkTypeId.Clear();
  m_trkCreatorProcessId.Clear();
}

////////////////////////////////////////////////////////////////////////////////
/**
 * @brief Collects and stores both hit and track data for a simulation step.
 *
 * Calls the appropriate methods to extract and record hit information from the provided step and track information from the associated track.
 *
 * @param aStep Pointer to the Geant4 step object to process.
 */
void StepAnalysis::FillStep(G4Step* aStep){
  FillHit(aStep);
  FillTrack(aStep->GetTrack());
}