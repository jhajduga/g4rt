#include "RunAction.hh"
#include "G4Run.hh"
#include "Services.hh"
#include "WorldConstruction.hh"
#include "SavePhSpAnalysis.hh"
#include "RunAnalysis.hh"
#include "BeamAnalysis.hh"
#include "PrimariesAnalysis.hh"
#include "StepAnalysis.hh"
#include "NTupleEventAnalisys.hh"
#include "colors.hh"
#include<map>
#include<fstream>
#include<iostream>

// NOTE:
// It is recommended, but not necessary, to create the analysis manager in the user
// run action constructor and delete it in its destructor. This guarantees correct
// behavior in multi-threading mode.

/////////////////////////////////////////////////////////////////////////////
///
RunAction::RunAction():G4UserRunAction(){
  auto analysisManager = G4AnalysisManager::Instance();
  // auto numberOfThreads = Service<ConfigSvc>()->GetValue<int>("RunSvc", "NumberOfThreads");
  if (Service<ConfigSvc>()->GetValue<bool>("RunSvc", "NTupleAnalysis")  )
    analysisManager->SetNtupleMerging(false); // we do manual merge, see RunSvc::MergeOutput
  analysisManager->SetVerboseLevel(0);
  //analysisManager->SetNtupleRowWise(true); // TODO: revise this functionality...
  if (Service<ConfigSvc>()->GetValue<bool>("RunSvc", "RunAnalysis"))
    m_run_scoring = true;
}

/////////////////////////////////////////////////////////////////////////////
///
RunAction::~RunAction(){
  delete G4AnalysisManager::Instance();
}

/////////////////////////////////////////////////////////////////////////////
///
G4Run* RunAction::GenerateRun(){
  auto control_point = Service<RunSvc>()->CurrentControlPoint();
  if (IsMaster()){
    // control_point->InitializeRun();
    G4cout << FGRN("[INFO]")<<":: " << FBLU("GENERATING NEW RUN... ") << G4endl;
    G4cout << FGRN("[INFO]")<<":: NEvents: " << control_point->GetNEvts() << G4endl;
    auto rot = control_point->GetRotation();
    if(rot)
      G4cout << FGRN("[INFO]")<<":: Rotation: " << *control_point->GetRotation() << G4endl;
  }
  return control_point->GenerateRun(m_run_scoring);
}


/////////////////////////////////////////////////////////////////////////////
///
void RunAction::BeginOfRunAction(const G4Run* aRun) {
  auto configSvc = Service<ConfigSvc>();
  auto runSvc = Service<RunSvc>();
  
  // For the single run only one file can be created
  auto analysisManager =  G4AnalysisManager::Instance();
  auto runId= std::to_string(aRun->GetRunID());
  analysisManager->SetFileName(runSvc->CurrentControlPoint()->GetSimOutputTFileName(true));
  analysisManager->OpenFile();
  //Master mode or sequential
  if (IsMaster())
    G4cout << "### Run " << aRun->GetRunID() << " starts (master)." << G4endl;
  else
    G4cout << "### Run " << aRun->GetRunID() << " starts (worker)." << G4endl;

  //___________________________________________________________________________
  // TODO:: This is not revised yet,
  // The SavePhSpAnalysis::FillPhSp is not finished
  if (configSvc->GetValue<bool>("RunSvc", "SavePhSp")) {
    SavePhSpAnalysis::GetInstance()->BeginOfRun(aRun, IsMaster());
  }

  //___________________________________________________________________________
  if (configSvc->GetValue<bool>("RunSvc", "RunAnalysis"))
    RunAnalysis::GetInstance()->BeginOfRun(aRun, IsMaster());

  if (configSvc->GetValue<bool>("RunSvc", "PrimariesAnalysis"))
    PrimariesAnalysis::GetInstance()->BeginOfRun(aRun, IsMaster());

  if (configSvc->GetValue<bool>("RunSvc", "BeamAnalysis"))
    BeamAnalysis::GetInstance()->BeginOfRun(aRun, IsMaster());

  if (configSvc->GetValue<bool>("RunSvc", "StepAnalysis"))
    StepAnalysis::GetInstance()->BeginOfRun(aRun, IsMaster());
  
  if (configSvc->GetValue<bool>("RunSvc", "NTupleAnalysis") && NTupleEventAnalisys::IsAnyTTreeDefined() ) 
    NTupleEventAnalisys::GetInstance()->BeginOfRun(aRun, IsMaster());

  //___________________________________________________________________________
  // Setup geometry configuration and write it's information to the screen
  if (IsMaster())
    Service<GeoSvc>()->World()->WriteInfo();

  m_timer.Start();
}

/////////////////////////////////////////////////////////////////////////////
///
void RunAction::EndOfRunAction(const G4Run* aRun) {
  m_timer.Stop();
  G4double loopRealElapsedTime = m_timer.GetRealElapsed();
  if (IsMaster()) {
    G4cout << "Global-loop elapsed time [s] : " << loopRealElapsedTime << G4endl;
  }
  else
    G4cout << "Local-loop elapsed time [s] : " << loopRealElapsedTime << G4endl;


  auto analysisManager = G4AnalysisManager::Instance();
  analysisManager->Write();
  analysisManager->CloseFile();

  //___________________________________________________________________________
  auto configSvc = Service<ConfigSvc>();
  if(configSvc->GetValue<bool>("RunSvc", "RunAnalysis") && IsMaster())
    RunAnalysis::GetInstance()->EndOfRun(aRun);

  // Service<RunSvc>()->EndOfRun();
}

