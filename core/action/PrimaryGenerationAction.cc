#include "PrimaryGenerationAction.hh"
#include "IaeaPrimaryGenerator.hh"
#include "G4GeneralParticleSource.hh"
#include "IonPrimaryGenerator.hh"
#include "PrimariesAnalysis.hh"
#include "G4Event.hh"
#include "Services.hh"
#include "PrimaryParticleInfo.hh"
#include "G4EventManager.hh"
#include "BeamCollimation.hh"


namespace {
  G4Mutex PrimGenMutex = G4MUTEX_INITIALIZER;
}

G4RotationMatrix* PrimaryGenerationAction::m_rotation_matrix=nullptr;
G4double PrimaryGenerationAction::m_source_isocentre_distance = 0.0;

////////////////////////////////////////////////////////////////////////////////
/**
 * @brief Constructs a PrimaryGenerationAction and initializes the primary particle generator.
 *
 * Determines the primary generator type from configuration and sets up the corresponding generator instance for use in simulation events.
 */
PrimaryGenerationAction::PrimaryGenerationAction(){
  SetPrimaryGenerator();
  ConfigurePrimaryGenerator();
}

////////////////////////////////////////////////////////////////////////////////
/**
 * @brief Determine and set the primary generator type from configuration.
 *
 * Reads the RunSvc "BeamType" value and sets m_generatorType accordingly.
 * For "IAEA" also reads RunSvc "PhspEvtVrtxMultiplicityTreshold" into
 * m_min_p_vrtx_vec_size. For unrecognized values a fatal G4Exception is raised.
 *
 * @throws G4Exception FatalErrorInArgument if the configured BeamType is not one of
 *         "IAEA", "gps", or "ion".
 */
void PrimaryGenerationAction::SetPrimaryGenerator() {
  auto configSvc = Service<ConfigSvc>();
  auto sourceType = configSvc->GetValue<std::string>("RunSvc", "BeamType");
  if (sourceType == "IAEA") {
    m_generatorType = PrimaryGeneratorType::PhspIAEA;
    m_min_p_vrtx_vec_size = configSvc->GetValue<int>("RunSvc", "PhspEvtVrtxMultiplicityTreshold");
  } else if (sourceType == "gps") {
    m_generatorType = PrimaryGeneratorType::GPS;
  } else if (sourceType == "ion") {
    m_generatorType = PrimaryGeneratorType::IonGPS;
  } else {
    G4ExceptionDescription msg;
    msg << "Primary generator of type '" << sourceType << "' is not defined";
    G4Exception("PrimaryGenerationAction", "SetPrimaryGenerator", FatalErrorInArgument, msg);
  }
}
  

////////////////////////////////////////////////////////////////////////////////
/**
 * @brief Instantiates and configures the primary particle generator based on the selected generator type.
 *
 * Depending on the configured generator type, this method creates and initializes the appropriate primary generator:
 * - For IAEA phase space files, it verifies the existence of required files and creates an `IaeaPrimaryGenerator`.
 * - For general particle source (GPS), it creates a `G4GeneralParticleSource`.
 * - For ion GPS, it creates an `IonPrimaryGenerator` with a predefined ion source.
 *
 * Throws a fatal exception if required IAEA files are missing.
 */
void PrimaryGenerationAction::ConfigurePrimaryGenerator() {
  auto configSvc = Service<ConfigSvc>();
  G4AutoLock lock(&PrimGenMutex);
  std::string file;
  switch (m_generatorType) {
    case PrimaryGeneratorType::PhspIAEA:
      file = configSvc->GetValue<std::string>("RunSvc", "PhspInputFileName");
      if(!svc::checkIfFileExist(file+".IAEAheader")){
        G4ExceptionDescription msg;
        msg << "IAEA header file doesn't exists: ";
        msg << file+".IAEAheader";
        G4Exception("PrimaryGenerationAction", "ConfigurePrimaryGenerator", FatalErrorInArgument, msg);
      }
      if(!svc::checkIfFileExist(file+".IAEAphsp")){
        G4ExceptionDescription msg;
        msg << "IAEA phsp file doesn't exists: ";
        msg << file+".IAEAphsp";
        G4Exception("PrimaryGenerationAction", "ConfigurePrimaryGenerator", FatalErrorInArgument, msg);
      }
      m_primaryGenerator = new IaeaPrimaryGenerator(G4String(file));
      break;
    case PrimaryGeneratorType::GPS:
      m_primaryGenerator = new G4GeneralParticleSource();
      break;
    case PrimaryGeneratorType::IonGPS:
      // Cd 109
      G4int A = 48;
      G4int Z = 109;
      G4int Q = 0;
      G4double E = 59.6; // keV, 109m1
      // G4double E = 463.0; // keV, 109m2
      m_primaryGenerator = new IonPrimaryGenerator();
      static_cast<IonPrimaryGenerator*>(m_primaryGenerator)->AddSource(A,Z,Q,E,1);
      break;
  }
}

////////////////////////////////////////////////////////////////////////////////
/**
 * @brief Destructor that safely deletes the primary generator instance.
 *
 * Ensures thread-safe cleanup of the primary generator by acquiring a mutex lock before deletion.
 */
PrimaryGenerationAction::~PrimaryGenerationAction(void) {
  G4AutoLock lock(&PrimGenMutex);
  if (m_primaryGenerator) {
    delete m_primaryGenerator;
    m_primaryGenerator = nullptr;
  }
}

////////////////////////////////////////////////////////////////////////////////
/**
 * @brief Generate and configure primary vertices for a single Geant4 event.
 *
 * Generates primaries according to the configured generator backend (IAEA phase-space reader, GPS, or IonGPS),
 * applies beam collimation filtering and — for IAEA sources — enforces a configurable minimum vertex multiplicity
 * by issuing repeated reader calls (with an internal safety cap). For every generated primary vertex this method:
 * - optionally applies a Z translation using the global source isocentre distance,
 * - optionally applies a global rotation (m_rotation_matrix) to vertex positions and primary momenta,
 * - re-attaches IAEA-generated vertices to the event after transformation (non-IAEA generators leave vertices as created),
 * - ensures each primary particle has a PrimaryParticleInfo user object (creating and filling one if absent).
 *
 * If at least one vertex exists and primaries analysis is enabled in configuration, the primaries analysis
 * singleton is invoked to record per-event primary data.
 *
 * The method is thread-safe with respect to primary-generator lifecycle and access (guarded by PrimGenMutex).
 *
 * @param anEvent The Geant4 event to populate and modify with transformed primary vertices and particle user info.
 */
void PrimaryGenerationAction::GeneratePrimaries(G4Event *anEvent) {
  auto runSvc = Service<RunSvc>();
  G4AutoLock lock(&PrimGenMutex);
  auto evtID = anEvent->GetEventID();
  std::vector<G4PrimaryVertex*> primary_vrtx; 
  if( dynamic_cast<IaeaPrimaryGenerator*>(m_primaryGenerator) ){
    auto p_gen = dynamic_cast<IaeaPrimaryGenerator*>(m_primaryGenerator);
    int n_reader_calls = 0;
    while(primary_vrtx.empty() || primary_vrtx.size() < m_min_p_vrtx_vec_size ){
        ++n_reader_calls;
        auto read_p_vrtx = p_gen->GeneratePrimaryVertexVector(anEvent);
        BeamCollimation::FilterPrimaries(read_p_vrtx);
        primary_vrtx.insert(primary_vrtx.end(),read_p_vrtx.begin(),read_p_vrtx.end());
        auto nVrtx = primary_vrtx.size();
        if(nVrtx < m_min_p_vrtx_vec_size && n_reader_calls > 199){ // in order to avoid infinit loop
          //   LOGSVC_WARN("Maximum number of reader calls to reach Evt Vtrx Multiplicity treshold reached!: #Vrtx({}/{}), #Calls:{}",nVrtx,m_min_p_vrtx_vec_size,n_reader_calls);
          break;
        }
    }
  } else {
    m_primaryGenerator->GeneratePrimaryVertex(anEvent);
    for(size_t i=0; i<anEvent->GetNumberOfPrimaryVertex(); ++i)
      primary_vrtx.push_back(anEvent->GetPrimaryVertex(i));
  }


G4ThreeVector new_centre;
// PERFORM BEAM ROTATION ADD VTRXES TO CURRENT EVENT
for (const auto& vrtx : primary_vrtx){
    G4ThreeVector position = vrtx->GetPosition();
    
    if (m_source_isocentre_distance > 0) {
        position.setZ(m_source_isocentre_distance * cm);
    }
    
    if (m_rotation_matrix) {
        new_centre = (*m_rotation_matrix) * position;
        vrtx->SetPosition(new_centre.x(), new_centre.y(), new_centre.z());
    } else {
        vrtx->SetPosition(position.x(), position.y(), position.z());
    }
    
    auto momentum = (vrtx->GetPrimary()->GetMomentum());
    if(m_rotation_matrix){
        momentum = (*m_rotation_matrix) * (momentum);
    }
    vrtx->GetPrimary()->SetMomentum(momentum.x(), momentum.y(), momentum.z());
    if( dynamic_cast<IaeaPrimaryGenerator*>(m_primaryGenerator) )
        anEvent->AddPrimaryVertex(vrtx);
}
  // FILL PRIMARYPARTICLEINFO
  auto nVrtx = anEvent->GetNumberOfPrimaryVertex();
  for(int i =0; i<nVrtx; ++i ){
    auto pparticle = anEvent->GetPrimaryVertex(i)->GetPrimary();
    auto pparticleInfo = pparticle->GetUserInformation();
    if(pparticleInfo){
      dynamic_cast<PrimaryParticleInfo*>(pparticleInfo)->FillInfo(pparticle);
    }
    else{
      pparticleInfo = new PrimaryParticleInfo(); // it's being cleaned up by kernel when event ends it's life
      dynamic_cast<PrimaryParticleInfo*>(pparticleInfo)->FillInfo(pparticle);
      pparticle->SetUserInformation(pparticleInfo);
    }
  }

  // FILL PRIMARY ANALYSIS
  if (nVrtx>0 && Service<ConfigSvc>()->GetValue<bool>("RunSvc", "PrimariesAnalysis") )
    PrimariesAnalysis::GetInstance()->FillPrimaries(anEvent);
}
