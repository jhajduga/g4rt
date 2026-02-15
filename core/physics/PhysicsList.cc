#include "PhysicsList.hh"
#include "Services.hh"
#include "G4ProcessManager.hh"
#include "G4EmStandardPhysics_option3.hh"
#include "G4EmStandardPhysics_option4.hh"
#include "G4EmLivermorePhysics.hh"
#include "G4EmLivermorePolarizedPhysics.hh"
#include "G4EmPenelopePhysics.hh"
#include "G4EmParameters.hh"
#include "StepMax.hh"
#include "G4OpticalPhoton.hh"

// ============================================================================
// TODO [Refactor]: Consider migrating to a fully modular physics list
// - Replace direct usage of ConstructParticle() and ConstructProcess()
//   with physics constructors via RegisterPhysics()
// - Use consistent, self-contained G4VPhysicsConstructor implementations
// - Add safety checks for EM physics registration
// - Modularize custom processes (e.g., StepMax, Optical) as independent physics modules
// - Document EM/optical configuration flags in PhysicsReadme.md
// ============================================================================

////////////////////////////////////////////////////////////////////////////////
/**
 * @brief Construct and configure the Geant4 physics list used for the run.
 *
 * Initializes default production cuts and verbosity, selects the electromagnetic
 * (EM) physics model from configuration, and instantiates core physics modules
 * for decay and radioactive decay. Optionally constructs and registers additional
 * physics modules (EM extras, hadronic, ion, stopping, optical) when enabled
 * via configuration, and creates a global step limiter applied to all particles.
 *
 * Configuration keys read from RunSvc:
 * - "Physics" : EM physics model name to activate.
 * - "EnableExtraProcesses" : enable construction/registration of extra physics modules.
 * - "StepMax" : global maximum step length applied (in mm).
 *
 * Side effects:
 * - Registers the optical physics module when extra processes are enabled.
 * - Prepares physics modules for later ConstructParticle/ConstructProcess calls.
 */
PhysicsList::PhysicsList() {

  SetDefaultCutValue(0.5 * mm);
  SetVerboseLevel(0);

  /// EM physics model selected from config 
  AddPhysicsList(G4String(Service<ConfigSvc>()->GetValue<std::string>("RunSvc", "Physics")));

  // Enable extra processes
  m_enableExtraProcesses = Service<ConfigSvc>()->GetValue<bool>("RunSvc", "EnableExtraProcesses");

  //
  m_stepMax = Service<ConfigSvc>()->GetValue<double>("RunSvc", "StepMax");

  /// Core physics components 
  m_decayPhysicsModelCtr          = std::make_unique<G4DecayPhysics>();
  m_radioactiveDecayPhysicsCtr    = std::make_unique<G4RadioactiveDecayPhysics>();

  if (m_enableExtraProcesses) {
  m_extraPhysicsCtr               = std::make_unique<G4EmExtraPhysics>();
  m_hadronElasticPhysicsCtr       = std::make_unique<G4HadronElasticPhysicsHP>();
  m_hadronInelasticPhysicsCtr     = std::make_unique<G4HadronPhysicsQGSP_BIC_HP>();
  m_ionElasticPhysicsCtr          = std::make_unique<G4IonElasticPhysics>();
  m_ionPhysicsCtr                 = std::make_unique<G4IonPhysics>();
  m_stoppingPhysicsCtr            = std::make_unique<G4StoppingPhysics>();
  m_opticalPhysicsCtr             = std::make_unique<G4OpticalPhysics>();
  /// Optical photon physics: detailed tracking of optical photons, boundaries, absorption 
  RegisterPhysics(m_opticalPhysicsCtr.get());

  }

  // Step limiter
  m_stepLimitPhysicsCtr = std::make_unique<G4StepLimiterPhysics>();
  m_stepLimitPhysicsCtr->SetApplyToAll(true);
}


////////////////////////////////////////////////////////////////////////////////
/**
 * @brief Constructs particles required for the simulation, including decay products and optical photons.
 *
 * Invokes the decay physics model to define all particles involved in decay processes and explicitly defines the optical photon particle for optical physics simulations.
 */
void PhysicsList::ConstructParticle() {

  /// Decay particles
  m_decayPhysicsModelCtr->ConstructParticle();

  /// Optical photon definition
  G4OpticalPhoton::OpticalPhotonDefinition();
}
////////////////////////////////////////////////////////////////////////////////
/**
 * @brief Attach a global hard step-limiter to all applicable, non-short-lived particles.
 *
 * Creates a StepMax process, sets its maximum step to the PhysicsList's m_stepMax (converted to millimeters),
 * and registers that same StepMax instance as a discrete process with each particle's process manager for which
 * the StepMax is applicable.
 *
 * The installed StepMax enforces an upper bound on step length and will constrain stepping when its maximum
 * is shorter than other competing step limitations.
 *
 * @note Allocates a single StepMax instance and registers the same pointer with multiple particle process managers.
 */
void PhysicsList::AddStepMax()
{
  // User-defined hard step limiter (StepMax process):
  // Imposes a strict upper limit on step length for applicable particles.
  // Overrides EM step size if the StepMax threshold is shorter.
  // Should be added manually via AddDiscreteProcess.
  StepMax* stepMaxProcess = new StepMax();
  stepMaxProcess->SetMaxStep(m_stepMax * mm);

  auto particleIterator=GetParticleIterator();
  particleIterator->reset();
  while ((*particleIterator)()){
      G4ParticleDefinition* particle = particleIterator->value();
      G4ProcessManager* pmanager = particle->GetProcessManager();

      if (stepMaxProcess->IsApplicable(*particle) && !particle->IsShortLived())
        {
	  pmanager ->AddDiscreteProcess(stepMaxProcess);
        }
  }
}


////////////////////////////////////////////////////////////////////////////////
/**
 * @brief Configures and registers all physics processes for the simulation.
 *
 * Adds the mandatory transportation process, the selected electromagnetic (EM) physics model, standard and radioactive decay processes, and, if enabled, additional physics modules for extra EM effects, hadronic and ion interactions, and stopping physics. Configures detailed electromagnetic parameters for atomic de-excitation, secondary emissions, low-energy electron tracking, dynamic step limitation, Mott correction, and ICRU90 data usage. Optionally adds a user-defined hard step limiter process if a maximum step size is specified.
 */
void PhysicsList::ConstructProcess() {

  /// Mandatory transportation process 
  AddTransportation();  // Enables particles to move through geometry

  /// Electromagnetic physics processes (standard EM model) 
  m_emPhysicsModelCtr->ConstructProcess();  // Main EM model (e.g., Livermore, Penelope, Opt3...)
  
  /// Standard particle decays (e.g., muon, pion, kaon) 
  m_decayPhysicsModelCtr->ConstructProcess();

  /// Radioactive decay (e.g., beta, alpha, gamma emissions from unstable nuclei) 
  m_radioactiveDecayPhysicsCtr->ConstructProcess();

  if (m_enableExtraProcesses) {
    /// Additional EM processes: Cerenkov, scintillation (basic), synchrotron radiation 
    m_extraPhysicsCtr->ConstructProcess();  // Supplements standard EM with extra high-energy effects

    /// Hadronic elastic scattering (high-precision models for neutrons) 
    m_hadronElasticPhysicsCtr->ConstructProcess();

    /// Hadronic inelastic scattering (e.g., neutron capture, spallation) 
    m_hadronInelasticPhysicsCtr->ConstructProcess();

    /// Elastic scattering for ions (e.g., protons, alpha particles, heavier nuclei) 
    m_ionElasticPhysicsCtr->ConstructProcess();

    /// Ion interaction models: ionization, nuclear interactions 
    m_ionPhysicsCtr->ConstructProcess();

    /// Stopping physics: energy loss and annihilation for low-energy ions 
    m_stoppingPhysicsCtr->ConstructProcess();

    /// Electromagnetic parameters configuration (critical for accurate results) 
    G4EmParameters* param = G4EmParameters::Instance();

    // Atomic de-excitation (Auger electrons, fluorescence X-rays, and PIXE)
    param->SetAuger(true);              // Enable Auger electron emission
    param->SetAugerCascade(true);      // Enable cascade emission for inner shell transitions
    param->SetFluo(true);              // Enable X-ray fluorescence
    param->SetPixe(true);              // Enable proton-induced X-ray emission

    // Enable full atomic de-excitation simulation in all regions
    param->SetDeexActiveRegion("DefaultRegionForTheWorld", true, true, true);

    // Allow secondary emission below standard cuts (no artificial energy threshold)
    param->SetDeexcitationIgnoreCut(true);

    // Set lowest trackable electron energy (down to 10 eV)
    param->SetLowestElectronEnergy(10*eV);

    // EM-based dynamic step limitation (soft constraint):
    // Limits step length based on energy loss fluctuations (dE/dx).
    // Applied automatically by EM processes; does not enforce a hard upper limit.
    // Useful for improving energy loss accuracy in high-gradient regions.
    param->SetStepFunction(0.2, 100*um);


    // Enable Mott correction for more accurate electron scattering (important for detectors, phantoms)
    param->SetUseMottCorrection(true);

    // Use updated ICRU90 data for electron interactions
    param->SetUseICRU90Data(true);
  }

  /// Optional: user-defined step limit process 
  if (m_stepMax > 1e-9 * mm) {
    AddStepMax();  // Enable this if you want to restrict maximum step size manually
  }
}

////////////////////////////////////////////////////////////////////////////////
/**
 * @brief Activate an electromagnetic physics model by name.
 *
 * Instantiates and sets the requested EM physics controller if the name matches a supported model; if the name is unrecognized, logs a warning and falls back to the current EM physics model name.
 *
 * @param name The name of the EM physics model to activate (e.g., "emstandard_opt3", "emstandard_opt4", "LowE_Livermore", "LowE_Polar_Livermore", "LowE_Penelope").
 */
void PhysicsList::AddPhysicsList(const G4String &name) {
  auto vlevel = GetVerboseLevel();
  if (vlevel > -1)
    G4cout << "PhysicsList::AddPhysicsList: <" << name << ">" << G4endl;

  if (name == "emstandard_opt3") {
    m_emPhysicsModelCtr = std::make_unique<G4EmStandardPhysics_option3>(vlevel);
    m_emPhysicsModelName = name;
  } else if (name == "emstandard_opt4") {
    m_emPhysicsModelCtr = std::make_unique<G4EmStandardPhysics_option4>(vlevel);
    m_emPhysicsModelName = name;
  } else if (name == "LowE_Livermore") {
    m_emPhysicsModelCtr = std::make_unique<G4EmLivermorePhysics>(vlevel);
    m_emPhysicsModelName = name;
  } else if (name == "LowE_Polar_Livermore") {
    m_emPhysicsModelCtr = std::make_unique<G4EmLivermorePolarizedPhysics>(vlevel);
    m_emPhysicsModelName = name;
  } else if (name == "LowE_Penelope") {
    m_emPhysicsModelCtr = std::make_unique<G4EmPenelopePhysics>(vlevel);
    m_emPhysicsModelName = name;
  } else{
    G4cout << "PhysicsList::AddPhysicsList: <" << name << "> is not defined" << G4endl;
    G4cout << "PhysicsList::AddPhysicsList: building default opt <"<<m_emPhysicsModelName<<">" << G4endl;
    AddPhysicsList(m_emPhysicsModelName);
  }
  G4cout << "THE FOLLOWING ELECTROMAGNETIC PHYSICS LIST HAS BEEN ACTIVATED: " << m_emPhysicsModelName << G4endl;
}