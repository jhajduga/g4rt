#include "StepMax.hh"



/**
 * @brief Constructs a StepMax process with the specified name and sets the maximum charged particle step length to the largest possible value.
 *
 * @param processName Name of the process.
 */
StepMax::StepMax(const G4String& processName)
: G4VDiscreteProcess(processName),MaxChargedStep(DBL_MAX){}



/**
 * @brief Destroys the StepMax process object.
 *
 * Default destructor with no additional cleanup.
 */
StepMax::~StepMax() {}



/**
 * @brief Determines if the step limitation process applies to the given particle.
 *
 * @param particle The particle definition to check.
 * @return G4bool True if the particle has nonzero electric charge; otherwise, false.
 */
G4bool StepMax::IsApplicable(const G4ParticleDefinition& particle) 
{ 
    return (particle.GetPDGCharge() != 0.);
}


    
/**
 * @brief Sets the maximum allowed step length for charged particles.
 *
 * @param step The maximum step length to be enforced for charged particles.
 */
void StepMax::SetMaxStep(G4double step) {MaxChargedStep = step;}



/**
 * @brief Returns the maximum allowed step length for charged particles after a simulation step.
 *
 * Sets the force condition to "NotForced" and provides the current maximum step length, which acts as an advisory limit for charged particle steps.
 *
 * @param condition Pointer to the force condition, set to NotForced.
 * @return G4double The maximum allowed step length for charged particles.
 */
G4double StepMax::PostStepGetPhysicalInteractionLength(const G4Track&,
                                                G4double,
                                                G4ForceCondition* condition )
{
    // condition is set to "Not Forced"
    *condition = NotForced;

    return MaxChargedStep;
}



/**
 * @brief Performs no action during the post-step phase and returns the initialized particle change.
 *
 * Initializes the particle change object with the current track and returns it without modifying the particle state. This method effectively enforces the step limit without altering the particle's properties.
 *
 * @return Pointer to the initialized particle change object.
 */
G4VParticleChange* StepMax::PostStepDoIt(const G4Track& aTrack, const G4Step&)
{
    // do nothing
    aParticleChange.Initialize(aTrack);
    return &aParticleChange;
}
