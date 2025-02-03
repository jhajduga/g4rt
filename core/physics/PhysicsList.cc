#include "PhysicsList.hh"
#include "Services.hh"

#include "G4ProcessManager.hh"
#include "G4EmStandardPhysics_option3.hh"
#include "G4EmStandardPhysics_option4.hh"
#include "G4EmLivermorePhysics.hh"
#include "G4EmPenelopePhysics.hh"
#include "G4DecayPhysics.hh"
#include "G4StepLimiterPhysics.hh"
#include "G4EmParameters.hh"
#include "StepMax.hh"

////////////////////////////////////////////////////////////////////////////////
///
PhysicsList::PhysicsList(){

  SetDefaultCutValue(0.1 * mm);
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
  stepMaxProcess->SetMaxStep(0.1 * mm);

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
///
void PhysicsList::ConstructProcess() {

  /// transportation
  AddTransportation();
  AddStepMax();

  /// electromagnetic physics list
  m_emPhysicsModelCtr->ConstructProcess();

  //  /// EmStandard additional parameters
  // G4EmParameters* param = G4EmParameters::Instance();
  // /// Auger electron production enabled
  // param->SetAuger(true);
  // /// Auger cascade enabled 
  // param->SetAugerCascade(true);
  // /// atomic de-excitation enabled
  // param->SetPixe(true);
  // /// Fluorescence enabled
  // param->SetFluo(true);
  // ///
  // param->SetDeexcitationIgnoreCut(true);
  // param->SetLowestElectronEnergy(10*eV);

  /// decay physics list
  m_decayPhysicsModelCtr->ConstructProcess();
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
