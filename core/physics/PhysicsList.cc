#include "PhysicsList.hh"
#include "Services.hh"

#include "G4ProcessManager.hh"
#include "G4EmStandardPhysics_option3.hh"
#include "G4EmStandardPhysics_option4.hh"
#include "G4EmLivermorePhysics.hh"
#include "G4EmLivermorePolarizedPhysics.hh"
#include "G4EmExtraPhysics.hh"
#include "G4EmPenelopePhysics.hh"
#include "G4DecayPhysics.hh"
#include "G4StepLimiterPhysics.hh"
#include "G4EmParameters.hh"
#include "StepMax.hh"
// Temp? 
#include "G4RadioactiveDecayPhysics.hh"
#include "G4HadronElasticPhysicsHP.hh"
#include "G4HadronPhysicsQGSP_BIC_HP.hh"
#include "G4IonElasticPhysics.hh"
#include "G4IonPhysics.hh"
#include "G4StoppingPhysics.hh"

////////////////////////////////////////////////////////////////////////////////
///
PhysicsList::PhysicsList(){

  SetDefaultCutValue(0.5 * mm);
  SetVerboseLevel(0);

  /// Run actual physics constructor with the user-specified physics lists
  AddPhysicsList(G4String(Service<ConfigSvc>()->GetValue<std::string>("RunSvc", "Physics")));

  /// Decay physics and all particles ctr instance
  m_decayPhysicsModelCtr = std::make_unique<G4DecayPhysics>();

  auto m_stepLimitPhysicsModelCtr = std::make_unique<G4StepLimiterPhysics>();
  m_stepLimitPhysicsModelCtr->SetApplyToAll(true);

}

////////////////////////////////////////////////////////////////////////////////
///
void PhysicsList::ConstructParticle() {
  m_decayPhysicsModelCtr->ConstructParticle();
}

////////////////////////////////////////////////////////////////////////////////
///
void PhysicsList::AddStepMax()
{
  // Step limitation seen as a process
  StepMax* stepMaxProcess = new StepMax();
  stepMaxProcess->SetMaxStep(0.5 * mm);

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


// ////////////////////////////////////////////////////////////////////////////////
// ///
// void PhysicsList::ConstructProcess() {

//   /// transportation
//   AddTransportation();
//   // AddStepMax();

//   /// electromagnetic physics list
//   m_emPhysicsModelCtr->ConstructProcess();

//    /// EmStandard additional parameters
//   G4EmParameters* param = G4EmParameters::Instance();
//   /// Auger electron production enabled
//   param->SetAuger(true);
//   /// Auger cascade enabled 
//   param->SetAugerCascade(true);
//   /// atomic de-excitation enabled
//   param->SetPixe(true);
//   /// Fluorescence enabled
//   param->SetFluo(true);
//   ///
//   param->SetDeexcitationIgnoreCut(true);
//   // param->SetLowestElectronEnergy(10*eV);

//   /// decay physics list
//   m_decayPhysicsModelCtr->ConstructProcess();
// }


// TEMP?:
////////////////////////////////////////////////////////////////////////////////
///  Defines all physics processes used in the simulation.
void PhysicsList::ConstructProcess() {

  // --- Mandatory transportation process ---
  AddTransportation();  // Enables particles to move through geometry

  // --- Electromagnetic physics processes (standard EM model) ---
  m_emPhysicsModelCtr->ConstructProcess();  // Custom EM physics builder

  // --- Additional EM processes (optical effects) ---
  auto extraPhysics = new G4EmExtraPhysics();  // Includes Cerenkov radiation, scintillation, synchrotron radiation
  extraPhysics->ConstructProcess();

  // --- Standard particle decays (e.g., muon, pion, kaon) ---
  auto decayPhysics = new G4DecayPhysics();
  decayPhysics->ConstructProcess();

  // --- Radioactive decay (e.g., beta, alpha, gamma emissions from unstable nuclei) ---
  auto radioactiveDecay = new G4RadioactiveDecayPhysics();
  radioactiveDecay->ConstructProcess();

  // --- Hadronic elastic scattering (high-precision models for neutrons) ---
  auto hadronElasticPhysics = new G4HadronElasticPhysicsHP();
  hadronElasticPhysics->ConstructProcess();

  // --- Hadronic inelastic scattering (e.g., neutron capture, spallation) ---
  auto hadronInelasticPhysics = new G4HadronPhysicsQGSP_BIC_HP();
  hadronInelasticPhysics->ConstructProcess();

  // --- Elastic scattering for ions (e.g., protons, alpha particles, heavier nuclei) ---
  auto ionElasticPhysics = new G4IonElasticPhysics();
  ionElasticPhysics->ConstructProcess();

  // --- Ion interaction models: ionization, nuclear interactions ---
  auto ionPhysics = new G4IonPhysics();
  ionPhysics->ConstructProcess();

  // --- Stopping physics: energy loss and annihilation for low-energy ions ---
  auto stoppingPhysics = new G4StoppingPhysics();
  stoppingPhysics->ConstructProcess();

  // --- Electromagnetic parameters configuration (critical for accurate results) ---
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

  // Finer step control for better tracking of energy loss and scattering
  param->SetStepFunction(0.2, 100*um);  // (dE/dx variance factor, step size limit)

  // Enable Mott correction for more accurate electron scattering (important for detectors, phantoms)
  param->SetUseMottCorrection(true);

  // Use updated ICRU90 data for electron interactions
  param->SetUseICRU90Data(true);


  // --- Optional: user-defined step limit process ---
  // AddStepMax();  // Enable this if you want to restrict maximum step size manually
}

////////////////////////////////////////////////////////////////////////////////
///
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
