#include "UIManager.hh"
#include "Services.hh"
#include "G4Timer.hh"
#include "toml.hh"
#include "PrimaryGenerationAction.hh"
// #include "LogSession.hh"
////////////////////////////////////////////////////////////////////////////////
///
UIManager::UIManager()
    : UIG4Manager(G4UImanager::GetUIpointer()), m_isG4kernelInitialized(false) {
      // LogSession * LoggedSession = new LogSession();
      // UIG4Manager->SetCoutDestination(LoggedSession);
      UIG4Manager->ApplyCommand("/control/verbose 2");
      UIG4Manager->ApplyCommand("/run/verbose 2");
    }

////////////////////////////////////////////////////////////////////////////////
///
UIManager *UIManager::GetInstance() {
  static UIManager instance;
  return &instance;
}

////////////////////////////////////////////////////////////////////////////////
///
void UIManager::ReadMacros(const std::vector<G4String> &m_MacFiles) {
  ProcessingFlag BeamOnFlag;
  for (auto macFile : m_MacFiles) {
    if ( G4StrUtil::contains(macFile,"_pre"))
      BeamOnFlag = preBeamOn;
    else if (G4StrUtil::contains( macFile, "_post"))
      BeamOnFlag = postBeamOn;
    else
      BeamOnFlag = preBeamOn;
    ReadMacro(macFile, BeamOnFlag);
  }
}

////////////////////////////////////////////////////////////////////////////////
///
void UIManager::ReadMacro(const G4String &m_MacFile, const ProcessingFlag m_WhenToApply) {
  G4cout << "[INFO]:: Reading the command file :: " << m_MacFile << G4endl;
  std::string line;
  std::ifstream file(m_MacFile.data());
  if (file.is_open()) {  // check if file exists
    while (getline(file, line)) {
      if (line.length() > 0 && line.at(0) != '#') {  // get rid of commented out or empty lines
        std::size_t lcomm = line.find('#');
        unsigned end =
            lcomm != std::string::npos ? lcomm - 1 : line.length();  // get rid of the comment at the end of the line
        switch (m_WhenToApply) {
          case preBeamOn:
            PreBeamOnCommands.push_back(line.substr(0, end));
            break;
          case postBeamOn:
            PostBeamOnCommands.push_back(line.substr(0, end));
            break;
        }
      }
    }
  } else {
    G4String description = "The " + m_MacFile + " not found";
    G4Exception("UIManager", "MACFILE", FatalErrorInArgument, description.data());
  }
}

////////////////////////////////////////////////////////////////////////////////
///
void UIManager::ApplyCommand(const G4String &m_command) {
  G4cout << "[INFO]:: UIManager :: Applying command :: " << m_command << G4endl;
  UIG4Manager->ApplyCommand(m_command.data());
}

////////////////////////////////////////////////////////////////////////////////
///
void UIManager::InitializeG4kernel() {
  if (!m_isG4kernelInitialized) {
    ApplyCommand("/run/initialize");
    m_isG4kernelInitialized = true;
  }
}

////////////////////////////////////////////////////////////////////////////////
///
void UIManager::UserRunInitialization() {
  auto runSvc = Service<RunSvc>();
  auto geoSvc = Service<GeoSvc>();

  G4Timer pretimer;
  pretimer.Start();

  // Read the .mac files commands to be applyied
  if (!runSvc->GetMacFiles().empty()) ReadMacros(runSvc->GetMacFiles());

  // Initialize G4 kernel
  //-----------------------------------------------------------------
  // InitializeG4kernel();

  pretimer.Stop();
  G4cout << "Pre-beam elapsed time [s] : " << pretimer.GetRealElapsed() << G4endl;

  // RUN LOOP SETUP
  //-----------------------------------------------------------------
  G4cout << "[INFO]:: UIManager :: RUN SETUP " << G4endl;
  auto controPoints = runSvc->GetControlPoints();
  // auto runId = int(0); // TODO: runSvc->GetCurrentRun();
  // geoSvc->Update(runId);
  // toml::table runList = toml::parse("/home/jack/work/dose3d-geant4-linac/jobs/example_job_jh.toml");
  // auto treatment = runList["TreatmentPlan"]["ControlPoints"];
  // if (toml::array* controlpoints = treatment.as_array()){
  //     G4cout << "\n\n\n\n" << treatment.type() << "\n\n\n" << G4endl;
  //     G4cout << "\n\n\n\n" << controlpoints->type() << "\n\n\n" << G4endl;
  // }


  for(auto& cp : controPoints){
    runSvc->CurrentControlPoint(&cp);
    runSvc->LoadSimulationPlan();
    runSvc->G4RunManagerPtr()->SetRunIDCounter(cp.GetId());
    G4cout << "DEBUG:: UIManager::UserRunInitialization:: rotation_matrix:\n" << *cp.GetRotation() << G4endl;
    PrimaryGenerationAction::SetRotation(cp.GetRotation());
    InitializeG4kernel();
    for (auto ic : PreBeamOnCommands) 
      ApplyCommand(ic);
    // LOGSVC_DEBUG("UIManager::BeamOn({})",cp.GetNEvts());
    runSvc->G4RunManagerPtr()->BeamOn(cp.GetNEvts());
  }

  // PostBeamOn commands
  //-----------------------------------------------------------------
  G4Timer posttimer;
  posttimer.Start();
  for (auto ic : PostBeamOnCommands) 
    ApplyCommand(ic);
  posttimer.Stop();
  G4cout << "Post-beam elapsed time [s] : " << posttimer.GetRealElapsed() << G4endl;
}

////////////////////////////////////////////////////////////////////////////////
///
void UIManager::UserRunFinalization() {
  G4cout << "[INFO]:: LinacUIManager :: Service finalization " << G4endl;

  // NOTE: different stuff for the simulation outcome can be define here,
  //       e.g. resulting files conversions etc.
}
