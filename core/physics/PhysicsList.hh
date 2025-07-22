/**
*
* \author B.Rachwal (brachwal [at] agh.edu.pl)
* \date 08.04.2020
*
*/

#ifndef Dose3D_PHYSICSLIST_H
#define Dose3D_PHYSICSLIST_H

// Geant4 tool to manage physics processes
#include "G4VModularPhysicsList.hh"

// Geant4 electromagnetic physics
#include "G4EmExtraPhysics.hh"
#include "G4OpticalPhysics.hh"
#include "G4StepLimiterPhysics.hh"

// Geant4 decay
#include "G4DecayPhysics.hh"
#include "G4RadioactiveDecayPhysics.hh"

// Geant4 hadronic physics
#include "G4HadronElasticPhysicsHP.hh"
#include "G4HadronPhysicsQGSP_BIC_HP.hh"
#include "G4IonElasticPhysics.hh"
#include "G4IonPhysics.hh"
#include "G4StoppingPhysics.hh"

#include "globals.hh"
#include <memory>

class G4VPhysicsConstructor;

///\class PhysicsList
class PhysicsList : public G4VModularPhysicsList {
  public:
    ///
    PhysicsList();

    ///
    ~PhysicsList() = default;

    ///
    void ConstructParticle() override;

    ///
    void AddStepMax();   

    ///
    void AddPhysicsList(const G4String &name);

    ///
    void ConstructProcess() override;

    ///
    bool m_enableExtraProcesses = false;

    /// Manual step limiter
    double m_stepMax = 0.0;

  private:
    ///
    G4String m_emPhysicsModelName = "emstandard_opt3";

    ///
    std::unique_ptr<G4VPhysicsConstructor> m_emPhysicsModelCtr;

    // EM and decay
    std::unique_ptr<G4DecayPhysics> m_decayPhysicsModelCtr;
    std::unique_ptr<G4RadioactiveDecayPhysics> m_radioactiveDecayPhysicsCtr;
    std::unique_ptr<G4EmExtraPhysics> m_extraPhysicsCtr;
    std::unique_ptr<G4OpticalPhysics> m_opticalPhysicsCtr;

    // Hadronic
    std::unique_ptr<G4HadronElasticPhysicsHP> m_hadronElasticPhysicsCtr;
    std::unique_ptr<G4HadronPhysicsQGSP_BIC_HP> m_hadronInelasticPhysicsCtr;
    std::unique_ptr<G4IonElasticPhysics> m_ionElasticPhysicsCtr;
    std::unique_ptr<G4IonPhysics> m_ionPhysicsCtr;
    std::unique_ptr<G4StoppingPhysics> m_stoppingPhysicsCtr;
    std::unique_ptr<G4StepLimiterPhysics> m_stepLimitPhysicsCtr;

};

#endif
