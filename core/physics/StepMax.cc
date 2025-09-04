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
 * @brief Returns whether this process applies to the given particle (applies to charged particles).
 *
 * @param particle Particle definition to test.
 * @return G4bool True if the particle has nonzero electric charge; otherwise false.
 */
G4bool StepMax::IsApplicable(const G4ParticleDefinition& particle) 
{ 
    return (particle.GetPDGCharge() != 0.);
}


    
/**
 * @brief Set the maximum step length enforced for charged particles.
 *
 * This sets the internal MaxChargedStep value used by the process to limit
 * the proposed step length for charged particles. The default is DBL_MAX.
 *
 * @param step Maximum allowed step length (stored verbatim).
 */
void StepMax::SetMaxStep(G4double step) {MaxChargedStep = step;}



/**
 * @brief Provide the advisory maximum step length for the process.
 *
 * Sets the force condition to NotForced and returns the currently configured
 * maximum charged-particle step length. The function ignores the input track
 * and proposed step length; it only supplies the advisory limit stored in
 * MaxChargedStep.
 *
 * @param condition Output pointer that will be set to NotForced.
 * @return G4double The configured maximum step length for charged particles.
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
